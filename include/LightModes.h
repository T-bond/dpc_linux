#ifndef LIGHTMODES_H
#define LIGHTMODES_H

#include <QString>
#include <QtGlobal>

#include <drevo/Types.h>

// settings the user can change in a light mode (brightness can always be changed)
enum LightSetting
{
    LightSpeed          = 0x1,
    LightDirection      = 0x2,
    LightColorSwitch    = 0x4,      // "Custom color": use the color or the keyboard's own colors
    LightColor          = 0x8,
};

struct LightModeInfo
{
    drevo::LightMode    mode;
    const char         *key;        // name in the settings file
    const char         *text;
    const char         *icon;
    int                 settings;   // LightSetting flags

    bool has(LightSetting setting) const { return settings & setting; }
};

// light modes in list order
inline constexpr LightModeInfo kLightModes[] = {
    { drevo::LightMode::Static,         "static",           QT_TRANSLATE_NOOP("Lighting", "Static"),            "icon_light_static",            LightColorSwitch | LightColor },
    { drevo::LightMode::Spectrum,       "spectrum",         QT_TRANSLATE_NOOP("Lighting", "Spectrum"),          "icon_light_spectrum",          LightSpeed },
    { drevo::LightMode::Rainbow,        "rainbow",          QT_TRANSLATE_NOOP("Lighting", "Rainbow"),           "icon_light_rainbow",           LightSpeed | LightDirection },
    { drevo::LightMode::PowerGauge,     "power_gauge",      QT_TRANSLATE_NOOP("Lighting", "Power Gauge"),       "icon_light_powergauge",        LightSpeed },
    { drevo::LightMode::Breathing,      "breathing",        QT_TRANSLATE_NOOP("Lighting", "Breathing"),         "icon_light_breathing",         LightSpeed | LightColorSwitch | LightColor },
    { drevo::LightMode::TwinklingStars, "twinkling_stars",  QT_TRANSLATE_NOOP("Lighting", "Twinkling Stars"),   "icon_light_twinklingstars",    LightSpeed | LightColorSwitch | LightColor },
    { drevo::LightMode::Reactive,       "reactive",         QT_TRANSLATE_NOOP("Lighting", "Reactive"),          "icon_light_reactive",          LightSpeed | LightColorSwitch | LightColor },
    { drevo::LightMode::Marquee,        "marquee",          QT_TRANSLATE_NOOP("Lighting", "Marquee"),           "icon_light_marquee",           LightSpeed },
    { drevo::LightMode::Aurora,         "aurora",           QT_TRANSLATE_NOOP("Lighting", "Aurora"),            "icon_light_aurora",            LightSpeed | LightColorSwitch | LightColor },
    // per-key colors; the effect color is set together with the colors of the checked keys
    { drevo::LightMode::Custom,         "custom",           QT_TRANSLATE_NOOP("Lighting", "Custom"),            "icon_light_custom",            LightColor },
};

// mode from its drevo::LightMode value, nullptr if the list has no such mode
inline const LightModeInfo *lightModeInfo(int mode)
{
    for (const LightModeInfo &info : kLightModes)
    {
        if (int(info.mode) == mode)
            return &info;
    }
    return nullptr;
}

// mode from its name in the settings file, nullptr if unknown
inline const LightModeInfo *lightModeInfo(const QString &key)
{
    for (const LightModeInfo &info : kLightModes)
    {
        if (key == QLatin1String(info.key))
            return &info;
    }
    return nullptr;
}

#endif // LIGHTMODES_H
