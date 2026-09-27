#include "Lighting.h"

#include "DeviceManager.h"
#include "LightModes.h"

#include <QDebug>

namespace
{

struct DirectionItem
{
    drevo::RainbowDirection     direction;
    const char                 *text;
};

// rainbow directions in list order
const DirectionItem kDirections[] = {
    { drevo::RainbowDirection::LeftToRight, QT_TRANSLATE_NOOP("Lighting", "Left to right") },
    { drevo::RainbowDirection::RightToLeft, QT_TRANSLATE_NOOP("Lighting", "Right to left") },
    { drevo::RainbowDirection::DownToUp,    QT_TRANSLATE_NOOP("Lighting", "Down to up") },
    { drevo::RainbowDirection::UpToDown,    QT_TRANSLATE_NOOP("Lighting", "Up to down") },
};

const int kLightModeCount = int(std::size(kLightModes));

const int LM_STATIC = int(drevo::LightMode::Static);
const int LM_CUSTOM = int(drevo::LightMode::Custom);

const QColor kDefaultLightColor(255, 225, 0, 255);

// get light mode by list index
int getLightModeByIndex(int index)
{
    if (index < 0 || index >= kLightModeCount)
        return LM_STATIC;
    return int(kLightModes[index].mode);
}

// get list index by light mode
int getIndexByLightMode(int mode)
{
    for (int i = 0; i < kLightModeCount; i++)
    {
        if (int(kLightModes[i].mode) == mode)
            return i;
    }
    return 1;
}

QColor radiColor(const RadiData &data)
{
    return QColor(data.r_value, data.g_value, data.b_value, 255);
}

// stored light mode settings as a keyboard effect, empty if the mode is unknown
std::optional<drevo::LightingEffect> toEffect(const RadiData &data)
{
    std::optional<drevo::LightMode> mode = drevo::enumFromValue<drevo::LightMode>(data.mode);
    if (!mode)
        return std::nullopt;

    const drevo::Brightness brightness = drevo::Brightness::clamped(data.rgb_brightness);
    const drevo::Speed speed = drevo::Speed::clamped(data.rgb_speed);
    const drevo::Rgb color { quint8(data.r_value), quint8(data.g_value), quint8(data.b_value) };
    const bool use_color = data.rgb_mode == 1;
    const drevo::RainbowDirection direction = drevo::enumFromValue<drevo::RainbowDirection>(data.rgb_direction)
                                                  .value_or(drevo::RainbowDirection::RightToLeft);

    switch (*mode)
    {
    case drevo::LightMode::Static:          return drevo::StaticEffect { brightness, color, use_color };
    case drevo::LightMode::Spectrum:        return drevo::SpectrumEffect { brightness, speed };
    case drevo::LightMode::Rainbow:         return drevo::RainbowEffect { brightness, speed, direction };
    case drevo::LightMode::PowerGauge:      return drevo::PowerGaugeEffect { brightness, speed };
    case drevo::LightMode::Breathing:       return drevo::BreathingEffect { brightness, speed, color, use_color };
    case drevo::LightMode::TwinklingStars:  return drevo::TwinklingStarsEffect { brightness, speed, color, use_color };
    case drevo::LightMode::Reactive:        return drevo::ReactiveEffect { brightness, speed, color, use_color };
    case drevo::LightMode::Marquee:         return drevo::MarqueeEffect { brightness, speed };
    case drevo::LightMode::Aurora:          return drevo::AuroraEffect { brightness, speed, color, use_color };
    case drevo::LightMode::Custom:          return drevo::CustomEffect { brightness, color };
    }
    return std::nullopt;
}

// stored key colors as keyboard LED colors; unknown keys are skipped
QList<drevo::LedColor> toLedColors(const QVector<RGBData*> &vec_data)
{
    QList<drevo::LedColor> colors;
    for (const RGBData *rgb_data : vec_data)
    {
        if (!rgb_data)
            continue;

        const drevo::Rgb color { quint8(rgb_data->r_value), quint8(rgb_data->g_value), quint8(rgb_data->b_value) };
        if (std::optional<drevo::SideLed> led = sideLedFromValue(rgb_data->key_value))
            colors.append({ *led, color });
        else if (std::optional<drevo::Key> key = drevo::Key::fromValue(rgb_data->key_value))
            colors.append({ *key, color });
    }
    return colors;
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
    m_direction = drevo::RainbowDirection::RightToLeft;
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
    // nothing is written at startup: the keyboard keeps its lighting, it may have been changed on it
    loadProfile(false);
}

// select the light mode of the current profile
void Lighting::loadProfile(bool send)
{
    // no current profile (its hardware profile shows none): the keyboard keeps its lighting
    if (m_current_profile <= 0)
        return;

    DeviceDB *dev_db = DeviceManager::instance()->db();

    int select_mode = dev_db->getSelectMode(m_current_profile);
    m_mode_index = getIndexByLightMode(select_mode);
    emit modeChanged();

    setBackLightMode(select_mode, send);
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
    for (const LightModeInfo &item : kLightModes)
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
    if (m_current_profile <= 0)
        return;

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

bool Lighting::directionVisible() const
{
    return m_current_mode == int(drevo::LightMode::Rainbow);
}

QVariantList Lighting::directions() const
{
    QVariantList list;
    for (const DirectionItem &item : kDirections)
        list.append(QVariantMap { { "value", int(item.direction) }, { "text", tr(item.text) } });
    return list;
}

void Lighting::setDirection(int value)
{
    std::optional<drevo::RainbowDirection> direction = drevo::enumFromValue<drevo::RainbowDirection>(value);
    if (!direction)
        return;

    m_direction = *direction;
    emit settingsChanged();

    RadiData data;
    if (!currentRadiData(data))
        return;

    data.rgb_direction = value;
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

    sendEffect(data);

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
void Lighting::setBackLightMode(int light_mode, bool send)
{
    if (m_current_mode == light_mode || m_current_profile <= 0)
        return ;

    RadiData data;
    if (!DeviceManager::instance()->db()->queryRadiInfo(m_current_profile, light_mode, data))
        return;

    m_current_mode = light_mode;
    emit modeChanged();

    if (send)
        sendEffect(data);

    m_brightness = data.rgb_brightness;
    m_direction = drevo::enumFromValue<drevo::RainbowDirection>(data.rgb_direction)
                      .value_or(drevo::RainbowDirection::RightToLeft);

    const LightModeInfo *info = lightModeInfo(light_mode);
    if (!info)
    {
        emit settingsChanged();
        return;
    }

    m_speed_enabled = info->has(LightSpeed);
    m_speed = m_speed_enabled ? data.rgb_speed : 0;
    m_color_visible = info->has(LightColor);
    m_custom_color_visible = info->has(LightColorSwitch);
    m_custom_color = data.rgb_mode == 1;
    // without the switch, a mode with a color always uses it
    m_color_enabled = !m_custom_color_visible || m_custom_color;
    if (m_color_visible)
        m_color = radiColor(data);
    emit settingsChanged();

    if (light_mode == LM_CUSTOM)
        sendKeyRGBData(data.radi_id, true, send);
    else
        setLightColor(m_color_visible ? radiColor(data) : kDefaultLightColor);
}

// query the data of the current mode
bool Lighting::currentRadiData(RadiData &data) const
{
    if (m_current_profile <= 0)
        return false;
    return DeviceManager::instance()->db()->queryRadiInfo(m_current_profile, m_current_mode, data);
}

// store the data of the current mode and send it to the keyboard
void Lighting::updateRadiData(RadiData &data)
{
    if (!DeviceManager::instance()->db()->modifyRadiRGBData(data))
        return;

    sendEffect(data);
}

// send the key colors of the custom mode to the keyboard
void Lighting::sendKeyRGBData(int radi_id, bool update_keyboard, bool send)
{
    QVector<RGBData*> vec_data;
    DeviceManager::instance()->db()->queryKeysRGBData(radi_id, vec_data);

    // key positions differ per layout; the widget UI always sent the 87-key layout
    if (send)
    {
        DeviceManager *device = DeviceManager::instance();
        device->keyboard()->setLedColors(toLedColors(vec_data));
        device->lightingSent();
    }

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

// send the effect of a light mode; it ends "lights off"
void Lighting::sendEffect(const RadiData &data)
{
    std::optional<drevo::LightingEffect> effect = toEffect(data);
    if (!effect)
    {
        qWarning() << "unknown light mode, not sent:" << data.mode;
        return;
    }
    DeviceManager *device = DeviceManager::instance();
    device->keyboard()->setLighting(*effect);
    device->lightingSent();
}

void Lighting::setLightColor(const QColor &color)
{
    if (m_light_color == color)
        return;
    m_light_color = color;
    emit lightColorChanged();
}
