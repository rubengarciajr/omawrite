#pragma once

#include <QObject>
#include <QPointer>
#include <QByteArray>
#include <QFileSystemWatcher>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

class MarkdownHighlighter;
class QProcess;
class QTextDocument;
class QWindow;
class QLockFile;

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileUrlChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int wordCount READ wordCount NOTIFY wordCountChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(qreal textScale READ textScale WRITE setTextScale NOTIFY textScaleChanged)
    Q_PROPERTY(QString themeBackground READ themeBackground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeForeground READ themeForeground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeAccent READ themeAccent NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeSelection READ themeSelection NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeMuted READ themeMuted NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeUrgent READ themeUrgent NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themePopupBackground READ themePopupBackground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themePopupText READ themePopupText NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themePopupBorder READ themePopupBorder NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeMenuBackground READ themeMenuBackground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeMenuText READ themeMenuText NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeMenuBorder READ themeMenuBorder NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeMenuSelectedBackground READ themeMenuSelectedBackground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeMenuSelectedText READ themeMenuSelectedText NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeMenuSelectedBorder READ themeMenuSelectedBorder NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeNormalFill READ themeNormalFill NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeHoverFill READ themeHoverFill NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themePressedFill READ themePressedFill NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeFocusFill READ themeFocusFill NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeNormalBorder READ themeNormalBorder NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeHoverBorder READ themeHoverBorder NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeFocusBorder READ themeFocusBorder NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeTransparent READ themeTransparent CONSTANT)
    Q_PROPERTY(int themeCornerRadius READ themeCornerRadius NOTIFY themeMetricsChanged)
    Q_PROPERTY(qreal themeSpacingScale READ themeSpacingScale NOTIFY themeMetricsChanged)
    Q_PROPERTY(int bodyFontSize READ bodyFontSize NOTIFY typographyChanged)
    Q_PROPERTY(int heading1FontSize READ heading1FontSize NOTIFY typographyChanged)
    Q_PROPERTY(int heading2FontSize READ heading2FontSize NOTIFY typographyChanged)
    Q_PROPERTY(int heading3FontSize READ heading3FontSize NOTIFY typographyChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    ~Backend() override;

    void setParentWindow(QWindow *window);

    QUrl fileUrl() const { return m_fileUrl; }
    QString fileName() const;

    bool modified() const { return m_modified; }
    QString status() const { return m_status; }
    int wordCount() const { return m_wordCount; }
    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool darkMode);
    qreal textScale() const { return m_textScale; }
    void setTextScale(qreal textScale);
    QString themeBackground() const { return m_themeBackground; }
    QString themeForeground() const { return m_themeForeground; }
    QString themeAccent() const { return m_themeAccent; }
    QString themeSelection() const { return m_themeSelection; }
    QString themeMuted() const { return m_themeMuted; }
    QString themeUrgent() const { return m_themeUrgent; }
    QString themePopupBackground() const { return m_themePopupBackground; }
    QString themePopupText() const { return m_themePopupText; }
    QString themePopupBorder() const { return m_themePopupBorder; }
    QString themeMenuBackground() const { return m_themeMenuBackground; }
    QString themeMenuText() const { return m_themeMenuText; }
    QString themeMenuBorder() const { return m_themeMenuBorder; }
    QString themeMenuSelectedBackground() const { return m_themeMenuSelectedBackground; }
    QString themeMenuSelectedText() const { return m_themeMenuSelectedText; }
    QString themeMenuSelectedBorder() const { return m_themeMenuSelectedBorder; }
    QString themeNormalFill() const { return m_themeNormalFill; }
    QString themeHoverFill() const { return m_themeHoverFill; }
    QString themePressedFill() const { return m_themePressedFill; }
    QString themeFocusFill() const { return m_themeFocusFill; }
    QString themeNormalBorder() const { return m_themeNormalBorder; }
    QString themeHoverBorder() const { return m_themeHoverBorder; }
    QString themeFocusBorder() const { return m_themeFocusBorder; }
    QString themeTransparent() const { return QStringLiteral("#00000000"); }
    int themeCornerRadius() const { return m_themeCornerRadius; }
    qreal themeSpacingScale() const { return m_themeSpacingScale; }
    int bodyFontSize() const { return m_bodyFontSize; }
    int heading1FontSize() const { return m_heading1FontSize; }
    int heading2FontSize() const { return m_heading2FontSize; }
    int heading3FontSize() const { return m_heading3FontSize; }
    static int countWords(const QString &text);
    static QString normalizedLinkUrl(const QString &clipboardText);
    static QString suggestedFileName(const QString &text);
    static QUrl localUrlFromPath(const QString &path);

    Q_INVOKABLE void attachDocument(QObject *textDocument);
    Q_INVOKABLE void openDialog();
    Q_INVOKABLE void open(const QUrl &url);
    Q_INVOKABLE void save();
    Q_INVOKABLE void saveForClose();
    Q_INVOKABLE void saveAsDialog();
    Q_INVOKABLE void saveAs(const QUrl &url);
    Q_INVOKABLE void fileDialogCanceled();
    Q_INVOKABLE void discardRecovery();
    Q_INVOKABLE void reloadFromDisk();
    Q_INVOKABLE void keepExternalVersion();
    Q_INVOKABLE void printDocument();
    Q_INVOKABLE void newWindow();
    Q_INVOKABLE QString clipboardUrl() const;
    Q_INVOKABLE QString clipboardText() const;
    Q_INVOKABLE void setTypographyLevel(const QString &level, int pixelSize);
    Q_INVOKABLE void resetTypography();
    Q_INVOKABLE bool editorTextChanged();
    Q_INVOKABLE QVariantList hiddenRangesAt(int position) const;
    Q_INVOKABLE void setSearchHighlight(const QString &query, int currentMatchStart);
    Q_INVOKABLE void openExternalUrl(const QUrl &url);
    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void fileUrlChanged();
    void modifiedChanged();
    void statusChanged();
    void wordCountChanged();
    void darkModeChanged();
    void textScaleChanged();
    void themeColorsChanged();
    void themeMetricsChanged();
    void typographyChanged();
    void closeAfterSave();
    void openDialogRequested();
    void saveDialogRequested(const QUrl &suggestedUrl);
    void saveSucceeded();
    void externalChangeDetected(bool deleted, bool locallyModified);

private:
    void loadDocumentText(const QString &text);
    void setFileUrl(const QUrl &url);
    void setModified(bool modified);
    void setStatus(const QString &status);
    void saveTo(const QUrl &url);
    QUrl suggestedSaveUrl() const;
    QString currentDocumentText() const;
    void setWordCount(int words);
    void refreshWordCount();
    void scheduleWordCount();
    void applyDocumentTypography();
    void reapplyTypographyToChange();
    void scheduleRecovery();
    void writeRecovery();
    void restoreRecovery();
    void clearRecovery();
    QString recoveryPath() const;
    void watchCurrentFile();
    void loadOmarchyTheme();
    void watchOmarchyTheme();
    void refreshHyprlandRounding();
    void updateHighlighterTypography();
    void saveTypography();

    QUrl m_fileUrl;
    bool m_modified = false;
    QString m_status;
    int m_wordCount = 0;
    bool m_darkMode = true;
    qreal m_textScale = 1.0;
    bool m_loading = false;
    bool m_closeAfterSave = false;
    bool m_formattingTypography = false;
    int m_formattedBlockCount = 0;
    int m_lastChangePos = 0;
    int m_lastChangeAdded = 0;
    QTimer m_wordCountTimer;
    QTimer m_recoveryTimer;
    QFileSystemWatcher m_fileWatcher;
    QPointer<QTextDocument> m_document;
    QPointer<QWindow> m_parentWindow;
    QPointer<MarkdownHighlighter> m_highlighter;
    QString m_lastDocumentText;
    QByteArray m_lastKnownFileContents;
    bool m_hasKnownFileContents = false;
    QString m_recoveryPath;
    std::unique_ptr<QLockFile> m_recoveryLock;

    QString m_themeBackground;
    QString m_themeForeground;
    QString m_themeAccent;
    QString m_themeSelection;
    QString m_themeMuted;
    QString m_themeUrgent;
    QString m_themePopupBackground;
    QString m_themePopupText;
    QString m_themePopupBorder;
    QString m_themeMenuBackground;
    QString m_themeMenuText;
    QString m_themeMenuBorder;
    QString m_themeMenuSelectedBackground;
    QString m_themeMenuSelectedText;
    QString m_themeMenuSelectedBorder;
    QString m_themeNormalFill;
    QString m_themeHoverFill;
    QString m_themePressedFill;
    QString m_themeFocusFill;
    QString m_themeNormalBorder;
    QString m_themeHoverBorder;
    QString m_themeFocusBorder;
    int m_themeCornerRadius = 8;
    qreal m_themeSpacingScale = 1.0;
    QFileSystemWatcher m_themeWatcher;
    QPointer<QProcess> m_roundingProcess;
    int m_bodyFontSize = 20;
    int m_heading1FontSize = 34;
    int m_heading2FontSize = 28;
    int m_heading3FontSize = 24;
};
