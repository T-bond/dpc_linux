#ifndef DEVICEDB_H
#define DEVICEDB_H

#include <QHash>
#include <QSettings>
#include <QString>
#include <QVector>

// radi light data
struct RadiData
{
    int             radi_id;
    int             profile_id;
    int             mode;
    int             rgb_mode;
    int             rgb_speed;
    int             rgb_brightness;
    int             rgb_direction;
    int             r_value;
    int             g_value;
    int             b_value;
    int             status;
};

// key rgb data
struct RGBData
{
    int             rgb_id;
    int             radi_id;
    int             key_value;
    int             r_value;
    int             g_value;
    int             b_value;
};

// key data
struct KeyData
{
    int             key_id;
    int             profile;
    int             key_value;
    int             macro_type;
    int             macro_value;
    int             macro_value1;
    int             macro_value2;
    QString     macro_name;
};

// key assignments by key value; keys with their default function are not listed
using KeyMap = QHash<int, KeyData>;

// hardware profiles of the keyboard (G1..G3, Fn+Ctrl+F1..F3), numbered 0..2
inline constexpr int kHardwareProfileCount = 3;

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
//   light_mode                         selected light mode (name, see LightModes.h)
//   light\<mode>\brightness            light mode settings, only the ones the mode has
//   light\<mode>\speed                 (see LightModes.h); missing ones have their default
//   light\<mode>\direction
//   light\<mode>\custom_color          true: use color, false: the keyboard's own colors
//   light\<mode>\color                 #rrggbb
//   light\custom\keys\<key value>      custom key color (#rrggbb)
//   light\custom\side\<bar>\<index>    custom side LED color, bar: left, top, right, bottom;
//                                      index left to right / top to bottom
//   keys\<key value>\...               key assignment (see KeyData)
//   hardware_profile                   hardware profile the profile belongs to, 1..3 (G1..G3)
//
// section [hardware], the hardware profiles (n = 1..3):
//   selected                           hardware profile shown in the application
//   g<n>\profile                       profile last written to it, missing if none (it was moved
//                                      to another hardware profile or deleted, or the keyboard was reset)
//   g<n>\name                          name of the profile last written to it
//   g<n>\known                         the keys below are what the keyboard stores; false if they
//                                      are not known (never written, or a transfer failed)
//   g<n>\keys\<key value>\...          key assignments last written to it (see KeyData)
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
    // key assignments of a profile
    KeyMap profileKeys(int profile) const;

    // hardware profile (0..2) a profile belongs to
    int profileHardwareProfile(int profile) const;
    bool setProfileHardwareProfile(int profile, int hardware_profile);
    // hardware profile shown in the application
    int selectedHardwareProfile() const;
    bool setSelectedHardwareProfile(int hardware_profile);
    // profile last written to a hardware profile, -1 if none
    int hardwareProfileProfile(int hardware_profile) const;
    bool setHardwareProfileProfile(int hardware_profile, int profile);
    // what the keyboard stores in a hardware profile: the key assignments last written, whether
    // they are known, and the name of the profile they came from
    KeyMap hardwareProfileKeys(int hardware_profile) const;
    bool hardwareProfileKnown(int hardware_profile) const;
    QString hardwareProfileName(int hardware_profile) const;
    bool setHardwareProfileContent(int hardware_profile, const KeyMap &keys, bool known, const QString &name);
    // set one key of what the keyboard stores (default = remove it)
    bool setHardwareProfileKey(int hardware_profile, int key_value, const KeyData *data, bool known);

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
    // hardware profile settings of a configuration from before hardware profiles
    void migrateHardwareProfiles();
    // key assignment stored in a settings group, false if there is none
    bool readKey(const QString &group, KeyData &data) const;
    void writeKey(const QString &group, const KeyData &data);
    // save and check the result
    bool store();

    // mutable: reading a group needs beginGroup()
    mutable QSettings   m_settings;
};

#endif // DEVICEDB_H
