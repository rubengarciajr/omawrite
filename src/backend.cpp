#include "backend.h"

#include <QClipboard>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRect>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QHash>
#include <QMimeData>
#include <QProcess>
#include <QPrintDialog>
#include <QPrinter>
#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QScreen>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QUrl>
#include <QVariantMap>
#include <QWindow>

#include <algorithm>

#include "markdownhighlighter.h"

constexpr qreal typoraLineHeightPercent = 140;
const QString lastSaveDirectorySetting = QStringLiteral("file/lastSaveDirectory");
const QString typographyBodySetting = QStringLiteral("typography/body");
const QString typographyHeading1Setting = QStringLiteral("typography/heading1");
const QString typographyHeading2Setting = QStringLiteral("typography/heading2");
const QString typographyHeading3Setting = QStringLiteral("typography/heading3");

namespace {
using TomlValues = QHash<QString, QString>;

QString tomlValue(QString value) {
    value = value.trimmed();
    if (!value.isEmpty()
            && (value.front() == QLatin1Char('"') || value.front() == QLatin1Char('\''))) {
        const int closingQuote = value.indexOf(value.front(), 1);
        if (closingQuote > 0)
            return value.mid(1, closingQuote - 1);
    }

    const int comment = value.indexOf(QStringLiteral(" #"));
    if (comment >= 0)
        value = value.left(comment);
    return value.trimmed();
}

TomlValues readToml(const QString &path, bool sections) {
    TomlValues values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return values;

    QString section;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        if (sections && line.startsWith(QLatin1Char('['))) {
            const int closingBracket = line.indexOf(QLatin1Char(']'), 1);
            if (closingBracket > 1) {
                section = line.mid(1, closingBracket - 1).trimmed();
                continue;
            }
        }

        const int equals = line.indexOf(QLatin1Char('='));
        if (equals < 1)
            continue;

        const QString key = line.left(equals).trimmed();
        const QString fullKey = sections && !section.isEmpty()
            ? section + QLatin1Char('.') + key
            : key;
        values.insert(fullKey, tomlValue(line.mid(equals + 1)));
    }
    return values;
}

QString firstColorToken(const QString &value) {
    const QStringList tokens = value.trimmed().split(QRegularExpression(QStringLiteral("\\s+")),
                                                      Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        if (!token.endsWith(QStringLiteral("deg")))
            return token;
    }
    return value.trimmed();
}

QColor hyprlandColor(const QString &value) {
    const QString token = firstColorToken(value);
    static const QRegularExpression rgbaRe(
        QStringLiteral("^rgba\\(([0-9A-Fa-f]{8})\\)$"));
    static const QRegularExpression rgbRe(
        QStringLiteral("^rgb\\(([0-9A-Fa-f]{6})\\)$"));

    const QRegularExpressionMatch rgba = rgbaRe.match(token);
    if (rgba.hasMatch()) {
        const QString hex = rgba.captured(1);
        return QColor(hex.mid(0, 2).toInt(nullptr, 16),
                      hex.mid(2, 2).toInt(nullptr, 16),
                      hex.mid(4, 2).toInt(nullptr, 16),
                      hex.mid(6, 2).toInt(nullptr, 16));
    }

    const QRegularExpressionMatch rgb = rgbRe.match(token);
    if (rgb.hasMatch())
        return QColor(QLatin1Char('#') + rgb.captured(1));

    return QColor(token);
}

QColor resolvedColor(const QString &raw, const TomlValues &palette,
                     const TomlValues &shell, const QColor &fallback,
                     int depth = 0) {
    if (depth > 8)
        return fallback;

    const QString token = firstColorToken(raw);
    if (palette.contains(token))
        return resolvedColor(palette.value(token), palette, shell, fallback, depth + 1);
    if (shell.contains(token))
        return resolvedColor(shell.value(token), palette, shell, fallback, depth + 1);

    const QColor color = hyprlandColor(token);
    return color.isValid() ? color : fallback;
}

qreal tomlNumber(const TomlValues &values, const QString &key, qreal fallback) {
    bool ok = false;
    const qreal number = values.value(key).toDouble(&ok);
    return ok ? number : fallback;
}

QColor withAlpha(QColor color, qreal alpha) {
    color.setAlphaF(qBound(0.0, alpha, 1.0));
    return color;
}

QString qmlColor(const QColor &color) {
    return color.alpha() == 255 ? color.name(QColor::HexRgb)
                                : color.name(QColor::HexArgb);
}

}

QString Backend::normalizedLinkUrl(const QString &clipboardText) {
    QString candidate = clipboardText.trimmed();
    static const QRegularExpression lineBreakRe(QStringLiteral("[\\r\\n]"));
    const int lineBreak = candidate.indexOf(lineBreakRe);
    if (lineBreak >= 0)
        candidate = candidate.left(lineBreak).trimmed();

    if (candidate.isEmpty())
        return {};

    if (candidate.startsWith(QStringLiteral("www."), Qt::CaseInsensitive))
        candidate.prepend(QStringLiteral("https://"));

    static const QRegularExpression schemeRe(
        QStringLiteral("^[A-Za-z][A-Za-z0-9+.-]*:"));
    if (!schemeRe.match(candidate).hasMatch())
        return {};

    const QUrl url(candidate);
    if (!url.isValid() || url.scheme().isEmpty())
        return {};

    const QString scheme = url.scheme().toLower();
    const bool webUrl = scheme == QStringLiteral("http")
        || scheme == QStringLiteral("https")
        || scheme == QStringLiteral("ftp");
    if (webUrl && url.host().isEmpty())
        return {};

    if (!webUrl && scheme != QStringLiteral("mailto"))
        return {};

    return url.toString();
}

Backend::Backend(QObject *parent) : QObject(parent) {
    const QSettings settings;
    m_bodyFontSize = qBound(12, settings.value(typographyBodySetting, 20).toInt(), 36);
    m_heading1FontSize = qBound(16, settings.value(typographyHeading1Setting, 34).toInt(), 64);
    m_heading2FontSize = qBound(16, settings.value(typographyHeading2Setting, 28).toInt(), 64);
    m_heading3FontSize = qBound(16, settings.value(typographyHeading3Setting, 24).toInt(), 64);

    const QString stateDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(stateDirectory);
    // Claim an orphaned snapshot before taking an empty slot. This ensures a
    // crash in window 2 is still recovered even if window 1 exited normally.
    for (int pass = 0; pass < 2 && !m_recoveryLock; ++pass) {
        for (int slot = 0; slot < 100; ++slot) {
            const QString base = QDir(stateDirectory).filePath(
                QStringLiteral("recovery-%1").arg(slot));
            const bool snapshotExists = QFileInfo::exists(base + QStringLiteral(".json"));
            if ((pass == 0) != snapshotExists)
                continue;
            auto lock = std::make_unique<QLockFile>(base + QStringLiteral(".lock"));
            lock->setStaleLockTime(5000);
            if (lock->tryLock()) {
                m_recoveryPath = base + QStringLiteral(".json");
                m_recoveryLock = std::move(lock);
                break;
            }
        }
    }
    m_wordCountTimer.setSingleShot(true);
    m_wordCountTimer.setInterval(120);
    connect(&m_wordCountTimer, &QTimer::timeout, this, &Backend::refreshWordCount);
    m_recoveryTimer.setSingleShot(true);
    m_recoveryTimer.setInterval(750);
    connect(&m_recoveryTimer, &QTimer::timeout, this, &Backend::writeRecovery);
    connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString &path) {
                if (path != m_fileUrl.toLocalFile())
                    return;

                const bool deleted = !QFileInfo::exists(path);
                if (!deleted && m_hasKnownFileContents) {
                    QFile file(path);
                    if (file.open(QIODevice::ReadOnly)
                            && file.readAll() == m_lastKnownFileContents) {
                        // Atomic saves can replace the watched inode. Re-arm the
                        // watcher, but do not report our own save as an outside edit.
                        watchCurrentFile();
                        return;
                    }
                }

                emit externalChangeDetected(deleted, m_modified);
            });

    loadOmarchyTheme();
    watchOmarchyTheme();
    connect(&m_themeWatcher, &QFileSystemWatcher::fileChanged, this, [this]() {
        loadOmarchyTheme();
        watchOmarchyTheme();
    });
    connect(&m_themeWatcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        loadOmarchyTheme();
        watchOmarchyTheme();
    });
}

Backend::~Backend() = default;

void Backend::setParentWindow(QWindow *window) {
    m_parentWindow = window;
}

QString Backend::fileName() const {
    if (!m_fileUrl.isValid() || m_fileUrl.isEmpty())
        return QStringLiteral("Untitled.md");

    if (m_fileUrl.isLocalFile()) {
        const QFileInfo info(m_fileUrl.toLocalFile());
        if (!info.fileName().isEmpty())
            return info.fileName();
    }

    const QString name = m_fileUrl.fileName();
    return name.isEmpty() ? QStringLiteral("Untitled.md") : name;
}

void Backend::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode)
        return;

    m_darkMode = darkMode;
    loadOmarchyTheme();
    emit darkModeChanged();
}

void Backend::setTextScale(qreal textScale) {
    if (qFuzzyCompare(m_textScale, textScale))
        return;

    m_textScale = textScale;
    updateHighlighterTypography();
    emit textScaleChanged();
}

void Backend::attachDocument(QObject *textDocument) {
    auto *quickDocument = qobject_cast<QQuickTextDocument *>(textDocument);
    if (!quickDocument || !quickDocument->textDocument()) {
        setStatus(QStringLiteral("Could not attach the Markdown renderer."));
        return;
    }

    if (m_highlighter)
        delete m_highlighter.data();

    m_document = quickDocument->textDocument();
    m_lastDocumentText = m_document->toPlainText();
    m_highlighter = new MarkdownHighlighter(m_document);
    m_highlighter->setDarkMode(m_darkMode);
    updateHighlighterTypography();
    m_highlighter->setColors(m_themeBackground, m_themeForeground, m_themeAccent,
                             m_themeMuted, m_themeSelection);

    connect(m_document, &QTextDocument::contentsChange, this,
            [this](int position, int, int charsAdded) {
                if (m_formattingTypography || m_loading)
                    return;
                m_lastChangePos = position;
                m_lastChangeAdded = charsAdded;
            });

    applyDocumentTypography();
    restoreRecovery();
}

void Backend::openDialog() {
    emit openDialogRequested();
}

void Backend::open(const QUrl &url) {
    if (!url.isLocalFile()) {
        setStatus(QStringLiteral("Only local files can be opened."));
        return;
    }

    const QString path = QFileInfo(url.toLocalFile()).absoluteFilePath();
    const QUrl localUrl = QUrl::fromLocalFile(path);
    const QString targetName = QFileInfo(path).fileName();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not open %1.").arg(targetName));
        return;
    }

    const QByteArray contents = file.readAll();
    loadDocumentText(QString::fromUtf8(contents));
    clearRecovery();
    m_lastKnownFileContents = contents;
    m_hasKnownFileContents = true;
    setFileUrl(localUrl);
    watchCurrentFile();
    setModified(false);
    setStatus(QStringLiteral("Opened %1").arg(fileName()));
}

void Backend::save() {
    if (!m_fileUrl.isValid() || m_fileUrl.isEmpty()) {
        saveAsDialog();
        return;
    }

    saveTo(m_fileUrl);
}

void Backend::saveForClose() {
    if (!m_modified) {
        emit closeAfterSave();
        return;
    }

    m_closeAfterSave = true;
    save();
}

void Backend::saveAsDialog() {
    emit saveDialogRequested(suggestedSaveUrl());
}

void Backend::saveAs(const QUrl &url) {
    saveTo(url);
}

void Backend::fileDialogCanceled() {
    m_closeAfterSave = false;
}

void Backend::discardRecovery() {
    clearRecovery();
}

void Backend::reloadFromDisk() {
    if (m_fileUrl.isLocalFile())
        open(m_fileUrl);
}

void Backend::keepExternalVersion() {
    QFile file(m_fileUrl.toLocalFile());
    if (file.open(QIODevice::ReadOnly)) {
        m_lastKnownFileContents = file.readAll();
        m_hasKnownFileContents = true;
    } else {
        m_lastKnownFileContents.clear();
        m_hasKnownFileContents = false;
    }
    setModified(true);
    scheduleRecovery();
    watchCurrentFile();
    setStatus(QStringLiteral("Kept your version"));
}

void Backend::printDocument() {
    if (!m_document) {
        setStatus(QStringLiteral("There is no document to print."));
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer);
    dialog.setWindowTitle(QStringLiteral("Print %1").arg(fileName()));
    dialog.winId();
    if (dialog.windowHandle() && m_parentWindow)
        dialog.windowHandle()->setTransientParent(m_parentWindow);

    if (dialog.exec() == QDialog::Accepted) {
        QTextDocument rendered;
        rendered.setDefaultFont(m_document->defaultFont());
        rendered.setMarkdown(currentDocumentText());
        rendered.print(&printer);
    }
}

void Backend::newWindow() {
    const bool started = QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                                 QStringList());
    if (!started)
        setStatus(QStringLiteral("Could not open a new window."));
}

QString Backend::clipboardUrl() const {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return {};

    const QMimeData *mimeData = clipboard->mimeData();
    if (!mimeData)
        return {};

    if (mimeData->hasUrls()) {
        const QList<QUrl> urls = mimeData->urls();
        for (const QUrl &url : urls) {
            const QString normalized = normalizedLinkUrl(url.toString());
            if (!normalized.isEmpty())
                return normalized;
        }
    }

    if (!mimeData->hasText())
        return {};

    return normalizedLinkUrl(mimeData->text());
}

QString Backend::clipboardText() const {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return {};

    const QMimeData *mimeData = clipboard->mimeData();
    return mimeData && mimeData->hasText() ? mimeData->text() : QString();
}

void Backend::setTypographyLevel(const QString &level, int pixelSize) {
    int *target = nullptr;
    int minimum = 16;
    int maximum = 64;
    if (level == QStringLiteral("body")) {
        target = &m_bodyFontSize;
        minimum = 12;
        maximum = 36;
    } else if (level == QStringLiteral("heading1")) {
        target = &m_heading1FontSize;
    } else if (level == QStringLiteral("heading2")) {
        target = &m_heading2FontSize;
    } else if (level == QStringLiteral("heading3")) {
        target = &m_heading3FontSize;
    }

    if (!target)
        return;

    const int boundedSize = qBound(minimum, pixelSize, maximum);
    if (*target == boundedSize)
        return;

    *target = boundedSize;
    saveTypography();
    updateHighlighterTypography();
    emit typographyChanged();
}

void Backend::resetTypography() {
    if (m_bodyFontSize == 20 && m_heading1FontSize == 34
            && m_heading2FontSize == 28 && m_heading3FontSize == 24) {
        return;
    }

    m_bodyFontSize = 20;
    m_heading1FontSize = 34;
    m_heading2FontSize = 28;
    m_heading3FontSize = 24;
    saveTypography();
    updateHighlighterTypography();
    emit typographyChanged();
}

void Backend::saveTypography() {
    QSettings settings;
    settings.setValue(typographyBodySetting, m_bodyFontSize);
    settings.setValue(typographyHeading1Setting, m_heading1FontSize);
    settings.setValue(typographyHeading2Setting, m_heading2FontSize);
    settings.setValue(typographyHeading3Setting, m_heading3FontSize);
}

void Backend::updateHighlighterTypography() {
    if (!m_highlighter)
        return;

    const auto scaled = [this](int size) {
        return qMax(1, qRound(size * m_textScale));
    };
    m_highlighter->setTypography(scaled(m_bodyFontSize), scaled(m_heading1FontSize),
                                 scaled(m_heading2FontSize), scaled(m_heading3FontSize));
}

bool Backend::editorTextChanged() {
    if (m_loading || m_formattingTypography)
        return false;

    const QString text = currentDocumentText();
    if (text == m_lastDocumentText)
        return false;
    m_lastDocumentText = text;

    if (m_document) {
        const int blockCount = m_document->blockCount();
        if (blockCount > m_formattedBlockCount)
            reapplyTypographyToChange();
        m_formattedBlockCount = blockCount;
    }

    scheduleWordCount();
    setModified(true);
    setStatus(QStringLiteral("Unsaved"));
    scheduleRecovery();
    return true;
}

QVariantList Backend::hiddenRangesAt(int position) const {
    QVariantList ranges;
    if (!m_document)
        return ranges;

    const QTextBlock block =
        m_document->findBlock(qBound(0, position, m_document->characterCount() - 1));
    if (!block.isValid())
        return ranges;

    const int lineStart = block.position();
    QList<QPair<int, int>> spans;
    const QList<MarkdownHighlighter::InlineMarkup> markup =
        MarkdownHighlighter::inlineMarkup(block.text());
    for (const MarkdownHighlighter::InlineMarkup &item : markup) {
        for (const MarkdownHighlighter::Span &marker : item.markers) {
            spans.append({lineStart + marker.start,
                          lineStart + marker.start + marker.length});
        }
    }
    std::sort(spans.begin(), spans.end());

    for (const auto &span : spans) {
        ranges.append(QVariantMap{{QStringLiteral("start"), span.first},
                                  {QStringLiteral("end"), span.second}});
    }
    return ranges;
}

void Backend::setSearchHighlight(const QString &query, int currentMatchStart) {
    if (m_highlighter)
        m_highlighter->setSearch(query, currentMatchStart);
}

void Backend::openExternalUrl(const QUrl &url) {
    const QString scheme = url.scheme().toLower();
    if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")
            || scheme == QStringLiteral("mailto"))
        QDesktopServices::openUrl(url);
}

QVariantMap Backend::windowGeometry() const {
    QSettings settings;
    int x = settings.value(QStringLiteral("window/x"), -1).toInt();
    int y = settings.value(QStringLiteral("window/y"), -1).toInt();
    int width = qMax(720, settings.value(QStringLiteral("window/width"), 1280).toInt());
    int height = qMax(520, settings.value(QStringLiteral("window/height"), 820).toInt());
    const bool maximized = settings.value(QStringLiteral("window/maximized"), false).toBool();

    QRect available;
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (const QScreen *screen : screens)
        available = available.united(screen->availableGeometry());

    if (!available.isNull() && x >= 0 && y >= 0) {
        width = qMin(width, available.width());
        height = qMin(height, available.height());
        x = qBound(available.left(), x, available.right() - width + 1);
        y = qBound(available.top(), y, available.bottom() - height + 1);
    }

    return {{QStringLiteral("x"), x},
            {QStringLiteral("y"), y},
            {QStringLiteral("width"), width},
            {QStringLiteral("height"), height},
            {QStringLiteral("maximized"), maximized}};
}

void Backend::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    QSettings settings;
    if (!maximized) {
        settings.setValue(QStringLiteral("window/x"), x);
        settings.setValue(QStringLiteral("window/y"), y);
        settings.setValue(QStringLiteral("window/width"), width);
        settings.setValue(QStringLiteral("window/height"), height);
    }
    settings.setValue(QStringLiteral("window/maximized"), maximized);
}

void Backend::loadDocumentText(const QString &text) {
    if (!m_document) {
        setStatus(QStringLiteral("Could not attach the Markdown renderer."));
        return;
    }

    m_loading = true;
    m_document->setPlainText(text);
    m_lastDocumentText = text;
    m_loading = false;

    applyDocumentTypography();
    m_wordCountTimer.stop();
    setWordCount(countWords(text));
}

void Backend::setFileUrl(const QUrl &url) {
    if (m_fileUrl == url)
        return;

    m_fileUrl = url;
    emit fileUrlChanged();
    watchCurrentFile();
}

void Backend::setModified(bool modified) {
    if (m_modified == modified)
        return;

    m_modified = modified;
    emit modifiedChanged();
}

void Backend::setStatus(const QString &status) {
    if (m_status == status)
        return;

    m_status = status;
    emit statusChanged();
}

void Backend::saveTo(const QUrl &url) {
    if (!url.isLocalFile()) {
        m_closeAfterSave = false;
        setStatus(QStringLiteral("Only local files can be saved."));
        return;
    }

    const QString targetName = QFileInfo(url.toLocalFile()).fileName();
    QSaveFile file(url.toLocalFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_closeAfterSave = false;
        setStatus(QStringLiteral("Could not save %1.").arg(targetName));
        return;
    }

    const QByteArray contents = currentDocumentText().toUtf8();
    file.write(contents);

    // QSaveFile commits by replacing the target. Stop watching the old inode
    // before that replacement so our own write is not classified as external.
    const QStringList watched = m_fileWatcher.files();
    if (!watched.isEmpty())
        m_fileWatcher.removePaths(watched);

    // commit() flushes, fsyncs, and atomically renames the temp file into place,
    // returning false (and leaving the original untouched) on any write error.
    if (!file.commit()) {
        watchCurrentFile();
        m_closeAfterSave = false;
        setStatus(QStringLiteral("Could not write %1.").arg(targetName));
        return;
    }

    const bool shouldClose = m_closeAfterSave;
    m_closeAfterSave = false;
    m_lastKnownFileContents = contents;
    m_hasKnownFileContents = true;
    setFileUrl(url);
    watchCurrentFile();
    QSettings().setValue(lastSaveDirectorySetting,
                         QFileInfo(url.toLocalFile()).absolutePath());
    setModified(false);
    setStatus(QStringLiteral("Saved %1").arg(fileName()));
    clearRecovery();
    emit saveSucceeded();

    if (shouldClose)
        emit closeAfterSave();
}

void Backend::scheduleRecovery() {
    m_recoveryTimer.start();
}

QString Backend::recoveryPath() const {
    return m_recoveryPath;
}

void Backend::writeRecovery() {
    if (!m_modified)
        return;
    const QString path = recoveryPath();
    if (path.isEmpty())
        return;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    const QJsonObject recovery{{QStringLiteral("fileUrl"), m_fileUrl.toString()},
                               {QStringLiteral("text"), currentDocumentText()}};
    file.write(QJsonDocument(recovery).toJson(QJsonDocument::Compact));
    file.commit();
}

void Backend::restoreRecovery() {
    QFile file(recoveryPath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll());
    if (!json.isObject() || !json.object().contains(QStringLiteral("text")))
        return;
    const QJsonObject recovery = json.object();
    loadDocumentText(recovery.value(QStringLiteral("text")).toString());
    const QUrl recoveredUrl(recovery.value(QStringLiteral("fileUrl")).toString());
    QFile diskFile(recoveredUrl.toLocalFile());
    if (recoveredUrl.isLocalFile() && diskFile.open(QIODevice::ReadOnly)) {
        m_lastKnownFileContents = diskFile.readAll();
        m_hasKnownFileContents = true;
    } else {
        m_lastKnownFileContents.clear();
        m_hasKnownFileContents = false;
    }
    setFileUrl(recoveredUrl);
    setModified(true);
    setStatus(QStringLiteral("Recovered unsaved changes"));
}

void Backend::clearRecovery() {
    m_recoveryTimer.stop();
    QFile::remove(recoveryPath());
}

void Backend::watchCurrentFile() {
    const QStringList watched = m_fileWatcher.files();
    if (!watched.isEmpty())
        m_fileWatcher.removePaths(watched);
    if (m_fileUrl.isLocalFile() && QFileInfo::exists(m_fileUrl.toLocalFile()))
        m_fileWatcher.addPath(m_fileUrl.toLocalFile());
}

void Backend::loadOmarchyTheme() {
    const QColor fallbackBackground = m_darkMode ? QColor(QStringLiteral("#101010"))
                                                  : QColor(QStringLiteral("#ffffff"));
    const QColor fallbackForeground = m_darkMode ? QColor(QStringLiteral("#eeeeee"))
                                                  : QColor(QStringLiteral("#222324"));
    const QColor fallbackAccent = m_darkMode ? QColor(QStringLiteral("#5584aa"))
                                              : QColor(QStringLiteral("#2077b2"));
    const QColor fallbackMuted = m_darkMode ? QColor(QStringLiteral("#909191"))
                                             : QColor(QStringLiteral("#6f7378"));
    const QColor fallbackUrgent = QColor(QStringLiteral("#d75f5f"));

    const QString themePath = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current/theme");
    const TomlValues palette = readToml(themePath + QStringLiteral("/colors.toml"), false);
    TomlValues shell = readToml(themePath + QStringLiteral("/shell.toml"), true);
    const TomlValues userShell = readToml(
        QDir::homePath() + QStringLiteral("/.config/omarchy/shell.toml"), true);
    for (auto it = userShell.constBegin(); it != userShell.constEnd(); ++it)
        shell.insert(it.key(), it.value());

    const QColor background = resolvedColor(
        palette.value(QStringLiteral("background"), palette.value(QStringLiteral("color0"))),
        palette, shell, fallbackBackground);
    const QColor foreground = resolvedColor(
        palette.value(QStringLiteral("foreground"), palette.value(QStringLiteral("color7"))),
        palette, shell, fallbackForeground);
    const QColor accent = resolvedColor(
        palette.value(QStringLiteral("accent"), palette.value(QStringLiteral("color4"))),
        palette, shell, fallbackAccent);
    const QColor selection = resolvedColor(palette.value(QStringLiteral("selection")),
                                            palette, shell, accent);
    const QColor muted = resolvedColor(
        palette.value(QStringLiteral("muted"),
                      palette.value(QStringLiteral("dark_foreground"),
                                    palette.value(QStringLiteral("color8")))),
        palette, shell, fallbackMuted);
    const QColor urgent = resolvedColor(
        palette.value(QStringLiteral("red"), palette.value(QStringLiteral("color1"))),
        palette, shell, fallbackUrgent);

    const auto colorFor = [&palette, &shell](const QString &key, const QColor &fallback) {
        return shell.contains(key)
            ? resolvedColor(shell.value(key), palette, shell, fallback)
            : fallback;
    };
    const auto composed = [&shell, &colorFor](const QString &colorKey,
                                              const QString &alphaKey,
                                              const QColor &fallback,
                                              qreal fallbackAlpha) {
        return withAlpha(colorFor(colorKey, fallback),
                         tomlNumber(shell, alphaKey, fallbackAlpha));
    };

    m_themeBackground = qmlColor(background);
    m_themeForeground = qmlColor(foreground);
    m_themeAccent = qmlColor(accent);
    m_themeSelection = qmlColor(selection);
    m_themeMuted = qmlColor(muted);
    m_themeUrgent = qmlColor(urgent);

    m_themePopupBackground = qmlColor(composed(
        QStringLiteral("popups.background"), QStringLiteral("popups.background-alpha"),
        background, 1.0));
    m_themePopupText = qmlColor(colorFor(QStringLiteral("popups.text"), foreground));
    m_themePopupBorder = qmlColor(composed(
        QStringLiteral("popups.border"), QStringLiteral("popups.border-alpha"),
        accent, 1.0));

    m_themeMenuBackground = qmlColor(composed(
        QStringLiteral("menu.background"), QStringLiteral("menu.background-alpha"),
        background, 1.0));
    m_themeMenuText = qmlColor(colorFor(QStringLiteral("menu.text"), foreground));
    m_themeMenuBorder = qmlColor(composed(
        QStringLiteral("menu.border"), QStringLiteral("menu.border-alpha"),
        foreground, 1.0));
    m_themeMenuSelectedBackground = qmlColor(composed(
        QStringLiteral("menu.selected-background"),
        QStringLiteral("menu.selected-background-alpha"), foreground, 0.08));
    m_themeMenuSelectedText = qmlColor(
        colorFor(QStringLiteral("menu.selected-text"), accent));
    m_themeMenuSelectedBorder = qmlColor(composed(
        QStringLiteral("menu.selected-border"),
        QStringLiteral("menu.selected-border-alpha"), foreground, 0.0));

    const QColor normalColor = colorFor(QStringLiteral("controls.normal-color"), foreground);
    const QColor hoverColor = colorFor(QStringLiteral("controls.hover-cursor-color"), foreground);
    const QColor pressedColor = colorFor(QStringLiteral("controls.pressed-color"), hoverColor);
    const QColor focusColor = colorFor(QStringLiteral("controls.focus-color"), hoverColor);
    m_themeNormalFill = qmlColor(withAlpha(
        normalColor, tomlNumber(shell, QStringLiteral("controls.normal-fill-alpha"), 0.04)));
    m_themeHoverFill = qmlColor(withAlpha(
        hoverColor, tomlNumber(shell, QStringLiteral("controls.hover-cursor-fill-alpha"), 0.08)));
    m_themePressedFill = qmlColor(withAlpha(
        pressedColor, tomlNumber(shell, QStringLiteral("controls.pressed-fill-alpha"), 0.22)));
    m_themeFocusFill = qmlColor(withAlpha(
        focusColor, tomlNumber(shell, QStringLiteral("controls.focus-fill-alpha"), 0.08)));
    m_themeNormalBorder = qmlColor(withAlpha(
        colorFor(QStringLiteral("controls.normal-border"), normalColor),
        tomlNumber(shell, QStringLiteral("controls.normal-border-alpha"), 0.4)));
    m_themeHoverBorder = qmlColor(withAlpha(
        colorFor(QStringLiteral("controls.hover-cursor-border"), hoverColor),
        tomlNumber(shell, QStringLiteral("controls.hover-cursor-border-alpha"), 0.25)));
    m_themeFocusBorder = qmlColor(withAlpha(
        colorFor(QStringLiteral("controls.focus-border"), focusColor),
        tomlNumber(shell, QStringLiteral("controls.focus-border-alpha"), 0.25)));

    const qreal oldSpacingScale = m_themeSpacingScale;
    m_themeSpacingScale = qBound(0.25,
                                 tomlNumber(shell, QStringLiteral("spacing.scale"), 1.0),
                                 4.0);

    bool themeModeKnown = false;
    bool themeIsDark = m_darkMode;
    const QString themeMode = palette.value(QStringLiteral("mode"));
    if (themeMode == QStringLiteral("dark")) {
        themeIsDark = true;
        themeModeKnown = true;
    } else if (themeMode == QStringLiteral("light")) {
        themeIsDark = false;
        themeModeKnown = true;
    } else {
        if (background.isValid()) {
            const double luminance = 0.299 * background.redF()
                + 0.587 * background.greenF() + 0.114 * background.blueF();
            themeIsDark = luminance < 0.5;
            themeModeKnown = true;
        }
    }
    if (themeModeKnown && themeIsDark != m_darkMode) {
        m_darkMode = themeIsDark;
        emit darkModeChanged();
    }

    if (m_highlighter) {
        m_highlighter->setDarkMode(m_darkMode);
        m_highlighter->setColors(m_themeBackground, m_themeForeground, m_themeAccent,
                                 m_themeMuted, m_themeSelection);
    }

    emit themeColorsChanged();
    if (!qFuzzyCompare(oldSpacingScale, m_themeSpacingScale))
        emit themeMetricsChanged();

    refreshHyprlandRounding();
}

void Backend::watchOmarchyTheme() {
    const QStringList watched = m_themeWatcher.files() + m_themeWatcher.directories();
    if (!watched.isEmpty())
        m_themeWatcher.removePaths(watched);

    const QString currentDir = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current");
    const QString themeDir = currentDir + QStringLiteral("/theme");
    const QString colorsPath = themeDir + QStringLiteral("/colors.toml");
    const QString shellPath = themeDir + QStringLiteral("/shell.toml");
    const QString userConfigDir = QDir::homePath() + QStringLiteral("/.config/omarchy");
    const QString userShellPath = userConfigDir + QStringLiteral("/shell.toml");

    if (QDir(currentDir).exists())
        m_themeWatcher.addPath(currentDir);
    if (QDir(themeDir).exists())
        m_themeWatcher.addPath(themeDir);
    if (QFile::exists(colorsPath))
        m_themeWatcher.addPath(colorsPath);
    if (QFile::exists(shellPath))
        m_themeWatcher.addPath(shellPath);
    if (QDir(userConfigDir).exists())
        m_themeWatcher.addPath(userConfigDir);
    if (QFile::exists(userShellPath))
        m_themeWatcher.addPath(userShellPath);
}

void Backend::refreshHyprlandRounding() {
    if (qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")
            || QGuiApplication::platformName() == QLatin1String("offscreen"))
        return;

    if (!m_roundingProcess) {
        m_roundingProcess = new QProcess(this);
        connect(m_roundingProcess, &QProcess::finished, this,
                [this](int exitCode, QProcess::ExitStatus status) {
            if (status != QProcess::NormalExit || exitCode != 0 || !m_roundingProcess)
                return;

            const QJsonDocument json =
                QJsonDocument::fromJson(m_roundingProcess->readAllStandardOutput());
            if (!json.isObject())
                return;

            const int rounding = qMax(0, json.object().value(QStringLiteral("int"))
                                             .toInt(m_themeCornerRadius));
            if (rounding == m_themeCornerRadius)
                return;
            m_themeCornerRadius = rounding;
            emit themeMetricsChanged();
        });
    }

    if (m_roundingProcess->state() != QProcess::NotRunning)
        return;

    m_roundingProcess->start(QStringLiteral("hyprctl"),
                             {QStringLiteral("-j"), QStringLiteral("getoption"),
                              QStringLiteral("decoration:rounding")});
}

QUrl Backend::localUrlFromPath(const QString &path) {
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty())
        return {};

    const QUrl asUrl = QUrl::fromUserInput(trimmed, QDir::currentPath(), QUrl::AssumeLocalFile);
    if (!asUrl.isLocalFile())
        return asUrl;

    return QUrl::fromLocalFile(QFileInfo(asUrl.toLocalFile()).absoluteFilePath());
}

QUrl Backend::suggestedSaveUrl() const {
    if (m_fileUrl.isLocalFile())
        return m_fileUrl;

    const QString savedDirectory = QSettings().value(lastSaveDirectorySetting).toString();
    const QDir directory = savedDirectory.isEmpty() || !QDir(savedDirectory).exists()
        ? QDir::home()
        : QDir(savedDirectory);
    return QUrl::fromLocalFile(
        directory.filePath(suggestedFileName(currentDocumentText())));
}

QString Backend::currentDocumentText() const {
    return m_document ? m_document->toPlainText() : QString();
}

int Backend::countWords(const QString &text) {
    static const QRegularExpression wordRe(
        QStringLiteral("[\\p{L}\\p{N}]+(?:['-][\\p{L}\\p{N}]+)*"));
    int count = 0;
    QRegularExpressionMatchIterator it = wordRe.globalMatch(text);
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}

QString Backend::suggestedFileName(const QString &text) {
    QString name = text.section(QLatin1Char('\n'), 0, 0).trimmed();
    name.replace(QRegularExpression(QStringLiteral("[/\\x00-\\x1f\\x7f]")),
                 QStringLiteral("-"));
    name = name.left(120).trimmed();
    if (name.isEmpty() || name == QStringLiteral(".") || name == QStringLiteral(".."))
        name = QStringLiteral("Untitled");
    if (!name.endsWith(QStringLiteral(".md"), Qt::CaseInsensitive))
        name += QStringLiteral(".md");
    return name;
}

void Backend::setWordCount(int words) {
    if (m_wordCount == words)
        return;

    m_wordCount = words;
    emit wordCountChanged();
}

void Backend::refreshWordCount() {
    setWordCount(countWords(currentDocumentText()));
}

void Backend::scheduleWordCount() {
    m_wordCountTimer.start();
}

void Backend::applyDocumentTypography() {
    if (!m_document)
        return;

    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(typoraLineHeightPercent, QTextBlockFormat::ProportionalHeight);

    // A full pass is only used for freshly loaded/attached documents, so it is
    // safe to drop undo history here (re-enabling clears the stack anyway).
    const bool undoEnabled = m_document->isUndoRedoEnabled();
    m_document->setUndoRedoEnabled(false);

    m_formattingTypography = true;
    QTextCursor cursor(m_document);
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(blockFormat);
    m_formattingTypography = false;

    m_document->setUndoRedoEnabled(undoEnabled);

    m_formattedBlockCount = m_document->blockCount();
}

void Backend::reapplyTypographyToChange() {
    if (!m_document)
        return;

    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(typoraLineHeightPercent, QTextBlockFormat::ProportionalHeight);

    // Format only the block(s) touched by the last edit instead of the whole
    // document, and fold the change into the preceding edit command so a single
    // undo reverts both the text and its formatting.
    const int maxPos = m_document->characterCount() - 1;
    const int start = qBound(0, m_lastChangePos, maxPos);
    const int end = qBound(start, m_lastChangePos + m_lastChangeAdded, maxPos);

    m_formattingTypography = true;
    QTextCursor cursor(m_document);
    cursor.joinPreviousEditBlock();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.mergeBlockFormat(blockFormat);
    cursor.endEditBlock();
    m_formattingTypography = false;
}
