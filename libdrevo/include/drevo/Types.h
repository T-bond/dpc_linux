#ifndef DREVO_TYPES_H
#define DREVO_TYPES_H

#include <QMetaEnum>
#include <QObject>
#include <QString>

#include <algorithm>
#include <optional>

// values accepted by drevo::Keyboard; anything that reaches the keyboard is one of these
namespace drevo
{
Q_NAMESPACE

// key layout of a keyboard (the values are part of the protocol)
enum class Layout : quint8
{
    Tkl87 = 1,      // 87 keys, ANSI
    Iso88 = 2,      // 88 keys, ISO
    Jis91 = 4,      // 91 keys, JIS
};
Q_ENUM_NS(Layout)

enum class Model
{
    BladeMasterTE87,
    BladeMasterTE88,
    BladeMasterTE91,
    BladeMasterPro87,
    BladeMasterPro88,
    BladeMasterPro91,
};
Q_ENUM_NS(Model)

// result of opening a keyboard
enum class ConnectionState
{
    Connected,
    NotFound,       // no supported keyboard is plugged in
    NoAccess,       // cannot open the device (udev rule missing)
    Busy,           // interface claimed by another program
    DetachFailed,   // kernel driver could not be detached
    ClaimFailed,    // interface could not be claimed
};
Q_ENUM_NS(ConnectionState)

// actions of the knob, programmed like keys
enum class KnobAction
{
    SingleClick = 500,
    DoubleClick = 501,
    Forward     = 502,
    Backward    = 503,
};
Q_ENUM_NS(KnobAction)

// modifier keys of a combo key (HID usages)
enum class Modifier : quint8
{
    None        = 0x00,
    LeftCtrl    = 0xE0,
    LeftShift   = 0xE1,
    LeftAlt     = 0xE2,
    LeftGui     = 0xE3,
    RightCtrl   = 0xE4,
    RightShift  = 0xE5,
    RightAlt    = 0xE6,
    RightGui    = 0xE7,
};
Q_ENUM_NS(Modifier)

// mouse functions a key can send (keyboard codes)
enum class MouseAction : quint8
{
    LeftClick   = 0xA5,
    RightClick  = 0xA6,
    MiddleClick = 0xA7,
    Scroll      = 0xA8,
    Button4     = 0xA9,
    Button5     = 0xAA,
};
Q_ENUM_NS(MouseAction)

// multimedia functions a key can send (keyboard codes)
enum class MediaAction : quint8
{
    VolumeUp    = 0xE8,
    VolumeDown  = 0xE9,
    Mute        = 0xEA,
    Previous    = 0xF1,
    PlayPause   = 0xF2,
    Next        = 0xF3,
    Stop        = 0xF4,
};
Q_ENUM_NS(MediaAction)

// report interval in ms
enum class ReportRate : quint8
{
    Ms1 = 1,
    Ms2 = 2,
    Ms4 = 4,
    Ms8 = 8,
};
Q_ENUM_NS(ReportRate)

enum class LightMode : quint8
{
    Static          = 1,
    Spectrum        = 2,
    Rainbow         = 3,
    PowerGauge      = 4,
    Breathing       = 5,
    TwinklingStars  = 6,
    Reactive        = 7,
    Marquee         = 8,
    Aurora          = 9,
    Custom          = 12,   // per-key colors, see Keyboard::setLedColors()
};
Q_ENUM_NS(LightMode)

// direction of the rainbow effect
// (left/right checked on the keyboard, down/up values not verified)
enum class RainbowDirection : quint8
{
    RightToLeft = 0,
    LeftToRight = 1,
    DownToUp    = 2,
    UpToDown    = 3,
};
Q_ENUM_NS(RainbowDirection)

// side light bars around the keyboard
enum class LightBar
{
    Left    = 600,
    Top     = 601,
    Right   = 602,
    Bottom  = 603,
};
Q_ENUM_NS(LightBar)

// enum value from an integer (e.g. a stored setting), empty if the enum has no such value
template <typename Enum>
std::optional<Enum> enumFromValue(int value)
{
    if (!QMetaEnum::fromType<Enum>().valueToKey(value))
        return std::nullopt;
    return Enum(value);
}

// integer limited to [Min, Max]
template <int Min, int Max>
class BoundedInt
{
public:
    static constexpr int min = Min;
    static constexpr int max = Max;

    constexpr BoundedInt() : m_value(Min) {}

    // empty if out of range
    static constexpr std::optional<BoundedInt> make(int value)
    {
        if (value < Min || value > Max)
            return std::nullopt;
        return BoundedInt(value);
    }
    static constexpr BoundedInt clamped(int value) { return BoundedInt(std::clamp(value, Min, Max)); }

    constexpr int value() const { return m_value; }
    constexpr bool operator==(const BoundedInt &other) const { return m_value == other.m_value; }

private:
    constexpr explicit BoundedInt(int value) : m_value(value) {}

    int     m_value;
};

// lighting brightness and effect speed (4-bit fields)
using Brightness = BoundedInt<0, 15>;
using Speed = BoundedInt<0, 4>;
// sleep time in seconds, 0 = never (16-bit field)
using SleepSeconds = BoundedInt<0, 0xFFFF>;

struct Rgb
{
    quint8  r = 0;
    quint8  g = 0;
    quint8  b = 0;

    constexpr bool operator==(const Rgb &other) const { return r == other.r && g == other.g && b == other.b; }
};

// a physical key of a supported keyboard (HID usage), or a knob action
class Key
{
public:
    // empty if no supported layout has this key
    static std::optional<Key> fromValue(int value);
    static constexpr Key knob(KnobAction action) { return Key(int(action)); }

    constexpr int value() const { return m_value; }
    constexpr bool isKnob() const { return m_value >= int(KnobAction::SingleClick) && m_value <= int(KnobAction::Backward); }
    constexpr bool operator==(const Key &other) const { return m_value == other.m_value; }

private:
    constexpr explicit Key(int value) : m_value(value) {}

    int     m_value;
};

// a keyboard page HID usage a key can send (target of a remap or combo key)
class Usage
{
public:
    // empty outside the keyboard usages (0x04..0xA4) and modifiers (0xE0..0xE7)
    static constexpr std::optional<Usage> fromValue(int value)
    {
        if ((value >= 0x04 && value <= 0xA4) || (value >= 0xE0 && value <= 0xE7))
            return Usage(quint8(value));
        return std::nullopt;
    }

    constexpr quint8 value() const { return m_value; }
    constexpr bool operator==(const Usage &other) const { return m_value == other.m_value; }

private:
    constexpr explicit Usage(quint8 value) : m_value(value) {}

    quint8  m_value;
};

// a supported keyboard
struct DeviceInfo
{
    Model       model;
    Layout      layout;
    quint16     vendorId;
    quint16     productId;
    QString     name;
};

} // namespace drevo

#endif // DREVO_TYPES_H
