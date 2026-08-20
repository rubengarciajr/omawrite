#include <QtTest>
#include <QFont>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>

#include "backend.h"
#include "markdownhighlighter.h"

class OmawriteTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QVERIFY(m_settingsDirectory.isValid());
        QQuickStyle::setStyle(QStringLiteral("Material"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_settingsDirectory.path());
    }

    void countsWords() {
        QCOMPARE(Backend::countWords(QStringLiteral("one two-three don't 42")), 4);
        QCOMPARE(Backend::countWords(QStringLiteral("你好 世界")), 2);
        QCOMPARE(Backend::countWords(QString()), 0);
    }

    void normalizesLinks() {
        QCOMPARE(Backend::normalizedLinkUrl(QStringLiteral("www.example.com/path")),
                 QStringLiteral("https://www.example.com/path"));
        QCOMPARE(Backend::normalizedLinkUrl(QStringLiteral("mailto:writer@example.com")),
                 QStringLiteral("mailto:writer@example.com"));
        QVERIFY(Backend::normalizedLinkUrl(QStringLiteral("example.com")).isEmpty());
        QVERIFY(Backend::normalizedLinkUrl(QStringLiteral("file:///tmp/private")).isEmpty());
    }

    void suggestsSafeNames() {
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("My first draft\nBody")),
                 QStringLiteral("My first draft.md"));
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("A/B")), QStringLiteral("A-B.md"));
        QCOMPARE(Backend::suggestedFileName(QString()), QStringLiteral("Untitled.md"));
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("Already.md")),
                 QStringLiteral("Already.md"));
    }

    void resolvesUserFilePaths() {
        QVERIFY(Backend::localUrlFromPath(QString()).isEmpty());

        const QString relative = QStringLiteral("notes.md");
        const QUrl relativeUrl = Backend::localUrlFromPath(relative);
        QVERIFY(relativeUrl.isLocalFile());
        QCOMPARE(relativeUrl.toLocalFile(), QFileInfo(relative).absoluteFilePath());

        const QString absolute = QDir::temp().filePath(QStringLiteral("draft.md"));
        QCOMPARE(Backend::localUrlFromPath(absolute).toLocalFile(),
                 QFileInfo(absolute).absoluteFilePath());

        const QUrl fileUrl = Backend::localUrlFromPath(QUrl::fromLocalFile(absolute).toString());
        QCOMPARE(fileUrl.toLocalFile(), QFileInfo(absolute).absoluteFilePath());
    }

    void findsInlineMarkdownRanges() {
        const auto markup = MarkdownHighlighter::inlineMarkup(
            QStringLiteral("**bold** and *italic* and [site](https://example.com)"));
        QCOMPARE(markup.size(), 3);
        QCOMPARE(markup.at(0).content.start, 2);
        QCOMPARE(markup.at(0).content.length, 4);
        QCOMPARE(markup.at(2).content.length, 4);
        QCOMPARE(markup.at(2).markers[0].length, 1);
    }

    void stylesHeadingLevelsAndListMarkers() {
        QTextDocument document;
        QFont bodyFont(QStringLiteral("iA Writer Mono S"));
        bodyFont.setPixelSize(20);
        document.setDefaultFont(bodyFont);

        MarkdownHighlighter highlighter(&document);
        highlighter.setColors(QStringLiteral("#101820"), QStringLiteral("#e8eef2"),
                              QStringLiteral("#45aabb"), QStringLiteral("#778899"),
                              QStringLiteral("#334455"));
        highlighter.setTypography(20, 36, 30, 24);
        document.setPlainText(QStringLiteral(
            "# Heading one\n## Heading two\n### Heading three\n- Bullet\n- [ ] Task\n> Readable quote"));
        highlighter.rehighlight();

        const auto formatAt = [](const QTextBlock &block, int position) {
            const QList<QTextLayout::FormatRange> formats = block.layout()->formats();
            for (const QTextLayout::FormatRange &range : formats) {
                if (position >= range.start && position < range.start + range.length)
                    return range.format;
            }
            return QTextCharFormat();
        };

        QTextBlock block = document.firstBlock();
        QCOMPARE(formatAt(block, 2).font().pixelSize(), 36);
        block = block.next();
        QCOMPARE(formatAt(block, 3).font().pixelSize(), 30);
        block = block.next();
        QCOMPARE(formatAt(block, 4).font().pixelSize(), 24);
        block = block.next();
        QCOMPARE(formatAt(block, 0).foreground().color(), QColor(QStringLiteral("#45aabb")));
        block = block.next();
        QCOMPARE(formatAt(block, 3).foreground().color(), QColor(QStringLiteral("#45aabb")));
        block = block.next();
        QCOMPARE(formatAt(block, 0).foreground().color(), QColor(QStringLiteral("#45aabb")));
        QCOMPARE(formatAt(block, 2).foreground().color(), QColor(QStringLiteral("#e8eef2")));
    }

    void loadsCurrentOmarchyTheme() {
        QTemporaryDir homeDirectory;
        QVERIFY(homeDirectory.isValid());

        const QByteArray originalHome = qgetenv("HOME");
        struct HomeRestorer {
            QByteArray value;
            ~HomeRestorer() { qputenv("HOME", value); }
        } restoreHome{originalHome};
        QVERIFY(qputenv("HOME", homeDirectory.path().toUtf8()));

        const QString themeDirectory = homeDirectory.path()
            + QStringLiteral("/.local/state/omarchy/current/theme");
        QVERIFY(QDir().mkpath(themeDirectory));

        QFile colorsFile(themeDirectory + QStringLiteral("/colors.toml"));
        QVERIFY(colorsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        const QByteArray palette(
            "mode = \"light\"\n"
            "accent = \"#112233\"\n"
            "selection = \"#445566\"\n"
            "muted = \"#778899\"\n"
            "red = \"#aa3344\"\n"
            "background = \"#fefefe\"\n"
            "foreground = \"#101010\"\n");
        QCOMPARE(colorsFile.write(palette), qint64(palette.size()));
        colorsFile.close();

        Backend backend;
        QCOMPARE(backend.themeBackground(), QStringLiteral("#fefefe"));
        QCOMPARE(backend.themeForeground(), QStringLiteral("#101010"));
        QCOMPARE(backend.themeAccent(), QStringLiteral("#112233"));
        QCOMPARE(backend.themeSelection(), QStringLiteral("#445566"));
        QCOMPARE(backend.themeMuted(), QStringLiteral("#778899"));
        QCOMPARE(backend.themeUrgent(), QStringLiteral("#aa3344"));
        QVERIFY(!backend.darkMode());
    }

    void resolvesOmarchyMenuRoles() {
        QTemporaryDir homeDirectory;
        QVERIFY(homeDirectory.isValid());

        const QByteArray originalHome = qgetenv("HOME");
        struct HomeRestorer {
            QByteArray value;
            ~HomeRestorer() { qputenv("HOME", value); }
        } restoreHome{originalHome};
        QVERIFY(qputenv("HOME", homeDirectory.path().toUtf8()));

        const QString themeDirectory = homeDirectory.path()
            + QStringLiteral("/.local/state/omarchy/current/theme");
        QVERIFY(QDir().mkpath(themeDirectory));

        QFile colorsFile(themeDirectory + QStringLiteral("/colors.toml"));
        QVERIFY(colorsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        colorsFile.write("background = \"#101820\"\nforeground = \"#e8eef2\"\n"
                         "accent = \"#45aabb\"\n");
        colorsFile.close();

        QFile shellFile(themeDirectory + QStringLiteral("/shell.toml"));
        QVERIFY(shellFile.open(QIODevice::WriteOnly | QIODevice::Text));
        shellFile.write("[menu] # surface overrides\nbackground = \"#182430\" # card\n"
                        "border = accent\nselected-background = foreground\n"
                        "selected-background-alpha = 0.08\nselected-text = accent\n");
        shellFile.close();

        Backend backend;
        QCOMPARE(backend.themeMenuBackground(), QStringLiteral("#182430"));
        QCOMPARE(backend.themeMenuBorder(), QStringLiteral("#45aabb"));
        QCOMPARE(backend.themeMenuSelectedBackground(), QStringLiteral("#14e8eef2"));
        QCOMPARE(backend.themeMenuSelectedText(), QStringLiteral("#45aabb"));
    }

    void ignoresFileWatcherEventsForSavedContents() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString path = directory.filePath(QStringLiteral("first-save.md"));
        Backend backend;
        QSignalSpy externalChangeSpy(&backend, &Backend::externalChangeDetected);

        backend.saveAs(QUrl::fromLocalFile(path));
        QVERIFY(QFileInfo::exists(path));

        QFile sameContents(path);
        QVERIFY(sameContents.open(QIODevice::WriteOnly | QIODevice::Truncate));
        sameContents.close();
        QTest::qWait(100);
        QCOMPARE(externalChangeSpy.count(), 0);

        QFile changedContents(path);
        QVERIFY(changedContents.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(changedContents.write("changed elsewhere"), qint64(17));
        changedContents.close();
        QTRY_COMPARE(externalChangeSpy.count(), 1);
    }

    void keepsCursorAndSelectionStableAcrossInsertions() {
        const QString mutationsPath = QFINDTESTDATA("../src/EditorMutations.js");
        QVERIFY(!mutationsPath.isEmpty());

        QQmlEngine engine;
        QQmlComponent component(&engine);
        const QByteArray harness = R"QML(
            import QtQuick
            import "EditorMutations.js" as EditorMutations

            TextEdit {
                property string insertionText
                property int insertionCursor
                property string wrappedText
                property int wrappedSelectionStart
                property int wrappedSelectionEnd

                Component.onCompleted: {
                    text = "alpha omega";
                    cursorPosition = 5;
                    EditorMutations.replaceRange(this, 5, 5, "one\r\ntwo");
                    insertionText = text;
                    insertionCursor = cursorPosition;

                    text = "alpha beta omega";
                    select(6, 10);
                    EditorMutations.replaceRange(this, selectionStart, selectionEnd,
                                                 "**beta**", 2, 6);
                    wrappedText = text;
                    wrappedSelectionStart = selectionStart;
                    wrappedSelectionEnd = selectionEnd;
                }
            }
        )QML";
        const QUrl harnessUrl = QUrl::fromLocalFile(
            QFileInfo(mutationsPath).absolutePath() + QStringLiteral("/MutationHarness.qml"));
        component.setData(harness, harnessUrl);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> editor(component.create());
        QVERIFY2(editor, qPrintable(component.errorString()));

        QCOMPARE(editor->property("insertionText").toString(),
                 QStringLiteral("alphaone\ntwo omega"));
        QCOMPARE(editor->property("insertionCursor").toInt(), 12);
        QCOMPARE(editor->property("wrappedText").toString(),
                 QStringLiteral("alpha **beta** omega"));
        QCOMPARE(editor->property("wrappedSelectionStart").toInt(), 8);
        QCOMPARE(editor->property("wrappedSelectionEnd").toInt(), 12);
    }

    void savesAndOpensFromFooterButtons() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QVERIFY(window->findChild<QObject *>(QStringLiteral("sourceEditor")));
        QVERIFY(!window->findChild<QObject *>(QStringLiteral("renderedPreview")));
        QVERIFY(!window->findChild<QObject *>(QStringLiteral("modeToggle")));

        QObject *saveButton = window->findChild<QObject *>(QStringLiteral("saveButton"));
        QObject *openButton = window->findChild<QObject *>(QStringLiteral("openButton"));
        QObject *typographyButton = window->findChild<QObject *>(QStringLiteral("typographyButton"));
        QObject *typographyPanel = window->findChild<QObject *>(QStringLiteral("typographyPanel"));
        QVERIFY(saveButton);
        QVERIFY(openButton);
        QVERIFY(typographyButton);
        QVERIFY(typographyPanel);

        QSignalSpy saveDialogSpy(&backend, &Backend::saveDialogRequested);
        QVERIFY(QMetaObject::invokeMethod(saveButton, "clicked"));
        QCOMPARE(saveDialogSpy.count(), 1);

        QSignalSpy openDialogSpy(&backend, &Backend::openDialogRequested);
        QVERIFY(QMetaObject::invokeMethod(openButton, "clicked"));
        QCOMPARE(openDialogSpy.count(), 1);

        QVERIFY(QMetaObject::invokeMethod(typographyButton, "clicked"));
        QVERIFY(window->property("typographyOpen").toBool());
    }

    void opensAndAppliesSlashCommands() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QObject *palette = window->findChild<QObject *>(QStringLiteral("commandPalette"));
        QVERIFY(editor);
        QVERIFY(palette);

        const QVariantList commands = palette->property("commands").toList();
        QCOMPARE(commands.size(), 8);
        QStringList commandIds;
        for (const QVariant &command : commands)
            commandIds.append(command.toMap().value(QStringLiteral("id")).toString());
        QCOMPARE(commandIds,
                 QStringList({QStringLiteral("text"), QStringLiteral("todo"),
                              QStringLiteral("heading1"), QStringLiteral("heading2"),
                              QStringLiteral("heading3"), QStringLiteral("bullet"),
                              QStringLiteral("numbered"), QStringLiteral("quote")}));

        editor->setProperty("text", QStringLiteral("/hea"));
        editor->setProperty("cursorPosition", 4);
        QVERIFY(window->property("commandOpen").toBool());
        QCOMPARE(window->property("commandQuery").toString(), QStringLiteral("hea"));

        QVERIFY(QMetaObject::invokeMethod(palette, "activateSelected"));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("# "));
        QVERIFY(!window->property("commandOpen").toBool());

        editor->setProperty("text", QStringLiteral("  /quote"));
        editor->setProperty("cursorPosition", 8);
        QVERIFY(window->property("commandOpen").toBool());
        QVERIFY(QMetaObject::invokeMethod(
            editor, "applySlashCommand",
            Q_ARG(QVariant, QVariant(QStringLiteral("quote")))));
        QCOMPARE(editor->property("text").toString(), QStringLiteral("  > "));
    }

    void persistsTypographyFoundation() {
        Backend backend;
        backend.resetTypography();
        QCOMPARE(backend.bodyFontSize(), 20);
        QCOMPARE(backend.heading1FontSize(), 34);
        QCOMPARE(backend.heading2FontSize(), 28);
        QCOMPARE(backend.heading3FontSize(), 24);

        QSignalSpy typographySpy(&backend, &Backend::typographyChanged);
        backend.setTypographyLevel(QStringLiteral("body"), 22);
        backend.setTypographyLevel(QStringLiteral("heading1"), 40);
        backend.setTypographyLevel(QStringLiteral("heading2"), 29);
        backend.setTypographyLevel(QStringLiteral("heading3"), 23);
        QCOMPARE(typographySpy.count(), 4);

        Backend restored;
        QCOMPARE(restored.bodyFontSize(), 22);
        QCOMPARE(restored.heading1FontSize(), 40);
        QCOMPARE(restored.heading2FontSize(), 29);
        QCOMPARE(restored.heading3FontSize(), 23);
        restored.resetTypography();
    }

    void scalesTextWithDesktopTextSize() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QVERIFY(editor);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 20);

        // `omarchy display text size 16` sets the GNOME factor to 16/12.
        backend.setTextScale(16.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 27);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 27);

        backend.setTextScale(9.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 15);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 15);

        backend.setTextScale(1.0);
        backend.setTypographyLevel(QStringLiteral("body"), 24);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 24);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 24);
        backend.resetTypography();
    }

    void remembersLastSaveDirectory() {
        QTemporaryDir saveDirectory;
        QVERIFY(saveDirectory.isValid());

        const QString savedPath = saveDirectory.filePath(QStringLiteral("first.md"));
        Backend savedDocument;
        savedDocument.saveAs(QUrl::fromLocalFile(savedPath));

        Backend nextDocument;
        QSignalSpy saveDialogSpy(&nextDocument, &Backend::saveDialogRequested);
        nextDocument.saveAsDialog();
        QCOMPARE(saveDialogSpy.count(), 1);

        const QUrl suggestedUrl = saveDialogSpy.takeFirst().constFirst().toUrl();
        QCOMPARE(QFileInfo(suggestedUrl.toLocalFile()).absolutePath(),
                 saveDirectory.path());
        QCOMPARE(QFileInfo(suggestedUrl.toLocalFile()).fileName(),
                 QStringLiteral("Untitled.md"));

        QSettings().setValue(QStringLiteral("file/lastSaveDirectory"),
                             saveDirectory.filePath(QStringLiteral("missing")));
        Backend fallbackDocument;
        QSignalSpy fallbackDialogSpy(&fallbackDocument, &Backend::saveDialogRequested);
        fallbackDocument.saveAsDialog();
        const QUrl fallbackUrl = fallbackDialogSpy.takeFirst().constFirst().toUrl();
        QCOMPARE(QFileInfo(fallbackUrl.toLocalFile()).absolutePath(), QDir::homePath());
    }

private:
    QTemporaryDir m_settingsDirectory;
};

QTEST_MAIN(OmawriteTest)
#include "tst_omawrite.moc"
