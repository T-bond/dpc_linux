#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "DeviceDB.h"

#include <drevo/Keyboard.h>

class QTimer;

class QQmlEngine;
class QJSEngine;

// owns the keyboard connection and the settings database (one instance, created in main)
class DeviceManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString deviceName READ deviceName NOTIFY connectionChanged)
    // a keyboard was found and opened
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
    // only a 2.4G receiver is plugged in: the keyboard cannot be programmed through it
    Q_PROPERTY(bool receiverOnly READ receiverOnly NOTIFY connectionChanged)
    // settings profiles: [{ id, name, hardwareProfile }]
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY profilesChanged)
    // Each profile belongs to one hardware profile of the keyboard (G1..G3 = 0..2). The application
    // shows one hardware profile; its current profile is the one last written to it, -1 if none.
    // Selecting a profile writes its key assignments to its hardware profile (and shows that one).
    Q_PROPERTY(int currentProfile READ currentProfile WRITE setCurrentProfile NOTIFY currentProfileChanged)
    // hardware profile shown in the application, 0..2; selecting one writes nothing
    Q_PROPERTY(int hardwareProfile READ hardwareProfile WRITE setHardwareProfile NOTIFY hardwareProfileChanged)
    // hardware profiles: [{ name, profile, known, storedName }], profile: its current profile or -1,
    // known: the application knows the key assignments the keyboard stores in it,
    // storedName: the profile they come from ("" if not known, or the factory settings)
    Q_PROPERTY(QVariantList hardwareProfiles READ hardwareProfiles NOTIFY hardwareProfilesChanged)
    // profiles of the shown hardware profile: [{ id, name }]
    Q_PROPERTY(QVariantList hardwareProfileProfiles READ hardwareProfileProfiles NOTIFY profilesChanged)
    // why settings cannot be stored, empty if they can
    Q_PROPERTY(QString databaseError READ databaseError CONSTANT)

    // regional variants of the layout: [{ value, text }], empty if it has none
    Q_PROPERTY(QVariantList keyboardRegions READ keyboardRegions NOTIFY connectionChanged)
    // selected regional variant, the keyboard does not report it
    Q_PROPERTY(QString keyboardRegion READ keyboardRegion WRITE setKeyboardRegion NOTIFY keyboardRegionChanged)
    // keyboard image for the layout and region
    Q_PROPERTY(QUrl keyboardImage READ keyboardImage NOTIFY keyboardRegionChanged)

    // closing the window hides it to the system tray
    Q_PROPERTY(bool closeToTray READ closeToTray WRITE setCloseToTray NOTIFY closeToTrayChanged)
    // keyboard lights switched off, without changing the profile
    Q_PROPERTY(bool lightsOff READ lightsOff WRITE setLightsOff NOTIFY lightsOffChanged)

public:
    // not default constructible, so QML uses create() and gets this instance
    explicit DeviceManager(QObject *parent);
    ~DeviceManager();

    static DeviceManager* instance() { return s_instance; }
    static DeviceManager* create(QQmlEngine *qml_engine, QJSEngine *js_engine);

    QString deviceName() const { return m_device_name; }
    bool connected() const { return m_keyboard.isConnected(); }
    bool receiverOnly() const { return m_keyboard.state() == drevo::ConnectionState::ReceiverOnly; }
    QVariantList profiles() const;
    int currentProfile() const { return m_current_profile; }
    void setCurrentProfile(int profile);
    int hardwareProfile() const { return m_hardware_profile; }
    void setHardwareProfile(int hardware_profile);
    QVariantList hardwareProfiles() const;
    QVariantList hardwareProfileProfiles() const;

    // look for the keyboard again and reopen it; nothing is written to it
    Q_INVOKABLE void refreshConnection();

    // new profile with default settings, selected; returns its id
    Q_INVOKABLE int addProfile(const QString &name);
    // new profile with the settings of another one, selected; returns its id (-1 if unknown)
    Q_INVOKABLE int duplicateProfile(int profile, const QString &name);
    Q_INVOKABLE void renameProfile(int profile, const QString &name);
    // removes a profile (not the last one); if its hardware profile shows it, that one shows the
    // previous profile of the hardware profile (written to it), or none
    Q_INVOKABLE void removeProfile(int profile);
    // moves a profile to another hardware profile and writes it there; the former hardware profile
    // keeps its key assignments on the keyboard, but shows no profile
    Q_INVOKABLE void moveProfile(int profile, int hardware_profile);

    // a key assignment of the current profile changed: write it to its hardware profile
    void writeKey(int key_value);
    // restore the factory settings; the keyboard then has the default key assignments in all
    // hardware profiles, which show no profile
    void resetKeyboard();
    QString databaseError() const { return m_database_error; }

    // key layout of the connected keyboard
    drevo::Layout keyboardLayout() const { return m_keyboard.layout(); }
    QVariantList keyboardRegions() const;
    QString keyboardRegion() const { return m_keyboard_region; }
    void setKeyboardRegion(const QString &region);
    QUrl keyboardImage() const;

    bool closeToTray() const { return m_close_to_tray; }
    void setCloseToTray(bool close_to_tray);

    bool lightsOff() const { return m_lights_off; }
    // on: switch the lights off; off: send the lighting of the profile again
    void setLightsOff(bool lights_off);
    // lighting data was sent to the keyboard, the lights are on
    void lightingSent();

    drevo::Keyboard* keyboard() { return &m_keyboard; }
    DeviceDB* db() { return &m_dev_db; }

signals:
    // deviceName, connected or the layout changed
    void connectionChanged();
    void keyboardRegionChanged();
    void profilesChanged();
    void hardwareProfileChanged();
    void hardwareProfilesChanged();
    void closeToTrayChanged();
    void lightsOffChanged();
    // the lighting of the current profile should be sent again
    void lightingRestoreRequested();
    // emitted after the key assignments of the profile were written; pages reload and send the rest
    void currentProfileChanged();

private:
    static DeviceManager*       s_instance;

    drevo::Keyboard             m_keyboard;
    DeviceDB                    m_dev_db;
    QString                     m_device_name;
    QString                     m_database_error;
    QString                     m_keyboard_region;
    bool                        m_close_to_tray;
    bool                        m_lights_off;
    int                         m_hardware_profile;
    int                         m_current_profile;
    // refreshes the connection a little after a keyboard was plugged in or removed
    QTimer                     *m_refresh_timer;

    // profiles of a hardware profile, in creation order
    QVector<int> profilesOf(int hardware_profile) const;
    // write the key assignments of a profile to a hardware profile: the keys that differ from what
    // it stores, or all keys if that is not known; the profile is then its current profile
    void writeProfile(int profile, int hardware_profile);
    // write one key assignment (nullptr: the default function), false if it was not sent
    bool sendKey(int key_value, const KeyData *data, int hardware_profile);
    // show a hardware profile and its current profile
    void showHardwareProfile(int hardware_profile);
    // title for the result of opening the keyboard
    QString connectionText(drevo::ConnectionState state) const;
};

#endif // DEVICEMANAGER_H
