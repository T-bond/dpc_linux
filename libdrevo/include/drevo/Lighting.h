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

// number of LEDs in a side light bar
constexpr int ledCount(LightBar bar)
{
    switch (bar)
    {
    case LightBar::Top:     return 14;
    case LightBar::Right:   return 5;
    case LightBar::Bottom:  return 14;
    case LightBar::Left:    return 5;
    }
    return 0;
}

// one LED of a side light bar; index counts left to right (top and bottom bar)
// or top to bottom (left and right bar)
class SideLed
{
public:
    // empty if the bar has no LED at index
    static constexpr std::optional<SideLed> make(LightBar bar, int index)
    {
        if (index < 0 || index >= ledCount(bar))
            return std::nullopt;
        return SideLed(bar, index);
    }

    constexpr LightBar bar() const { return m_bar; }
    constexpr int index() const { return m_index; }
    constexpr bool operator==(const SideLed &other) const { return m_bar == other.m_bar && m_index == other.m_index; }

private:
    constexpr SideLed(LightBar bar, int index) : m_bar(bar), m_index(index) {}

    LightBar    m_bar;
    int         m_index;
};

// color of a key, a whole side light bar or one side LED in the custom light mode
struct LedColor
{
    std::variant<Key, LightBar, SideLed>    target;
    Rgb                                     color;
};

} // namespace drevo

#endif // DREVO_LIGHTING_H
