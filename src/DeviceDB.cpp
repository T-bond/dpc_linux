#include "DeviceDB.h"

#include "LightModes.h"

#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <algorithm>
#include <cstring>

namespace
{

// radi_id of a light mode, the key colors are stored with it
int radiId(int profile, int mode)
{
    return profile * 100 + mode;
}

QString profileGroup(int profile)
{
    return QString("profile%1").arg(profile);
}

const char *kProfilePrefix = "profile";
const char *kSelectedProfile = "selected_profile";

// settings of a light mode, empty for an unknown mode
QString lightGroup(int profile, int mode)
{
    const LightModeInfo *info = lightModeInfo(mode);
    if (!info)
        return QString();
    return QString("profile%1/light/%2").arg(profile).arg(info->key);
}

QString keyColorsGroup(int radi_id)
{
    const QString group = lightGroup(radi_id / 100, radi_id % 100);
    return group.isEmpty() ? QString() : group + "/keys";
}

QString sideColorsGroup(int radi_id)
{
    const QString group = lightGroup(radi_id / 100, radi_id % 100);
    return group.isEmpty() ? QString() : group + "/side";
}

// setting of a key or side LED color, empty for an unknown light mode
QString colorSetting(int radi_id, int key_value)
{
    if (std::optional<drevo::SideLed> led = sideLedFromValue(key_value))
    {
        const QString group = sideColorsGroup(radi_id);
        return group.isEmpty() ? QString()
                               : QString("%1/%2/%3").arg(group, lightBarKey(led->bar())).arg(led->index());
    }
    const QString group = keyColorsGroup(radi_id);
    return group.isEmpty() ? QString() : group + "/" + QString::number(key_value);
}

QString keyGroup(int profile, int key_value)
{
    return QString("profile%1/keys/%2").arg(profile).arg(key_value);
}

// light mode settings that are not stored have these values
const bool kDefaultCustomColor = false;
const int kDefaultSpeed = 3;
const int kDefaultBrightness = 10;
const int kDefaultDirection = 0;
const QColor kDefaultColor(255, 225, 0);

} // namespace

DeviceDB::DeviceDB()
{
    // first start: a default profile
    if (profiles().isEmpty())
    {
        initConfigData(1);
        writeRadiData(1);
    }
}

DeviceDB::~DeviceDB()
{
    m_settings.sync();
}

// why settings cannot be stored, empty if they can
QString DeviceDB::databaseError() const
{
    QString file_name = m_settings.fileName();
    QFileInfo file_info(file_name);
    QFileInfo dir_info(file_info.absolutePath());

    // QSettings writes a new file next to the old one and renames it
    bool writable = file_info.exists() ? file_info.isWritable() && dir_info.isWritable()
                                       : dir_info.isWritable() || !dir_info.exists();
    if (!writable || m_settings.status() != QSettings::NoError)
    {
        return QCoreApplication::translate("DeviceDB",
                   "The settings file %1 is not writable, so changes cannot be saved or sent to the "
                   "keyboard. It was probably created by running the program as root; make it writable, "
                   "e.g. with: sudo chown -R $USER \"%2\"").arg(file_name, file_info.absolutePath());
    }
    return QString();
}

// profile ids, in creation order
QVector<int> DeviceDB::profiles() const
{
    QVector<int> ids;
    const QStringList groups = m_settings.childGroups();
    for (const QString &group : groups)
    {
        bool ok = false;
        int id = group.startsWith(kProfilePrefix) ? group.mid(int(strlen(kProfilePrefix))).toInt(&ok) : 0;
        if (ok && id > 0)
            ids.append(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

QString DeviceDB::profileName(int profile) const
{
    QString name = m_settings.value(profileGroup(profile) + "/name").toString();
    if (name.isEmpty())
        name = profile == 1 ? QCoreApplication::translate("DeviceDB", "Default")
                            : QCoreApplication::translate("DeviceDB", "Profile %1").arg(profile);
    return name;
}

bool DeviceDB::setProfileName(int profile, const QString &name)
{
    m_settings.setValue(profileGroup(profile) + "/name", name);
    return store();
}

// new profile with default settings, returns its id
int DeviceDB::addProfile(const QString &name)
{
    QVector<int> ids = profiles();
    int profile = ids.isEmpty() ? 1 : ids.last() + 1;

    initConfigData(profile);
    writeRadiData(profile);
    setProfileName(profile, name);
    return profile;
}

// new profile with the settings of another one, returns its id
int DeviceDB::duplicateProfile(int source, const QString &name)
{
    QVector<int> ids = profiles();
    int profile = ids.isEmpty() ? 1 : ids.last() + 1;

    m_settings.beginGroup(profileGroup(source));
    const QStringList keys = m_settings.allKeys();
    QVariantList values;
    for (const QString &key : keys)
        values.append(m_settings.value(key));
    m_settings.endGroup();

    m_settings.beginGroup(profileGroup(profile));
    for (int i = 0; i < keys.size(); i++)
        m_settings.setValue(keys.at(i), values.at(i));
    m_settings.endGroup();

    setProfileName(profile, name);
    return profile;
}

bool DeviceDB::removeProfile(int profile)
{
    m_settings.remove(profileGroup(profile));
    return store();
}

// profile selected when the program was closed
int DeviceDB::selectedProfile() const
{
    return m_settings.value(kSelectedProfile, 1).toInt();
}

bool DeviceDB::setSelectedProfile(int profile)
{
    m_settings.setValue(kSelectedProfile, profile);
    return store();
}

// read config data
bool DeviceDB::readConfigData(int profile, QString key, QString& value)
{
    QString setting = profileGroup(profile) + "/" + key;
    if (m_settings.contains(setting))
        value = m_settings.value(setting).toString();
    return true;
}

// update config data
bool DeviceDB::updateConfigData(int profile, QString key, QString value)
{
    m_settings.setValue(profileGroup(profile) + "/" + key, value);
    return store();
}

// write radi data (select the first light mode if none is selected)
void DeviceDB::writeRadiData(int profile)
{
    QString setting = profileGroup(profile) + "/light_mode";
    if (m_settings.contains(setting))
        return;
    m_settings.setValue(setting, kLightModes[0].key);
    store();
}

// modify rgb param (only the settings the mode has)
bool DeviceDB::modifyRadiRGBData(RadiData &data)
{
    const LightModeInfo *info = lightModeInfo(data.mode);
    if (!info)
        return false;

    m_settings.beginGroup(lightGroup(data.profile_id, data.mode));
    m_settings.setValue("brightness", data.rgb_brightness);
    if (info->has(LightSpeed))
        m_settings.setValue("speed", data.rgb_speed);
    if (info->has(LightDirection))
        m_settings.setValue("direction", data.rgb_direction);
    if (info->has(LightColorSwitch))
        m_settings.setValue("custom_color", data.rgb_mode == 1);
    if (info->has(LightColor))
        m_settings.setValue("color", QColor(data.r_value, data.g_value, data.b_value).name());
    m_settings.endGroup();
    return store();
}

// set select mode with current profile
bool DeviceDB::setSelectMode(int profile, int mode)
{
    // an unknown mode leaves no mode selected
    const LightModeInfo *info = lightModeInfo(mode);
    m_settings.setValue(profileGroup(profile) + "/light_mode", info ? info->key : "");
    return store();
}

// get  select mode with current profile (0 if none)
int DeviceDB::getSelectMode(int profile)
{
    const LightModeInfo *info = lightModeInfo(m_settings.value(profileGroup(profile) + "/light_mode").toString());
    return info ? int(info->mode) : 0;
}

// query  light data info with profile and mode
bool DeviceDB::queryRadiInfo(int profile, int mode, RadiData &data)
{
    QString group = lightGroup(profile, mode);
    if (group.isEmpty())
        return false;

    m_settings.beginGroup(group);
    QColor color(m_settings.value("color", kDefaultColor.name()).toString());
    data.radi_id        = radiId(profile, mode);
    data.profile_id     = profile;
    data.mode           = mode;
    data.rgb_mode       = m_settings.value("custom_color", kDefaultCustomColor).toBool() ? 1 : 0;
    data.rgb_speed      = m_settings.value("speed", kDefaultSpeed).toInt();
    data.rgb_brightness = m_settings.value("brightness", kDefaultBrightness).toInt();
    data.rgb_direction  = m_settings.value("direction", kDefaultDirection).toInt();
    data.r_value        = color.red();
    data.g_value        = color.green();
    data.b_value        = color.blue();
    data.status         = getSelectMode(profile) == mode ? 1 : 0;
    m_settings.endGroup();
    return true;
}

//  add key rgb color (black removes the color)
bool DeviceDB::addKeyRGBData(RGBData data)
{
    QString setting = colorSetting(data.radi_id, data.key_value);
    if (setting.isEmpty())
        return false;
    if (data.r_value == 0 && data.g_value == 0 && data.b_value == 0)
        m_settings.remove(setting);
    else
        m_settings.setValue(setting, QColor(data.r_value, data.g_value, data.b_value).name());
    return store();
}

// query key rgb color (keys and side LEDs)
bool DeviceDB::queryKeysRGBData(int radi_id, QVector<RGBData*> &vecdata)
{
    QString group = keyColorsGroup(radi_id);
    if (group.isEmpty())
        return false;

    auto append = [&](int key_value, const QString &setting) {
        QColor color(m_settings.value(setting).toString());

        RGBData *data = new RGBData;
        data->rgb_id    = 0;
        data->radi_id   = radi_id;
        data->key_value = key_value;
        data->r_value   = color.red();
        data->g_value   = color.green();
        data->b_value   = color.blue();
        vecdata.push_back(data);
    };

    m_settings.beginGroup(group);
    const QStringList keys = m_settings.childKeys();
    m_settings.endGroup();
    for (const QString &key : keys)
        append(key.toInt(), group + "/" + key);

    const QString side_group = sideColorsGroup(radi_id);
    m_settings.beginGroup(side_group);
    const QStringList bars = m_settings.childGroups();
    m_settings.endGroup();
    for (const QString &bar_key : bars)
    {
        std::optional<drevo::LightBar> bar = lightBarFromKey(bar_key);
        if (!bar)
            continue;

        const QString bar_group = side_group + "/" + bar_key;
        m_settings.beginGroup(bar_group);
        const QStringList indexes = m_settings.childKeys();
        m_settings.endGroup();
        for (const QString &index : indexes)
        {
            if (std::optional<drevo::SideLed> led = drevo::SideLed::make(*bar, index.toInt()))
                append(sideLedValue(*led), bar_group + "/" + index);
        }
    }
    return true;
}

// delete key rgb with radi-mode (keys and side LEDs)
bool DeviceDB::deleteKeyRGBData(int radi_id)
{
    QString group = keyColorsGroup(radi_id);
    if (group.isEmpty())
        return false;
    m_settings.remove(group);
    m_settings.remove(sideColorsGroup(radi_id));
    return store();
}

// add key data (macro value 0 only removes the assignment)
bool DeviceDB::addKeyProfile(KeyData &data)
{
    if (data.key_value <= 0)
        return false ;

    QString group = keyGroup(data.profile, data.key_value);
    m_settings.remove(group);

    if (data.macro_value != 0)
    {
        m_settings.beginGroup(group);
        m_settings.setValue("macro_type", data.macro_type);
        m_settings.setValue("macro_value", data.macro_value);
        m_settings.setValue("macro_value1", data.macro_value1);
        m_settings.setValue("macro_value2", data.macro_value2);
        m_settings.setValue("macro_name", data.macro_name);
        m_settings.endGroup();
    }
    return store();
}

// query key data
int DeviceDB::queryKeyProfileInfo(int profile, QVector<KeyData*> &vecdata, bool keyvalue_only)
{
    m_settings.beginGroup(profileGroup(profile) + "/keys");
    const QStringList keys = m_settings.childGroups();
    m_settings.endGroup();

    for (const QString &key : keys)
    {
        KeyData *data = new KeyData();
        data->key_value = key.toInt();
        if (!keyvalue_only)
            queryKeyProfileInfo(profile, data->key_value, *data);
        vecdata.push_back(data);
    }
    return vecdata.size();
}

// query key data  (key info)
bool DeviceDB::queryKeyProfileInfo(int profile, int key_value, KeyData &data)
{
    QString group = keyGroup(profile, key_value);
    if (!m_settings.contains(group + "/macro_type"))
        return false;

    m_settings.beginGroup(group);
    data.key_id       = 0;
    data.profile      = profile;
    data.key_value    = key_value;
    data.macro_type   = m_settings.value("macro_type").toInt();
    data.macro_value  = m_settings.value("macro_value").toInt();
    data.macro_value1 = m_settings.value("macro_value1").toInt();
    data.macro_value2 = m_settings.value("macro_value2").toInt();
    data.macro_name   = m_settings.value("macro_name").toString();
    m_settings.endGroup();
    return true;
}

// write default config data
void DeviceDB::initConfigData(int profile)
{
    m_settings.beginGroup(profileGroup(profile));
    m_settings.setValue("report_rate", "2");
    m_settings.setValue("delay_usbwiresleep", "300");
    m_settings.setValue("delay_closelight", "120");
    m_settings.setValue("delay_wirelesssleep", "180");
    m_settings.endGroup();
    store();
}

// save and check the result
bool DeviceDB::store()
{
    m_settings.sync();
    return m_settings.status() == QSettings::NoError;
}
