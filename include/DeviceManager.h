#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "DeviceComm.h"
#include "DeviceDB.h"

class QQmlEngine;
class QJSEngine;

// owns the keyboard connection and the settings database (one instance, created in main)
class DeviceManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString deviceName READ deviceName CONSTANT)
    // settings profiles: [{ id, name }]
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY profilesChanged)
    // selecting a profile writes its settings to the keyboard
    Q_PROPERTY(int currentProfile READ currentProfile WRITE setCurrentProfile NOTIFY currentProfileChanged)
    // why settings cannot be stored, empty if they can
    Q_PROPERTY(QString databaseError READ databaseError CONSTANT)

    // key layout of the connected keyboard (1: 87 keys, 2: 88 keys ISO, 4: 91 keys JIS)
    Q_PROPERTY(int keyboardLayout READ keyboardLayout CONSTANT)
    // regional variants of the layout: [{ value, text }], empty if it has none
    Q_PROPERTY(QVariantList keyboardRegions READ keyboardRegions CONSTANT)
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
    QVariantList profiles() const;
    int currentProfile() const { return m_current_profile; }
    void setCurrentProfile(int profile);

    // new profile with default settings, selected; returns its id
    Q_INVOKABLE int addProfile(const QString &name);
    // new profile with the settings of another one, selected; returns its id (-1 if unknown)
    Q_INVOKABLE int duplicateProfile(int profile, const QString &name);
    Q_INVOKABLE void renameProfile(int profile, const QString &name);
    // removes a profile (not the last one), selecting another one if needed
    Q_INVOKABLE void removeProfile(int profile);

    // write a key assignment of the current profile to the keyboard
    void writeKeyData(const KeyData &data);
    // write the default function of a key to the keyboard
    void writeKeyDefault(int key_value);
    QString databaseError() const { return m_database_error; }

    int keyboardLayout() const { return m_device_comm.getKeyboardLayout(); }
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

    DeviceComm* comm() { return &m_device_comm; }
    DeviceDB* db() { return &m_dev_db; }

signals:
    void keyboardRegionChanged();
    void profilesChanged();
    void closeToTrayChanged();
    void lightsOffChanged();
    // the lighting of the current profile should be sent again
    void lightingRestoreRequested();
    // emitted after the key assignments of the profile were written; pages reload and send the rest
    void currentProfileChanged();

private:
    static DeviceManager*       s_instance;

    DeviceComm                  m_device_comm;
    DeviceDB                    m_dev_db;
    QString                     m_device_name;
    QString                     m_database_error;
    QString                     m_keyboard_region;
    bool                        m_close_to_tray;
    bool                        m_lights_off;
    int                         m_current_profile;

    // key values with an assignment in a profile
    QVector<int> assignedKeys(int profile);
};

#endif // DEVICEMANAGER_H
