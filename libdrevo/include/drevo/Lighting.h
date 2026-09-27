#ifndef DREVO_LIGHTING_H
#define DREVO_LIGHTING_H

#include "drevo/Types.h"

#include <variant>

namespace drevo
{

// Backlight effects, one type per light mode with only the settings that mode has.
// useColor: use color instead of the keyboard's own colors.

// one color on all keys
struct StaticEffect
{
    static constexpr LightMode mode = LightMode::Static;

    Brightness  brightness;
    Rgb         color;
    bool        useColor = false;

    // all lights dark: color black, brightness 0
    static constexpr StaticEffect off() { return StaticEffect { Brightness(), Rgb(), true }; }
};

// animation with the keyboard's own colors
template <LightMode Mode>
struct AnimatedEffect
{
    static constexpr LightMode mode = Mode;

    Brightness  brightness;
    Speed       speed;
};

using SpectrumEffect = AnimatedEffect<LightMode::Spectrum>;
using PowerGaugeEffect = AnimatedEffect<LightMode::PowerGauge>;
using MarqueeEffect = AnimatedEffect<LightMode::Marquee>;

struct RainbowEffect
{
    static constexpr LightMode mode = LightMode::Rainbow;

    Brightness          brightness;
    Speed               speed;
    RainbowDirection    direction = RainbowDirection::RightToLeft;
};

// animation that can use a color
template <LightMode Mode>
struct ColorAnimatedEffect
{
    static constexpr LightMode mode = Mode;

    Brightness  brightness;
    Speed       speed;
    Rgb         color;
    bool        useColor = false;
};

using BreathingEffect = ColorAnimatedEffect<LightMode::Breathing>;
using TwinklingStarsEffect = ColorAnimatedEffect<LightMode::TwinklingStars>;
using ReactiveEffect = ColorAnimatedEffect<LightMode::Reactive>;
using AuroraEffect = ColorAnimatedEffect<LightMode::Aurora>;

// per-key and side LED colors, set with Keyboard::setLedColors()
struct CustomEffect
{
    static constexpr LightMode mode = LightMode::Custom;

    Brightness  brightness;
    Rgb         color;
};

using LightingEffect = std::variant<StaticEffect, SpectrumEffect, RainbowEffect, PowerGaugeEffect,
                                    BreathingEffect, TwinklingStarsEffect, ReactiveEffect, MarqueeEffect,
                                    AuroraEffect, CustomEffect>;

// light mode of an effect
inline LightMode lightMode(const LightingEffect &effect)
{
    return std::visit([](const auto &e) { return e.mode; }, effect);
}

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
