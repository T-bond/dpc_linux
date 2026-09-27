#include "DeviceManager.h"

#include <QDebug>
#include <QJSEngine>
#include <QSettings>

DeviceManager* DeviceManager::s_instance = nullptr;

namespace
{

struct KeyboardRegion
{
    const char     *value;      // suffix of image/keyboard/img_keyboard_88<value>.png
    const char     *text;
};

// regional variants of the 88 key (ISO) layout
const KeyboardRegion kRegions88[] = {
    { "uk",  QT_TRANSLATE_NOOP("DeviceManager", "English (UK)") },
    { "de",  QT_TRANSLATE_NOOP("DeviceManager", "German") },
    { "fr",  QT_TRANSLATE_NOOP("DeviceManager", "French") },
    { "es",  QT_TRANSLATE_NOOP("DeviceManager", "Spanish") },
    { "ita", QT_TRANSLATE_NOOP("DeviceManager", "Italian") },
    { "nd",  QT_TRANSLATE_NOOP("DeviceManager", "Nordic") },
    { "kr",  QT_TRANSLATE_NOOP("DeviceManager", "Korean") },
};

// stored key assignment as a keyboard action, empty if it is invalid
std::optional<drevo::KeyAction> toKeyAction(const KeyData &data)
{
    auto combo = [&data]() -> std::optional<drevo::KeyAction> {
        std::optional<drevo::Usage> target = drevo::Usage::fromValue(data.macro_value);
        std::optional<drevo::Modifier> first = drevo::enumFromValue<drevo::Modifier>(data.macro_value1);
        std::optional<drevo::Modifier> second = drevo::enumFromValue<drevo::Modifier>(data.macro_value2);
        if (!target || !first || !second)
            return std::nullopt;
        return drevo::ComboAction { *target, *first, *second };
    };

    switch (data.macro_type)
    {
    case KEY_REALVALUE:
        return drevo::DefaultAction {};
    case KEY_DISABLE:
        return drevo::DisableAction {};
    case KEY_REDEFINEVALUE:
    case KEY_LINUX:
        return combo();
    case KEY_MOUSE:
        if (std::optional<drevo::MouseAction> mouse = drevo::enumFromValue<drevo::MouseAction>(data.macro_value))
            return *mouse;
        return std::nullopt;
    case KEY_MULTIMEDIA:
        if (std::optional<drevo::MediaAction> media = drevo::enumFromValue<drevo::MediaAction>(data.macro_value))
            return *media;
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

const char *kRegionSetting = "keyboard/region";
const char *kCloseToTraySetting = "window/close_to_tray";

} // namespace

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    // find drevo device
    m_device_name = connectionText(m_keyboard.open());

    QVector<int> ids = m_dev_db.profiles();
    m_current_profile = m_dev_db.selectedProfile();
    if (!ids.contains(m_current_profile))
        m_current_profile = ids.isEmpty() ? 1 : ids.first();
    m_dev_db.writeRadiData(m_current_profile);

    m_database_error = m_dev_db.databaseError();
    if (!m_database_error.isEmpty())
        qWarning().noquote() << m_database_error;

    m_keyboard_region = QSettings().value(kRegionSetting, kRegions88[0].value).toString();
    m_close_to_tray = QSettings().value(kCloseToTraySetting, false).toBool();
    m_lights_off = false;
}

DeviceManager::~DeviceManager()
{
    s_instance = nullptr;
}

QVariantList DeviceManager::keyboardRegions() const
{
    QVariantList regions;
    if (keyboardLayout() != drevo::Layout::Iso88)
        return regions;

    for (const KeyboardRegion &region : kRegions88)
        regions.append(QVariantMap { { "value", QString(region.value) }, { "text", tr(region.text) } });
    return regions;
}

void DeviceManager::setKeyboardRegion(const QString &region)
{
    if (m_keyboard_region == region)
        return;

    m_keyboard_region = region;
    QSettings().setValue(kRegionSetting, region);
    emit keyboardRegionChanged();
}

void DeviceManager::setCloseToTray(bool close_to_tray)
{
    if (m_close_to_tray == close_to_tray)
        return;
    m_close_to_tray = close_to_tray;
    QSettings().setValue(kCloseToTraySetting, close_to_tray);
    emit closeToTrayChanged();
}

// on: switch the lights off; off: send the lighting of the profile again
void DeviceManager::setLightsOff(bool lights_off)
{
    if (m_lights_off == lights_off)
        return;

    if (lights_off)
    {
        // static mode, custom color black, brightness 0 (not stored in the profile)
        m_keyboard.setLighting(drevo::StaticEffect::off());

        m_lights_off = true;
        emit lightsOffChanged();
    }
    else
    {
        // the lighting page sends the profile's lighting, which calls lightingSent()
        emit lightingRestoreRequested();
        lightingSent();
    }
}

// lighting data was sent to the keyboard, the lights are on
void DeviceManager::lightingSent()
{
    if (!m_lights_off)
        return;
    m_lights_off = false;
    emit lightsOffChanged();
}

QUrl DeviceManager::keyboardImage() const
{
    switch (keyboardLayout())
    {
    case drevo::Layout::Iso88:
        for (const KeyboardRegion &region : kRegions88)
        {
            if (m_keyboard_region == region.value)
                return QUrl(QString("qrc:/image/keyboard/img_keyboard_88%1.png").arg(region.value));
        }
        return QUrl(QString("qrc:/image/keyboard/img_keyboard_88%1.png").arg(kRegions88[0].value));
    case drevo::Layout::Jis91:
        return QUrl("qrc:/image/keyboard/img_keyboard_91.png");
    case drevo::Layout::Tkl87:
        break;
    }
    return QUrl("qrc:/image/keyboard/img_keyboard_87.png");
}

QVariantList DeviceManager::profiles() const
{
    QVariantList list;
    for (int id : m_dev_db.profiles())
        list.append(QVariantMap { { "id", id }, { "name", m_dev_db.profileName(id) } });
    return list;
}

void DeviceManager::setCurrentProfile(int profile)
{
    if (profile == m_current_profile || !m_dev_db.profiles().contains(profile))
        return;

    // keys of the old profile get their default function back
    const QVector<int> old_keys = assignedKeys(m_current_profile);
    const QVector<int> new_keys = assignedKeys(profile);
    for (int key_value : old_keys)
    {
        if (!new_keys.contains(key_value))
            writeKeyDefault(key_value);
    }

    m_current_profile = profile;
    m_dev_db.setSelectedProfile(profile);

    for (int key_value : new_keys)
    {
        KeyData data = {};
        if (m_dev_db.queryKeyProfileInfo(profile, key_value, data))
            writeKeyData(data);
    }

    emit currentProfileChanged();
}

// new profile with default settings, selected; returns its id
int DeviceManager::addProfile(const QString &name)
{
    int profile = m_dev_db.addProfile(name);
    emit profilesChanged();
    setCurrentProfile(profile);
    return profile;
}

// new profile with the settings of another one, selected; returns its id (-1 if unknown)
int DeviceManager::duplicateProfile(int profile, const QString &name)
{
    if (!m_dev_db.profiles().contains(profile))
        return -1;

    int copy = m_dev_db.duplicateProfile(profile, name);
    emit profilesChanged();
    setCurrentProfile(copy);
    return copy;
}

void DeviceManager::renameProfile(int profile, const QString &name)
{
    if (name.isEmpty() || !m_dev_db.profiles().contains(profile))
        return;
    m_dev_db.setProfileName(profile, name);
    emit profilesChanged();
}

// removes a profile (not the last one), selecting another one if needed
void DeviceManager::removeProfile(int profile)
{
    QVector<int> ids = m_dev_db.profiles();
    if (ids.size() <= 1 || !ids.contains(profile))
        return;

    if (profile == m_current_profile)
    {
        int index = ids.indexOf(profile);
        setCurrentProfile(index > 0 ? ids.at(index - 1) : ids.at(index + 1));
    }

    m_dev_db.removeProfile(profile);
    emit profilesChanged();
}

// write a key assignment of the current profile to the keyboard
void DeviceManager::writeKeyData(const KeyData &data)
{
    std::optional<drevo::Key> key = drevo::Key::fromValue(data.key_value);
    std::optional<drevo::KeyAction> action = toKeyAction(data);
    if (!key || !action)
    {
        qWarning() << "invalid key assignment, not written: key" << data.key_value << "type" << data.macro_type
                   << "values" << data.macro_value << data.macro_value1 << data.macro_value2;
        return;
    }
    m_keyboard.setKeyAction(*key, *action);
}

// write the default function of a key to the keyboard
void DeviceManager::writeKeyDefault(int key_value)
{
    if (std::optional<drevo::Key> key = drevo::Key::fromValue(key_value))
        m_keyboard.resetKey(*key);
}

// key values with an assignment in a profile
QVector<int> DeviceManager::assignedKeys(int profile)
{
    QVector<KeyData*> vec_data;
    m_dev_db.queryKeyProfileInfo(profile, vec_data);

    QVector<int> keys;
    for (KeyData *data : vec_data)
    {
        if (data)
            keys.append(data->key_value);
        delete data;
    }
    return keys;
}

// title for the result of opening the keyboard
QString DeviceManager::connectionText(drevo::ConnectionState state) const
{
    if (!m_keyboard.device())
        return tr("CONNECT YOUR DEVICE");

    QString name = m_keyboard.device()->name;
    switch (state)
    {
    case drevo::ConnectionState::NoAccess:  name.append(tr(" (no access, see the udev rule in the README)")); break;
    case drevo::ConnectionState::Busy:      name.append(tr(" (in use by another program)")); break;
    default:                                break;
    }
    return name;
}

DeviceManager* DeviceManager::create(QQmlEngine *, QJSEngine *)
{
    // the instance is owned by main(), not by the QML engine
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}
