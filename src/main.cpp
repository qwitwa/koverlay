#include <QApplication>
#include <QDBusConnection>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QFileSystemWatcher>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>
#include <QFileInfo>

#include "overlay_config.h"
#include "overlay_view.h"
#include "overlay_adaptor.h"
#include "Position.h"

// ---------- Defaults ----------
static QString defaultOverlayText() {
    return QStringLiteral("⌨ Keybindings:\n• Super+Enter — Terminal\n• Ctrl+Alt+H — Toggle Overlay");
}

static QString resolveConfigPath() {
    const QByteArray env = qgetenv("KOVERLAY_CONFIG");
    if (!env.isEmpty()) {
        QString p = QString::fromLocal8Bit(env);
        QFileInfo fi(p);
        if (fi.isDir()) p = fi.filePath() + "/config.ini";
        return p;
    }
    QString cfgRoot = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (cfgRoot.isEmpty()) return {};
    QDir().mkpath(cfgRoot + "/koverlay");
    return cfgRoot + "/koverlay/config.ini";
}

static QString expandUserPath(QString p) {
    if (p.startsWith("~/")) return QDir::homePath() + p.mid(1);
    return p;
}

static QString readWholeFileUtf8(const QString &path) {
    QFile f(expandUserPath(path));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QString content = QString::fromUtf8(f.readAll());

    // Trim a single trailing newline or carriage-return+newline
    if (content.endsWith("\r\n"))
        content.chop(2);
    else if (content.endsWith('\n'))
        content.chop(1);

    return content;
}

// Triple-quoted multiline: text=""" ... """
static QString parseOverlayMultilineText(const QString &iniPath) {
    QFile f(iniPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream in(&f);
    bool inOverlay = false;
    while (!in.atEnd()) {
        QString line = in.readLine();
        const QString t = line.trimmed();
        if (t.startsWith('[') && t.endsWith(']')) {
            const QString sec = t.mid(1, t.size() - 2).trimmed();
            inOverlay = (sec.compare("overlay", Qt::CaseInsensitive) == 0);
            continue;
        }
        if (!inOverlay) continue;

        static const QRegularExpression rx(R"(^\s*text\s*=\s*(.*)\s*$)");
        const auto m = rx.match(line);
        if (!m.hasMatch()) continue;

        QString rhs = m.captured(1);

        if (rhs.startsWith(R"(""")") || rhs.startsWith("'''")) {
            const QString delim = rhs.left(3);
            QStringList out;
            QString first = rhs.mid(3);
            if (!first.isEmpty()) out << first;
            while (!in.atEnd()) {
                QString l = in.readLine();
                int idx = l.indexOf(delim);
                if (idx >= 0) {
                    out << l.left(idx);
                    break;
                }
                out << l;
            }
            return out.join('\n');
        }
        return rhs; // single-line
    }
    return {};
}

// ---------------- helpers for position + textFile path ----------------
static QString normalizedLower(QString s) {
    s = s.trimmed();
    s = s.toLower();
    return s;
}

static QString readTextFilePathFromIni(const QString &path) {
    if (path.isEmpty() || !QFile::exists(path)) return {};
    QSettings s(path, QSettings::IniFormat);
    QString grp = "overlay";
    const auto groups = s.childGroups();
    for (const auto &g: groups)
        if (g.compare("overlay", Qt::CaseInsensitive) == 0) {
            grp = g;
            break;
        }
    s.beginGroup(grp);
    const QString textFile = expandUserPath(s.value("textFile").toString());
    s.endGroup();
    return textFile;
}

static OverlayPosition readOverlayPositionFromIni(const QString &path) {
    OverlayPosition pos; // defaults set in struct (16 margins, position empty)
    // defaults per spec: top-right if absent
    pos.position = QStringLiteral("top-right");
    pos.marginTop = 16;
    pos.marginRight = 16;
    pos.marginBottom = 16;
    pos.marginLeft = 16;
    pos.x = 0;
    pos.y = 0;

    if (path.isEmpty() || !QFile::exists(path)) return pos;

    QSettings s(path, QSettings::IniFormat);
    QString grp = "overlay";
    const auto groups = s.childGroups();
    for (const auto &g: groups)
        if (g.compare("overlay", Qt::CaseInsensitive) == 0) {
            grp = g;
            break;
        }
    s.beginGroup(grp);

    pos.position = normalizedLower(s.value("position", "top-right").toString());
    pos.marginTop = s.value("marginTop", 16).toInt();
    pos.marginRight = s.value("marginRight", 16).toInt();
    pos.marginBottom = s.value("marginBottom", 16).toInt();
    pos.marginLeft = s.value("marginLeft", 16).toInt();
    pos.x = s.value("x", 0).toInt();
    pos.y = s.value("y", 0).toInt();

    s.endGroup();
    return pos;
}

// --------------------------------------------------------------------------

static void loadSettingsInto(const QString &path, OverlayConfig *oc) {
    QString text = defaultOverlayText();
    QString family;
    int size = 28;
    QString textColor = QStringLiteral("#FFFFFF"); // default white
    bool bold = true;
    double panelOpacity = 0.35;

    if (!path.isEmpty() && QFile::exists(path)) {
        QSettings s(path, QSettings::IniFormat);

        QString grp = "overlay";
        const auto groups = s.childGroups();
        for (const auto &g: groups)
            if (g.compare("overlay", Qt::CaseInsensitive) == 0) {
                grp = g;
                break;
            }

        s.beginGroup(grp);
        const QString textFile = s.value("textFile").toString();
        if (!textFile.isEmpty()) {
            const QString fileText = readWholeFileUtf8(textFile);
            if (!fileText.isEmpty()) text = fileText;
            if (oc->metaObject()->indexOfProperty("textFile") >= 0) {
                oc->setProperty("textFile", expandUserPath(textFile));
            }
        } else {
            // clear textFile in cfg if previously set
            if (oc->metaObject()->indexOfProperty("textFile") >= 0) {
                oc->setProperty("textFile", QString{});
            }
            QString multi = parseOverlayMultilineText(path);
            if (!multi.isEmpty()) {
                text = multi;
            } else {
                QString t1 = s.value("text").toString();
                if (!t1.isEmpty()) {
                    t1.replace("\\n", "\n");
                    text = t1;
                }
            }
        }
        family = s.value("fontFamily", family).toString();
        size = s.value("fontSize", size).toInt();
        textColor = s.value("textColor", textColor).toString(); // e.g. "#ffcc00" or "orange"
        bold = s.value("bold", bold).toBool(); // true/false or 1/0
        panelOpacity = s.value("panelOpacity", panelOpacity).toDouble();


        const QString posStr = normalizedLower(s.value("position", "top-right").toString());
        oc->setProperty("position", posStr);
        oc->setProperty("marginTop", s.value("marginTop", 16).toInt());
        oc->setProperty("marginRight", s.value("marginRight", 16).toInt());
        oc->setProperty("marginBottom", s.value("marginBottom", 16).toInt());
        oc->setProperty("marginLeft", s.value("marginLeft", 16).toInt());
        oc->setProperty("x", s.value("x", 0).toInt());
        oc->setProperty("y", s.value("y", 0).toInt());

        s.endGroup();
    } else {
        // default position when no file: top-right + 16s + x/y=0
        oc->setProperty("position", QStringLiteral("top-right"));
        oc->setProperty("marginTop", 16);
        oc->setProperty("marginRight", 16);
        oc->setProperty("marginBottom", 16);
        oc->setProperty("marginLeft", 16);
        oc->setProperty("x", 0);
        oc->setProperty("y", 0);
        // clear textFile if any
        if (oc->metaObject()->indexOfProperty("textFile") >= 0) {
            oc->setProperty("textFile", QString{});
        }
    }

    oc->setText(text);
    oc->setFontFamily(family);
    oc->setFontSize(size);
    oc->setTextColor(textColor);
    oc->setBold(bold);
    oc->setPanelOpacity(panelOpacity);

    qInfo() << "koverlay: applied config — len(text):" << text.size()
            << ", fontFamily:" << (family.isEmpty() ? "<default>" : family)
            << ", fontSize:" << size
            << ", textColor:" << textColor
            << ", bold:" << bold
            << ", panelOpacity:" << panelOpacity;
}

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", QByteArray("wayland"));
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("KOverlay — Wayland overlay panel");
    parser.addHelpOption();
    QCommandLineOption showOpt(QStringList{"s", "show"}, "Show overlay on start.");
    QCommandLineOption screenIdxOpt(QStringList{"S", "screen-index"}, "Screen index (0..N-1)", "index", "0");
    parser.addOption(showOpt);
    parser.addOption(screenIdxOpt);
    parser.process(app);

    auto *cfg = new OverlayConfig(&app);
    const QString cfgPath = resolveConfigPath();
    loadSettingsInto(cfgPath, cfg);

    OverlayView view(cfg);
    view.selectScreenByIndex(parser.value(screenIdxOpt).toInt());

    // ---------------- initial position apply on the window ----------------
    {
        OverlayPosition pos = readOverlayPositionFromIni(cfgPath);
        applyOverlayPosition(&view, pos);
        qInfo() << "koverlay: applied position"
                << pos.position
                << "margins L/T/R/B:" << pos.marginLeft << pos.marginTop << pos.marginRight << pos.marginBottom
                << "custom x/y:" << pos.x << pos.y;
    }
    // --------------------------------------------------------------------------

    // ---------------- INI watcher ----------------
    QFileSystemWatcher watcher;
    if (!cfgPath.isEmpty()) {
        if (!QFile::exists(cfgPath)) {
            QDir().mkpath(QFileInfo(cfgPath).dir().absolutePath());
            QFile f(cfgPath);
            f.open(QIODevice::WriteOnly);
            f.close();
        }
        watcher.addPath(cfgPath);
    }

    // ---------------- textFile watcher (file + its parent dir) -----------
    QFileSystemWatcher textWatcher;
    auto rewatchTextFile = [&](const QString &filePath) {
        if (!textWatcher.files().isEmpty()) textWatcher.removePaths(textWatcher.files());
        if (!textWatcher.directories().isEmpty()) textWatcher.removePaths(textWatcher.directories());
        if (filePath.isEmpty()) return;
        QFileInfo fi(expandUserPath(filePath));
        const auto file = fi.absoluteFilePath();
        const auto dir = fi.absoluteDir().absolutePath();
        if (QFile::exists(file)) textWatcher.addPath(file);
        if (QDir(dir).exists()) textWatcher.addPath(dir);
    };

    QString currentTextFile = readTextFilePathFromIni(cfgPath);
    rewatchTextFile(currentTextFile);

    auto reloadTextOnly = [&]() {
        if (!currentTextFile.isEmpty()) {
            const QString txt = readWholeFileUtf8(currentTextFile);
            if (!txt.isEmpty()) cfg->setText(txt);
        }
    };

    QObject::connect(&textWatcher, &QFileSystemWatcher::fileChanged, &app, [&](const QString &) {
        reloadTextOnly();
        // in case of atomic replace, reattach
        if (!currentTextFile.isEmpty()) {
            const QString f = expandUserPath(currentTextFile);
            if (!textWatcher.files().contains(f) && QFile::exists(f))
                textWatcher.addPath(f);
        }
    });
    QObject::connect(&textWatcher, &QFileSystemWatcher::directoryChanged, &app, [&](const QString &) {
        rewatchTextFile(currentTextFile);
        reloadTextOnly();
    });


    if (parser.isSet(showOpt)) view.showOverlay(); else view.hide();
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, &app,
                     [cfgPath, cfg, &watcher, &view, &currentTextFile, &rewatchTextFile]() {
                         if (!watcher.files().contains(cfgPath) && QFile::exists(cfgPath)) watcher.addPath(cfgPath);
                         loadSettingsInto(cfgPath, cfg);
                         // re-apply position on INI changes
                         OverlayPosition pos = readOverlayPositionFromIni(cfgPath);
                         applyOverlayPosition(&view, pos);

                         // Force a surface commit so anchors take effect even if size didn't change
                         if (view.isVisible()) {
                             view.hideOverlay();
                             view.showOverlay();
                         }

                         // re-target textFile watcher if changed in INI
                         QString newTextFile = readTextFilePathFromIni(cfgPath);
                         if (newTextFile != currentTextFile) {
                             currentTextFile = newTextFile;
                             rewatchTextFile(currentTextFile);
                         }
                         qInfo() << "koverlay: reloaded config from" << cfgPath;
                     });


    QDBusConnection session = QDBusConnection::sessionBus();
    session.registerService("org.erx.KOverlay");
    session.registerObject("/Overlay", &view);
    new OverlayAdaptor(&view);

    return QApplication::exec();
}
