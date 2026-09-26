#ifndef DEVICEDB_H
#define DEVICEDB_H

#include <QSettings>
#include <QString>
#include <QVector>
#include "KeyboardData.h"

// KEY DEFINE
enum KEY_DEFINE
{
    KEY_REALVALUE = 0,
    KEY_DISABLE,
    KEY_REDEFINEVALUE,
    KEY_MACRO,
    KEY_MOUSE,
    KEY_MULTIMEDIA,
    KEY_LINUX,
};
enum KEY_MOUSE_DEFINE
{
    KEY_MOUSE_LEFT = 1,
    KEY_MOUSE_MIDDLE,
    KEY_MOUSE_RIGHT,
    KEY_MOUSE_DBLEFT,
    KEY_MOUSE_SCROLLUP,
    KEY_MOUSE_SCROLLDOWN,
    KEY_MOUSE_BUTTON4,
    KEY_MOUSE_BUTTON5,
};
enum KEY_MULTIMEDIA_DEFINE
{
    KEY_MEDIA_PLAYPAUSE = 1,
    KEY_MEDIA_STOP,
    KEY_MEDIA_PREVIOUS,
    KEY_MEDIA_NEXT,
    KEY_MEDIA_VOL_UP,
    KEY_MEDIA_VOL_DOWN,
    KEY_MEDIA_VOL_SILENT,
};
enum KEY_LINUX_DEFINE
{
    KEY_LINUX_TERMINAL = 1,
    KEY_LINUX_COPY,
    KEY_LINUX_PASTE,
    KEY_LINUX_CUT,
};

// keyboard settings, stored in the application's settings file
// (~/.config/DrevoPowerConsole/DrevoPowerConsole.conf), section [profile<n>]:
//
//   name                               profile name
//   report_rate, delay_*               config values
//   light_mode                         selected light mode
//   light\<mode>\...                   light mode settings (see RadiData)
//   light\<mode>\keys\<key value>      custom key color (#rrggbb)
//   keys\<key value>\...               key assignment (see KeyData)
class DeviceDB
{
public:
    DeviceDB();
    ~DeviceDB();

public:
    // why settings cannot be stored, empty if they can
    QString databaseError() const;

    // profile ids, in creation order
    QVector<int> profiles() const;
    QString profileName(int profile) const;
    bool setProfileName(int profile, const QString &name);
    // new profile with default settings, returns its id
    int addProfile(const QString &name);
    // new profile with the settings of another one, returns its id
    int duplicateProfile(int source, const QString &name);
    bool removeProfile(int profile);
    // profile selected when the program was closed
    int selectedProfile() const;
    bool setSelectedProfile(int profile);

    // read config data
    bool readConfigData(int profile, QString key, QString& value);
    // update config data
    bool updateConfigData(int profile, QString key, QString value);

    // write radi data(if profile not exisit)
    void writeRadiData(int profile);
    // modify rgb param
    bool modifyRadiRGBData(RadiData &data);
    // set select mode with current profile
    bool setSelectMode(int profile, int mode);
    // get  select mode with current profile
    int getSelectMode(int profile);
    // query  light data info with profile and mode
    bool queryRadiInfo(int profile, int mode, RadiData &data);

    //  add key rgb color
    bool addKeyRGBData(RGBData data);
    // query key rgb color
    bool queryKeysRGBData(int radi_id, QVector<RGBData*> &vecdata);
    // delete key rgb with radi-mode
    bool deleteKeyRGBData(int radi_id);

    // add key data
    bool addKeyProfile(KeyData &data);
    // query key data ( keyonly == true,  only return keyvalue)
    int queryKeyProfileInfo(int profile, QVector<KeyData*> &vecdata, bool keyvalue_only = true);
    // query key data  (key info)
    bool queryKeyProfileInfo(int profile, int key_value, KeyData &data);

private:
    // write default config data
    void initConfigData(int profile);
    // save and check the result
    bool store();

    QSettings           m_settings;
};

#endif // DEVICEDB_H
