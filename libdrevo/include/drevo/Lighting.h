#ifndef DREVO_LIGHTING_H
#define DREVO_LIGHTING_H

#include "drevo/Types.h"

#include <variant>

namespace drevo
{

// backlight effect; fields a mode does not use are ignored:
//   Static, Custom                              brightness, useColor, color
//   Spectrum, PowerGauge, Marquee               brightness, speed
//   Rainbow                                     brightness, speed, direction
//   Breathing, TwinklingStars, Reactive, Aurora brightness, speed, useColor, color
struct LightingEffect
{
    LightMode           mode = LightMode::Static;
    Brightness          brightness;
    Speed               speed;
    // use color instead of the keyboard's own colors
    bool                useColor = false;
    Rgb                 color;
    RainbowDirection    direction = RainbowDirection::RightToLeft;

    // all lights dark: static mode, color black, brightness 0
    static LightingEffect off()
    {
        LightingEffect effect;
        effect.useColor = true;
        return effect;
    }
};

// color of a key or a side light bar in the custom light mode
struct LedColor
{
    std::variant<Key, LightBar>     target;
    Rgb                             color;
};

} // namespace drevo

#endif // DREVO_LIGHTING_H
