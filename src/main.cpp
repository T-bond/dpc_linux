#include <QCommandLineParser>
#include <QFont>
#include <QFontDatabase>
#include <QApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTextStream>
#include <QTranslator>
#include <QWindow>

#include "DeviceManager.h"
#include "SingleInstance.h"
#include "SystemTray.h"
#include "IconProvider.h"

int main(int argc, char *argv[])
{
    // QApplication, not QGuiApplication: the KDE platform theme builds tray menus from widgets
    QApplication app(argc, argv);
    // the window decides: quit, or hide to the tray (see Main.qml)
    app.setQuitOnLastWindowClosed(false);
    // settings in ~/.config/DrevoPowerConsole/
    app.setOrganizationName("DrevoPowerConsole");
    app.setApplicationName("DrevoPowerConsole");

    // translations for the system language: Qt's own (dialog buttons) and the program's
    QTranslator qt_translator;
    if (qt_translator.load(QLocale(), "qt", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qt_translator);
    QTranslator translator;
    if (translator.load(QLocale(), "DrevoPowerConsole", "_", ":/i18n"))
        app.installTranslator(&translator);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Configuration tool for DREVO keyboards"));
    parser.addHelpOption();
    QCommandLineOption debug_option("debug", QCoreApplication::translate("main", "Print the packets sent to the keyboard."));
    parser.addOption(debug_option);
    parser.process(app);
    if (parser.isSet(debug_option))
        QLoggingCategory::setFilterRules("drevo.packets.debug=true");

    // only one instance can use the keyboard: bring the running one to the front instead
    SingleInstance single_instance;
    if (single_instance.activateRunningInstance())
    {
        QTextStream(stderr) << QCoreApplication::translate("main",
                                   "DrevoPowerConsole is already running, switching to it.") << Qt::endl;
        return 0;
    }
    single_instance.listen();
    app.setWindowIcon(QIcon(":/image/drevo-power-console.png"));
    // Wayland shows the icon of the matching drevo-power-console.desktop
    app.setDesktopFileName("drevo-power-console");

    const char *fonts[] = {
        ":/font/OpenSans-Regular.ttf",
        ":/font/OpenSans-Semibold.ttf",
        ":/font/OpenSans-ExtraBold.ttf",
        ":/font/OpenSans-Light.ttf",
    };
    for (const char *font : fonts)
        QFontDatabase::addApplicationFont(font);
    app.setFont(QFont("Open Sans"));

    // Material with the compact desktop sizes; the colors are set in Theme.qml
    QQuickStyle::setStyle("Material");
    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_MATERIAL_VARIANT"))
        qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");

    // keyboard connection and database, used by the QML pages
    DeviceManager device_manager(nullptr);
    // tray icon, used by the QML window
    SystemTray system_tray(nullptr);

    QQmlApplicationEngine engine;
    engine.addImageProvider("icon", new IconProvider);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("DrevoPowerConsole", "Main");

    QObject::connect(&single_instance, &SingleInstance::activationRequested, &app, [&engine](const QString &token) {
        QWindow *window = qobject_cast<QWindow*>(engine.rootObjects().value(0));
        if (!window)
            return;
        // on Wayland, the token of the new instance allows taking the focus
        if (!token.isEmpty())
            qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
        if (window->windowStates() & Qt::WindowMinimized)
            window->showNormal();
        else
            window->show();
        window->raise();
        window->requestActivate();
    });

    return app.exec();
}
