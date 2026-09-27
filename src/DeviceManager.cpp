#include "DeviceManager.h"

#include <QDebug>
#include <QJSEngine>
#include <QSettings>
#include <QTimer>

#include <algorithm>

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

// the two assignments do the same (the name is not sent to the keyboard)
bool sameAssignment(const KeyData *a, const KeyData *b)
{
    if (!a || !b)
        return a == b;
    return a->macro_type == b->macro_type && a->macro_value == b->macro_value
        && a->macro_value1 == b->macro_value1 && a->macro_value2 == b->macro_value2;
}

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

    // follow plugging and unplugging; the delay lets the udev rule give access to the new device
    m_refresh_timer = new QTimer(this);
    m_refresh_timer->setSingleShot(true);
    m_refresh_timer->setInterval(1000);
    connect(m_refresh_timer, &QTimer::timeout, this, &DeviceManager::refreshConnection);
    if (!m_keyboard.watchDevices([this]() { m_refresh_timer->start(); }))
        qWarning() << "Keyboard plugging cannot be followed, use the refresh button";

    // nothing is written at startup: the keyboard keeps what was written before
    m_hardware_profile = m_dev_db.selectedHardwareProfile();
    m_current_profile = m_dev_db.hardwareProfileProfile(m_hardware_profile);
    if (m_current_profile > 0)
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

void DeviceManager::refreshConnection()
{
    const drevo::Layout old_layout = keyboardLayout();
    m_device_name = connectionText(m_keyboard.open());
    emit connectionChanged();
    // the keyboard image depends on the layout
    if (keyboardLayout() != old_layout)
        emit keyboardRegionChanged();
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
    {
        list.append(QVariantMap {
            { "id", id },
            { "name", m_dev_db.profileName(id) },
            { "hardwareProfile", m_dev_db.profileHardwareProfile(id) },
        });
    }
    return list;
}

QVector<int> DeviceManager::profilesOf(int hardware_profile) const
{
    QVector<int> ids;
    for (int id : m_dev_db.profiles())
    {
        if (m_dev_db.profileHardwareProfile(id) == hardware_profile)
            ids.append(id);
    }
    return ids;
}

QVariantList DeviceManager::hardwareProfileProfiles() const
{
    QVariantList list;
    for (int id : profilesOf(m_hardware_profile))
        list.append(QVariantMap { { "id", id }, { "name", m_dev_db.profileName(id) } });
    return list;
}

QVariantList DeviceManager::hardwareProfiles() const
{
    QVariantList list;
    for (int i = 0; i < kHardwareProfileCount; i++)
    {
        list.append(QVariantMap {
            { "name", QString("G%1").arg(i + 1) },
            { "profile", m_dev_db.hardwareProfileProfile(i) },
            { "known", m_dev_db.hardwareProfileKnown(i) },
            { "storedName", m_dev_db.hardwareProfileKnown(i) ? m_dev_db.hardwareProfileName(i) : QString() },
        });
    }
    return list;
}

void DeviceManager::setCurrentProfile(int profile)
{
    if (!m_dev_db.profiles().contains(profile))
        return;

    const int hardware_profile = m_dev_db.profileHardwareProfile(profile);
    if (m_dev_db.hardwareProfileProfile(hardware_profile) != profile)
        writeProfile(profile, hardware_profile);
    showHardwareProfile(hardware_profile);
}

void DeviceManager::setHardwareProfile(int hardware_profile)
{
    if (hardware_profile < 0 || hardware_profile >= kHardwareProfileCount)
        return;
    showHardwareProfile(hardware_profile);
}

// show a hardware profile and its current profile
void DeviceManager::showHardwareProfile(int hardware_profile)
{
    const int profile = m_dev_db.hardwareProfileProfile(hardware_profile);
    const bool hardware_changed = hardware_profile != m_hardware_profile;
    const bool profile_changed = profile != m_current_profile;

    m_hardware_profile = hardware_profile;
    m_current_profile = profile;
    m_dev_db.setSelectedHardwareProfile(hardware_profile);
    if (profile > 0)
        m_dev_db.writeRadiData(profile);

    if (hardware_changed)
    {
        emit hardwareProfileChanged();
        // the profile list shows the profiles of the hardware profile
        emit profilesChanged();
    }
    emit hardwareProfilesChanged();
    if (profile_changed)
        emit currentProfileChanged();
}

// write the key assignments of a profile to a hardware profile
void DeviceManager::writeProfile(int profile, int hardware_profile)
{
    const KeyMap keys = m_dev_db.profileKeys(profile);
    const bool known = m_dev_db.hardwareProfileKnown(hardware_profile);

    // keys to write: all keys of the layout if the keyboard's are not known, else the ones that differ
    QList<int> key_values;
    if (known)
    {
        const KeyMap stored = m_dev_db.hardwareProfileKeys(hardware_profile);
        QList<int> candidates = stored.keys() + keys.keys();
        std::sort(candidates.begin(), candidates.end());
        candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
        for (int key_value : candidates)
        {
            const auto old_it = stored.constFind(key_value);
            const auto new_it = keys.constFind(key_value);
            if (!sameAssignment(old_it != stored.cend() ? &*old_it : nullptr, new_it != keys.cend() ? &*new_it : nullptr))
                key_values.append(key_value);
        }
    }
    else
    {
        for (const drevo::Key &key : m_keyboard.keys())
            key_values.append(key.value());
    }

    bool sent = true;
    for (int key_value : key_values)
    {
        const auto it = keys.constFind(key_value);
        sent &= sendKey(key_value, it != keys.cend() ? &*it : nullptr, hardware_profile);
    }

    // if a transfer failed, the keyboard's keys are not known: the next write sends all of them
    m_dev_db.setHardwareProfileContent(hardware_profile, keys, sent, m_dev_db.profileName(profile));
    m_dev_db.setHardwareProfileProfile(hardware_profile, profile);
}

// a key assignment of the current profile changed: write it to its hardware profile
void DeviceManager::writeKey(int key_value)
{
    if (m_current_profile <= 0)
        return;

    KeyData data = {};
    const bool assigned = m_dev_db.queryKeyProfileInfo(m_current_profile, key_value, data);

    const KeyMap stored_keys = m_dev_db.hardwareProfileKeys(m_hardware_profile);
    const auto it = stored_keys.constFind(key_value);
    const bool known = m_dev_db.hardwareProfileKnown(m_hardware_profile);

    // unknown: send it anyway, the other keys stay unknown
    bool sent = true;
    if (!known || !sameAssignment(it != stored_keys.cend() ? &*it : nullptr, assigned ? &data : nullptr))
        sent = sendKey(key_value, assigned ? &data : nullptr, m_hardware_profile);
    m_dev_db.setHardwareProfileKey(m_hardware_profile, key_value, assigned ? &data : nullptr, known && sent);
}

// write one key assignment (nullptr: the default function), false if it was not sent
bool DeviceManager::sendKey(int key_value, const KeyData *data, int hardware_profile)
{
    std::optional<drevo::Key> key = drevo::Key::fromValue(key_value);
    std::optional<drevo::HardwareProfile> target = drevo::enumFromValue<drevo::HardwareProfile>(hardware_profile);
    if (!key || !target)
        return false;

    if (!data)
        return m_keyboard.resetKey(*key, *target);

    std::optional<drevo::KeyAction> action = toKeyAction(*data);
    if (!action)
    {
        qWarning() << "invalid key assignment, not written: key" << data->key_value << "type" << data->macro_type
                   << "values" << data->macro_value << data->macro_value1 << data->macro_value2;
        return false;
    }
    return m_keyboard.setKeyAction(*key, *action, *target);
}

// restore the factory settings
void DeviceManager::resetKeyboard()
{
    const bool reset = m_keyboard.resetToFactory();
    for (int i = 0; i < kHardwareProfileCount; i++)
    {
        m_dev_db.setHardwareProfileContent(i, KeyMap(), reset, QString());
        m_dev_db.setHardwareProfileProfile(i, -1);
    }
    showHardwareProfile(m_hardware_profile);
}

// moves a profile to another hardware profile and writes it there
void DeviceManager::moveProfile(int profile, int hardware_profile)
{
    if (!m_dev_db.profiles().contains(profile) || hardware_profile < 0 || hardware_profile >= kHardwareProfileCount)
        return;
    const int old_hardware_profile = m_dev_db.profileHardwareProfile(profile);
    if (old_hardware_profile == hardware_profile)
        return;

    m_dev_db.setProfileHardwareProfile(profile, hardware_profile);
    // the former hardware profile keeps the keys on the keyboard
    if (m_dev_db.hardwareProfileProfile(old_hardware_profile) == profile)
        m_dev_db.setHardwareProfileProfile(old_hardware_profile, -1);
    writeProfile(profile, hardware_profile);

    emit profilesChanged();
    // the current profile is followed to its new hardware profile
    if (profile == m_current_profile)
        showHardwareProfile(hardware_profile);
    else
        showHardwareProfile(m_hardware_profile);
}

// new profile with default settings, selected; returns its id
int DeviceManager::addProfile(const QString &name)
{
    int profile = m_dev_db.addProfile(name);
    m_dev_db.setProfileHardwareProfile(profile, m_hardware_profile);
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
    // the hardware profile that has it names it
    const int hardware_profile = m_dev_db.profileHardwareProfile(profile);
    if (m_dev_db.hardwareProfileProfile(hardware_profile) == profile)
    {
        m_dev_db.setHardwareProfileContent(hardware_profile, m_dev_db.hardwareProfileKeys(hardware_profile),
                                           m_dev_db.hardwareProfileKnown(hardware_profile), name);
    }
    emit profilesChanged();
    emit hardwareProfilesChanged();
}

// removes a profile (not the last one)
void DeviceManager::removeProfile(int profile)
{
    QVector<int> ids = m_dev_db.profiles();
    if (ids.size() <= 1 || !ids.contains(profile))
        return;

    const int hardware_profile = m_dev_db.profileHardwareProfile(profile);
    if (m_dev_db.hardwareProfileProfile(hardware_profile) == profile)
    {
        // the previous profile of the hardware profile replaces it (the next one for the first)
        const QVector<int> siblings = profilesOf(hardware_profile);
        const int index = siblings.indexOf(profile);
        const int next = siblings.size() <= 1 ? -1 : siblings.at(index > 0 ? index - 1 : index + 1);
        if (next > 0)
            writeProfile(next, hardware_profile);
        else
            m_dev_db.setHardwareProfileProfile(hardware_profile, -1);
    }

    m_dev_db.removeProfile(profile);
    emit profilesChanged();
    showHardwareProfile(m_hardware_profile);
}

// title for the result of opening the keyboard
QString DeviceManager::connectionText(drevo::ConnectionState state) const
{
    if (state == drevo::ConnectionState::ReceiverOnly && m_keyboard.receiver())
        return m_keyboard.receiver()->name;
    if (!m_keyboard.device())
        return tr("CONNECT YOUR DEVICE");

    QString name = m_keyboard.device()->name;
    switch (state)
    {
    case drevo::ConnectionState::NoAccess:  name.append(tr(" (no access, see the udev rule in the README)")); break;
    case drevo::ConnectionState::OpenFailed: name.append(tr(" (cannot be opened)")); break;
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
