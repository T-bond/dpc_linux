#include "Describe.h"

namespace drevo::describe
{

namespace
{

// HID keyboard usages 0x28..0x65 that are not letters or digits
const char *const kUsageNames[] = {
    "Enter", "Esc", "Backspace", "Tab", "Space", "-", "=", "[", "]", "\\", "# (ISO)", ";", "'", "`", ",",
    ".", "/", "Caps Lock",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    "Print Screen", "Scroll Lock", "Pause", "Insert", "Home", "Page Up", "Delete", "End", "Page Down",
    "Right", "Left", "Down", "Up", "Num Lock",
    "Keypad /", "Keypad *", "Keypad -", "Keypad +", "Keypad Enter",
    "Keypad 1", "Keypad 2", "Keypad 3", "Keypad 4", "Keypad 5", "Keypad 6", "Keypad 7", "Keypad 8",
    "Keypad 9", "Keypad 0", "Keypad .", "\\ (ISO)", "Menu",
};
static_assert(std::size(kUsageNames) == 0x65 - 0x28 + 1);

QString hexName(int value)
{
    return QStringLiteral("0x%1").arg(value, 2, 16, QLatin1Char('0'));
}

QString color(Rgb rgb)
{
    return QStringLiteral("#%1%2%3").arg(rgb.r, 2, 16, QLatin1Char('0'))
                                    .arg(rgb.g, 2, 16, QLatin1Char('0'))
                                    .arg(rgb.b, 2, 16, QLatin1Char('0'));
}


// settings of a lighting effect
struct LightingSettings
{
    QStringList operator()(const StaticEffect &e) const { return { brightness(e.brightness), useColor(e.useColor, e.color) }; }
    template <LightMode Mode>
    QStringList operator()(const AnimatedEffect<Mode> &e) const { return { brightness(e.brightness), speed(e.speed) }; }
    QStringList operator()(const RainbowEffect &e) const
    {
        return { brightness(e.brightness), speed(e.speed), enumName(e.direction) };
    }
    template <LightMode Mode>
    QStringList operator()(const ColorAnimatedEffect<Mode> &e) const
    {
        return { brightness(e.brightness), speed(e.speed), useColor(e.useColor, e.color) };
    }
    QStringList operator()(const CustomEffect &e) const { return { brightness(e.brightness), color(e.color) }; }

    static QString brightness(Brightness b) { return QStringLiteral("brightness %1").arg(b.value()); }
    static QString speed(Speed s) { return QStringLiteral("speed %1").arg(s.value()); }
    static QString useColor(bool use, Rgb rgb) { return use ? color(rgb) : QStringLiteral("own colors"); }
};

} // namespace

QString key(int key_value)
{
    if (key_value >= 0x04 && key_value <= 0x1D)
        return QString(QChar('A' + key_value - 0x04));
    if (key_value >= 0x1E && key_value <= 0x26)
        return QString(QChar('1' + key_value - 0x1E));
    if (key_value == 0x27)
        return QStringLiteral("0");
    if (key_value >= 0x28 && key_value <= 0x65)
        return QString::fromLatin1(kUsageNames[key_value - 0x28]);
    if (std::optional<Modifier> modifier = enumFromValue<Modifier>(key_value); modifier && *modifier != Modifier::None)
        return enumName(*modifier);
    if (std::optional<KnobAction> knob = enumFromValue<KnobAction>(key_value))
        return QStringLiteral("knob ") + enumName(*knob);

    switch (key_value)
    {
    case 0x87:  return QStringLiteral("Ro");
    case 0x88:  return QStringLiteral("Katakana/Hiragana");
    case 0x89:  return QStringLiteral("Yen");
    case 0x8A:  return QStringLiteral("Henkan");
    case 0x8B:  return QStringLiteral("Muhenkan");
    case 0xFE:  return QStringLiteral("Fn");
    default:    return hexName(key_value);
    }
}

QString action(const KeyAction &action)
{
    struct Visitor
    {
        QString operator()(DefaultAction) const { return QStringLiteral("Default"); }
        QString operator()(DisableAction) const { return QStringLiteral("Disabled"); }
        QString operator()(const ComboAction &combo) const
        {
            QStringList parts;
            if (combo.first != Modifier::None)
                parts.append(enumName(combo.first));
            if (combo.second != Modifier::None)
                parts.append(enumName(combo.second));
            parts.append(key(combo.target.value()));
            return parts.join('+');
        }
        QString operator()(MouseAction mouse) const { return QStringLiteral("Mouse ") + enumName(mouse); }
        QString operator()(MediaAction media) const { return QStringLiteral("Media ") + enumName(media); }
    };
    return std::visit(Visitor {}, action);
}

QString lighting(const LightingEffect &effect)
{
    return enumName(lightMode(effect)) + " (" + std::visit(LightingSettings {}, effect).join(", ") + ")";
}

QString seconds(SleepSeconds time)
{
    return time.value() == 0 ? QStringLiteral("never") : QStringLiteral("%1 s").arg(time.value());
}

} // namespace drevo::describe
