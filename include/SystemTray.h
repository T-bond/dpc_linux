#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

class QAction;
class QActionGroup;
class QJSEngine;
class QMenu;
class QQmlEngine;
class QSystemTrayIcon;

// tray icon, shown while "close to the system tray" is on: click shows the window, the menu selects
// the profile, switches the lights off or exits (one instance, created in main)
class SystemTray : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // the desktop shows tray icons
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    // not default constructible, so QML uses create() and gets this instance
    explicit SystemTray(QObject *parent);
    ~SystemTray();

    static SystemTray* create(QQmlEngine *qml_engine, QJSEngine *js_engine);

    bool available() const;

signals:
    void showWindowRequested();
    void quitRequested();

private:
    void updateVisible();
    void updateProfiles();

    static SystemTray*  s_instance;

    QSystemTrayIcon    *m_tray;
    QMenu              *m_menu;
    QMenu              *m_profile_menu;
    QActionGroup       *m_profile_group;
    QAction            *m_lights_off;
};

#endif // SYSTEMTRAY_H
