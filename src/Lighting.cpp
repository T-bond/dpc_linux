#include "Lighting.h"

#include "DeviceManager.h"

namespace
{

struct LightModeItem
{
    int             mode;
    const char     *text;
    const char     *icon;
};

// light modes in list order
const LightModeItem kLightModes[] = {
    { 1,  QT_TRANSLATE_NOOP("Lighting", "Static"),      "icon_light_static" },
    { 2,  QT_TRANSLATE_NOOP("Lighting", "Spectrum"),    "icon_light_spectrum" },
    { 3,  QT_TRANSLATE_NOOP("Lighting", "Rainbow"),     "icon_light_rainbow" },
    { 4,  QT_TRANSLATE_NOOP("Lighting", "Power Gauge"), "icon_light_powergauge" },
    { 5,  QT_TRANSLATE_NOOP("Lighting", "Breathing"),   "icon_light_breathing" },
    { 6,  QT_TRANSLATE_NOOP("Lighting", "Twinkling Stars"), "icon_light_twinklingstars" },
    { 7,  QT_TRANSLATE_NOOP("Lighting", "Reactive"),    "icon_light_reactive" },
    { 8,  QT_TRANSLATE_NOOP("Lighting", "Marquee"),     "icon_light_marquee" },
    { 9,  QT_TRANSLATE_NOOP("Lighting", "Aurora"),      "icon_light_aurora" },
    { 12, QT_TRANSLATE_NOOP("Lighting", "Custom"),      "icon_light_custom" },
};
const int kLightModeCount = sizeof(kLightModes) / sizeof(kLightModes[0]);

const int LM_STATIC = 1;
const int LM_CUSTOM = 12;

const QColor kDefaultLightColor(255, 225, 0, 255);

// get light mode by list index
int getLightModeByIndex(int index)
{
    if (index < 0 || index >= kLightModeCount)
        return LM_STATIC;
    return kLightModes[index].mode;
}

// get list index by light mode
int getIndexByLightMode(int mode)
{
    for (int i = 0; i < kLightModeCount; i++)
    {
        if (kLightModes[i].mode == mode)
            return i;
    }
    return 1;
}

QColor radiColor(const RadiData &data)
{
    return QColor(data.r_value, data.g_value, data.b_value, 255);
}

} // namespace

Lighting::Lighting(QObject *parent)
    : QObject(parent)
{
    m_current_mode = 0;
    m_mode_index = -1;
    m_current_profile = DeviceManager::instance()->currentProfile();

    m_brightness = 0;
    m_speed = 0;
    m_speed_enabled = true;
    m_color_visible = true;
    m_color_enabled = true;
    m_color = kDefaultLightColor;
    m_custom_color_visible = true;
    m_custom_color = false;
    m_light_color = kDefaultLightColor;

    // "lights off" ended from the tray: send the lighting of the profile again
    connect(DeviceManager::instance(), &DeviceManager::lightingRestoreRequested, this, [this]() {
        m_current_mode = 0;
        loadProfile();
    });

    connect(DeviceManager::instance(), &DeviceManager::currentProfileChanged, this, [this]() {
        m_current_profile = DeviceManager::instance()->currentProfile();
        // forget the colors and selection of the previous profile
        if (m_keyboard)
        {
            m_keyboard->setAllKeysColor(QColor(0, 0, 0, 0));
            m_keyboard->setAllKeyCheck(false);
        }
        m_current_mode = 0;
        loadProfile();
    });
}

void Lighting::componentComplete()
{
    loadProfile();
}

// select the light mode of the current profile
void Lighting::loadProfile()
{
    DeviceDB *dev_db = DeviceManager::instance()->db();

    int select_mode = dev_db->getSelectMode(m_current_profile);
    m_mode_index = getIndexByLightMode(select_mode);
    emit modeChanged();

    setBackLightMode(select_mode);
}

void Lighting::setKeyboard(KeyboardModel *keyboard)
{
    if (m_keyboard == keyboard)
        return;
    m_keyboard = keyboard;
    emit keyboardChanged();
}

QVariantList Lighting::modes() const
{
    QVariantList list;
    for (const LightModeItem &item : kLightModes)
    {
        list.append(QVariantMap {
            { "text", tr(item.text) },
            { "icon", QString("qrc:/image/icon/%1.png").arg(item.icon) },
        });
    }
    return list;
}

int Lighting::modeIndex() const
{
    return m_mode_index;
}

bool Lighting::customMode() const
{
    return m_current_mode == LM_CUSTOM;
}

// select the light mode at a list index
void Lighting::selectModeIndex(int index)
{
    int light_mode = getLightModeByIndex(index);

    m_mode_index = index;
    emit modeChanged();

    DeviceManager::instance()->db()->setSelectMode(m_current_profile, light_mode);
    setBackLightMode(light_mode);
}

void Lighting::setBrightness(int value)
{
    m_brightness = value;
    emit settingsChanged();

    RadiData data;
    if (!currentRadiData(data))
        return;

    data.rgb_brightness = value;
    updateRadiData(data);
}

void Lighting::setSpeed(int value)
{
    m_speed = value;
    emit settingsChanged();

    RadiData data;
    if (!currentRadiData(data))
        return;

    data.rgb_speed = value;
    updateRadiData(data);
}

// set the effect color, or the color of the checked keys in custom mode
void Lighting::setColor(const QColor &color)
{
    RadiData data;
    if (!currentRadiData(data))
        return;

    data.r_value = color.red();
    data.g_value = color.green();
    data.b_value = color.blue();

    if (data.r_value == 0 && data.g_value == 0 && data.b_value == 0)
        return ;

    // picking a color means using it
    if (m_current_mode != LM_CUSTOM)
        data.rgb_mode = 1;

    DeviceDB *dev_db = DeviceManager::instance()->db();
    if (!dev_db->modifyRadiRGBData(data))
        return;

    uint8_t  kb_data[8] = {0};
    hid_getRadiData(data, kb_data);
    sendPacket(kb_data, 8);

    m_color = radiColor(data);
    if (m_current_mode != LM_CUSTOM)
    {
        m_custom_color = true;
        m_color_enabled = true;
    }
    emit settingsChanged();

    if (m_current_mode == LM_CUSTOM)
    {
        if (m_keyboard)
        {
            m_keyboard->setCheckKeyColor(color);

            for (const KeyboardKey &key : m_keyboard->keys())
            {
                RGBData rgb_data;
                rgb_data.radi_id    = data.radi_id;
                rgb_data.key_value  = key.key_value;
                rgb_data.r_value    = key.key_color.red();
                rgb_data.g_value    = key.key_color.green();
                rgb_data.b_value    = key.key_color.blue();

                dev_db->addKeyRGBData(rgb_data);
            }
        }
        sendKeyRGBData(data.radi_id, false);
    }
    else
    {
        setLightColor(color);
    }
}

void Lighting::setCustomColor(bool custom_color)
{
    m_custom_color = custom_color;
    emit settingsChanged();

    RadiData data;
    if (!currentRadiData(data))
        return;

    data.rgb_mode = custom_color ? 1 : 0;
    m_color_enabled = data.rgb_mode == 1;
    emit settingsChanged();

    updateRadiData(data);
}

// remove all custom key colors
void Lighting::resetAllLeds()
{
    RadiData data;
    if (!currentRadiData(data))
        return;

    if (!DeviceManager::instance()->db()->deleteKeyRGBData(data.radi_id))
        return;

    sendKeyRGBData(data.radi_id, false);

    if (m_keyboard)
        m_keyboard->setAllKeysColor(QColor(0, 0, 0, 255));
}

// set backlight mode
void Lighting::setBackLightMode(int light_mode)
{
    if (m_current_mode == light_mode)
        return ;

    RadiData data;
    if (!DeviceManager::instance()->db()->queryRadiInfo(m_current_profile, light_mode, data))
        return;

    m_current_mode = light_mode;
    emit modeChanged();

    uint8_t  kb_data[8] = {0};
    hid_getRadiData(data, kb_data);
    sendPacket(kb_data, 8);

    m_brightness = data.rgb_brightness;

    if (light_mode == 1 || light_mode == 12)
    {
        m_speed = 0;
        m_speed_enabled = false;
        m_color_visible = true;
        m_color = radiColor(data);
        m_custom_color = data.rgb_mode == 1;

        if (light_mode == 12)
        {
            m_custom_color_visible = false;
            m_color_enabled = true;
            emit settingsChanged();

            sendKeyRGBData(data.radi_id, true);
        }
        else
        {
            m_custom_color_visible = true;
            m_color_enabled = data.rgb_mode == 1;
            emit settingsChanged();

            setLightColor(radiColor(data));
        }
    }
    else if (light_mode == 2 || light_mode == 3 || light_mode == 4 || light_mode == 8)
    {
        m_speed = data.rgb_speed;
        m_speed_enabled = true;
        m_color_visible = false;
        m_custom_color_visible = false;
        emit settingsChanged();

        setLightColor(kDefaultLightColor);
    }
    else if (light_mode == 5 || light_mode == 6 || light_mode == 7 || light_mode == 9)
    {
        m_speed = data.rgb_speed;
        m_speed_enabled = true;
        m_color_visible = true;
        m_custom_color_visible = true;
        m_color = radiColor(data);
        m_custom_color = data.rgb_mode == 1;
        m_color_enabled = data.rgb_mode == 1;
        emit settingsChanged();

        setLightColor(radiColor(data));
    }
    else
    {
        emit settingsChanged();
    }
}

// query the data of the current mode
bool Lighting::currentRadiData(RadiData &data) const
{
    return DeviceManager::instance()->db()->queryRadiInfo(m_current_profile, m_current_mode, data);
}

// store the data of the current mode and send it to the keyboard
void Lighting::updateRadiData(RadiData &data)
{
    if (!DeviceManager::instance()->db()->modifyRadiRGBData(data))
        return;

    uint8_t  kb_data[8] = {0};
    hid_getRadiData(data, kb_data);
    sendPacket(kb_data, 8);
}

// send the key colors of the custom mode to the keyboard
void Lighting::sendKeyRGBData(int radi_id, bool update_keyboard)
{
    QVector<RGBData*> vec_data;
    DeviceManager::instance()->db()->queryKeysRGBData(radi_id, vec_data);

    // key positions differ per layout; the widget UI always sent the 87-key layout
    int kb_layout = DeviceManager::instance()->comm()->getKeyboardLayout();
    uint8_t kb_data[408] = {0};
    hid_getKeyRGBData(vec_data, kb_layout, kb_data);
    sendPacket(kb_data, 408);

    for (RGBData *rgb_data : vec_data)
    {
        if (rgb_data && update_keyboard && m_keyboard)
        {
            QColor color(rgb_data->r_value, rgb_data->g_value, rgb_data->b_value, 255);
            m_keyboard->setKeyColor(rgb_data->key_value, color);
        }
        delete rgb_data;
    }
}

// send lighting data; it ends "lights off"
void Lighting::sendPacket(const uint8_t *kb_data, size_t length)
{
    DeviceManager *device = DeviceManager::instance();
    if (length > 8)
        device->comm()->setDeviceDatas(kb_data, length);
    else
        device->comm()->setDeviceData(kb_data, length);
    device->lightingSent();
}

void Lighting::setLightColor(const QColor &color)
{
    if (m_light_color == color)
        return;
    m_light_color = color;
    emit lightColorChanged();
}
