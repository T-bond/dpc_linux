// Keyboard HID packet encoders.
//
// Clean-room reimplementation of the former prebuilt lib/libhidkeyboard.a
// (keyboarddata.o, GCC 7.4 / Qt 5.9). Every encoder produces byte-identical
// output to the original, including a few quirks that are marked "quirk" below.
//
// Packet formats (sent as 8-byte HID feature reports, see UsbTransport):
//
//   8-byte commands, header 0x05:
//     05 F7 rate                       report rate
//     05 F6                            reset keyboard
//     05 FE 0E 01 tH tL                USB mode sleep time (big endian)
//     05 FE 0E 02 bH bL wH wL          wireless mode backlight / wireless sleep time
//     05 FE mode p0 p1 p2 p3 00        lighting mode, see encodeLighting()
//
//   long packets, split into reports by chunk():
//
//   256-byte key programming packets, header FF 00:
//     FF 00 aH aL <payload...>         aH/aL = flash address of the key,
//                                      see keyFlashAddr()
//
//   408-byte per-key RGB packet:
//     F3 00 00 layout {R G B} * n      one RGB triple per key index,
//                                      followed by the side light bars

#include "Protocol.h"

#include <cstring>

Q_LOGGING_CATEGORY(lcPackets, "drevo.packets", QtInfoMsg)

namespace drevo::protocol
{

namespace
{

struct KeyIndexEntry
{
    int     key_value;      // HID usage id (or 0x1F4..0x1F7 for the knob)
    int     index;          // physical key index on the keyboard
};

// HID usage -> key index, per keyboard layout.
// The number of entries is also the number of keys stored per profile in flash.
const KeyIndexEntry kLayout87[] = {
    {0x29,  0}, {0x3A,  1}, {0x3B,  2}, {0x3C,  3}, {0x3D,  4}, {0x3E,  5}, {0x3F,  6}, {0x40,  7},
    {0x41,  8}, {0x42,  9}, {0x43, 10}, {0x44, 11}, {0x45, 12}, {0x46, 13}, {0x47, 14}, {0x48, 15},
    {0x35, 16}, {0x1E, 17}, {0x1F, 18}, {0x20, 19}, {0x21, 20}, {0x22, 21}, {0x23, 22}, {0x24, 23},
    {0x25, 24}, {0x26, 25}, {0x27, 26}, {0x2D, 27}, {0x2E, 28}, {0x2A, 29}, {0x49, 30}, {0x4A, 31},
    {0x4B, 32}, {0x2B, 33}, {0x14, 34}, {0x1A, 35}, {0x08, 36}, {0x15, 37}, {0x17, 38}, {0x1C, 39},
    {0x18, 40}, {0x0C, 41}, {0x12, 42}, {0x13, 43}, {0x2F, 44}, {0x30, 45}, {0x31, 46}, {0x4C, 47},
    {0x4D, 48}, {0x4E, 49}, {0x39, 50}, {0x04, 51}, {0x16, 52}, {0x07, 53}, {0x09, 54}, {0x0A, 55},
    {0x0B, 56}, {0x0D, 57}, {0x0E, 58}, {0x0F, 59}, {0x33, 60}, {0x34, 61}, {0x28, 62}, {0xE1, 63},
    {0x1D, 64}, {0x1B, 65}, {0x06, 66}, {0x19, 67}, {0x05, 68}, {0x11, 69}, {0x10, 70}, {0x36, 71},
    {0x37, 72}, {0x38, 73}, {0xE5, 74}, {0x52, 75}, {0xE0, 76}, {0xE3, 77}, {0xE2, 78}, {0x2C, 79},
    {0xE6, 80}, {0xE7, 81}, {0xFE, 82}, {0xE4, 83}, {0x50, 84}, {0x51, 85}, {0x4F, 86}, {0x1F4, 87},
    {0x1F5, 88}, {0x1F6, 89}, {0x1F7, 90},
};

const KeyIndexEntry kLayout88[] = {
    {0x29,  0}, {0x3A,  1}, {0x3B,  2}, {0x3C,  3}, {0x3D,  4}, {0x3E,  5}, {0x3F,  6}, {0x40,  7},
    {0x41,  8}, {0x42,  9}, {0x43, 10}, {0x44, 11}, {0x45, 12}, {0x46, 13}, {0x47, 14}, {0x48, 15},
    {0x35, 16}, {0x1E, 17}, {0x1F, 18}, {0x20, 19}, {0x21, 20}, {0x22, 21}, {0x23, 22}, {0x24, 23},
    {0x25, 24}, {0x26, 25}, {0x27, 26}, {0x2D, 27}, {0x2E, 28}, {0x2A, 29}, {0x49, 30}, {0x4A, 31},
    {0x4B, 32}, {0x2B, 33}, {0x14, 34}, {0x1A, 35}, {0x08, 36}, {0x15, 37}, {0x17, 38}, {0x1C, 39},
    {0x18, 40}, {0x0C, 41}, {0x12, 42}, {0x13, 43}, {0x2F, 44}, {0x30, 45}, {0x28, 46}, {0x4C, 47},
    {0x4D, 48}, {0x4E, 49}, {0x39, 50}, {0x04, 51}, {0x16, 52}, {0x07, 53}, {0x09, 54}, {0x0A, 55},
    {0x0B, 56}, {0x0D, 57}, {0x0E, 58}, {0x0F, 59}, {0x33, 60}, {0x34, 61}, {0x31, 62}, {0xE1, 63},
    {0x64, 64}, {0x1D, 65}, {0x1B, 66}, {0x06, 67}, {0x19, 68}, {0x05, 69}, {0x11, 70}, {0x10, 71},
    {0x36, 72}, {0x37, 73}, {0x38, 74}, {0xE5, 75}, {0x52, 76}, {0xE0, 77}, {0xE3, 78}, {0xE2, 79},
    {0x2C, 80}, {0xE6, 81}, {0xE7, 82}, {0xFE, 83}, {0xE4, 84}, {0x50, 85}, {0x51, 86}, {0x4F, 87},
    {0x1F4, 88}, {0x1F5, 89}, {0x1F6, 90}, {0x1F7, 91},
};

const KeyIndexEntry kLayout91[] = {
    {0x29,  0}, {0x3A,  1}, {0x3B,  2}, {0x3C,  3}, {0x3D,  4}, {0x3E,  5}, {0x3F,  6}, {0x40,  7},
    {0x41,  8}, {0x42,  9}, {0x43, 10}, {0x44, 11}, {0x45, 12}, {0x46, 13}, {0x47, 14}, {0x48, 15},
    {0x35, 16}, {0x1E, 17}, {0x1F, 18}, {0x20, 19}, {0x21, 20}, {0x22, 21}, {0x23, 22}, {0x24, 23},
    {0x25, 24}, {0x26, 25}, {0x27, 26}, {0x2D, 27}, {0x2E, 28}, {0x89, 29}, {0x2A, 30}, {0x49, 31},
    {0x4A, 32}, {0x4B, 33}, {0x2B, 34}, {0x14, 35}, {0x1A, 36}, {0x08, 37}, {0x15, 38}, {0x17, 39},
    {0x1C, 40}, {0x18, 41}, {0x0C, 42}, {0x12, 43}, {0x13, 44}, {0x2F, 45}, {0x30, 46}, {0x28, 47},
    {0x4C, 48}, {0x4D, 49}, {0x4E, 50}, {0x39, 51}, {0x04, 52}, {0x16, 53}, {0x07, 54}, {0x09, 55},
    {0x0A, 56}, {0x0B, 57}, {0x0D, 58}, {0x0E, 59}, {0x0F, 60}, {0x33, 61}, {0x34, 62}, {0x31, 63},
    {0xE1, 64}, {0x1D, 65}, {0x1B, 66}, {0x06, 67}, {0x19, 68}, {0x05, 69}, {0x11, 70}, {0x10, 71},
    {0x36, 72}, {0x37, 73}, {0x38, 74}, {0x87, 75}, {0xE5, 76}, {0x52, 77}, {0xE0, 78}, {0xE3, 79},
    {0xE2, 80}, {0x8B, 81}, {0x2C, 82}, {0x8A, 83}, {0x88, 84}, {0xE6, 85}, {0xFE, 86}, {0xE4, 87},
    {0x50, 88}, {0x51, 89}, {0x4F, 90}, {0x1F4, 91}, {0x1F5, 92}, {0x1F6, 93}, {0x1F7, 94},
};

template <size_t N>
int lookupKeyIndex(const KeyIndexEntry (&table)[N], int key_value)
{
    for (const KeyIndexEntry &entry : table)
    {
        if (entry.key_value == key_value)
            return entry.index;
    }
    return -1;
}

// keys stored per profile in flash
int keysPerProfile(Layout layout)
{
    switch (layout)
    {
    case Layout::Iso88: return int(std::size(kLayout88));
    case Layout::Jis91: return int(std::size(kLayout91));
    case Layout::Tkl87: break;
    }
    return int(std::size(kLayout87));
}

// Encode a macro delay as one byte:
//   bit 7    : type of the preceding event (0 = press, 1 = release)
//   bits 6-5 : time unit
//   bits 4-0 : count
// quirk: the count is added, not masked, so it may carry into the unit bits.
quint8 delayTime(int prev_type, int delay_ms)
{
    if (delay_ms > 31000)
        delay_ms = 31000;

    int unit = 0;
    int count = 0;
    if (delay_ms >= 1 && delay_ms <= 10)
    {
        count = 1;
    }
    else if (delay_ms >= 11 && delay_ms <= 310)
    {
        count = delay_ms / 10;
    }
    else if (delay_ms >= 311 && delay_ms <= 1550)
    {
        unit = 1;
        count = delay_ms / 50;
    }
    else if (delay_ms >= 1551 && delay_ms <= 3100)
    {
        unit = 1;
        count = delay_ms / 100;
    }
    else if (delay_ms >= 3101)
    {
        unit = 3;
        count = delay_ms / 1000;
    }

    return quint8(((prev_type & 0xFF) << 7) + (unit << 5) + count);
}

// Key packet with its header and key flash address:
// (slot * keys_per_profile + key_index) * 16, as a big-endian 16-bit value.
std::optional<KeyPacket> keyPacket(Key key, Layout layout, Slot slot)
{
    int index = keyIndex(key.value(), layout);
    if (index < 0)
        return std::nullopt;

    int address = int(slot) * keysPerProfile(layout) + index;

    KeyPacket packet {};
    packet[0] = 0xFF;
    packet[1] = 0x00;
    packet[2] = quint8(address / 16);
    packet[3] = quint8(unsigned(address) << 4);
    return packet;
}

// the key sends another key code
void writeRemap(KeyPacket &packet, quint8 target)
{
    packet[4] = 0x00;
    packet[5] = target;
    packet[6] = 0x00;
    packet[7] = target;
    packet[8] = 0x00;
}

// combo key: press/release events, 0x01 = press, 0x81 = release
void writeCombo(KeyPacket &packet, quint8 modifier1, quint8 modifier2, quint8 target)
{
    packet[4] = 0x01;
    if (modifier1 == 0 && modifier2 == 0)
    {
        packet[5] = target;
        packet[6] = 0x01;
        packet[7] = target;
        packet[8] = 0x81;
    }
    else if (modifier1 != 0 && modifier2 == 0)
    {
        // quirk: the target is pressed twice and never released
        packet[5]  = modifier1;
        packet[6]  = 0x01;
        packet[7]  = target;
        packet[8]  = 0x01;
        packet[9]  = target;
        packet[10] = 0x81;
        packet[11] = modifier1;
        packet[12] = 0x81;
    }
    else if (modifier1 != 0 && modifier2 != 0)
    {
        packet[5]  = modifier1;
        packet[6]  = 0x01;
        packet[7]  = modifier2;
        packet[8]  = 0x01;
        packet[9]  = target;
        packet[10] = 0x01;
        packet[11] = target;
        packet[12] = 0x81;
        packet[13] = modifier1;
        packet[14] = 0x81;
        packet[15] = modifier2;
        packet[16] = 0x81;
    }
    // quirk: only the second modifier set writes no events
}

void setLed(LedPacket &packet, int offset, Rgb color)
{
    packet[offset]     = color.r;
    packet[offset + 1] = color.g;
    packet[offset + 2] = color.b;
}

// first slot of a side light bar in the side LED data of the RGB packet;
// the slots run around the keyboard: top 0..13, right 15..19, bottom 21..34, left 35..39.
// Slots 14, 20 and 40 have no LED (the former library wrote 20..34 and 35..40, counting 15 bottom
// and 6 left LEDs). The bottom bar is verified on the keyboard, slot 40 is assumed.
int firstSideSlot(LightBar bar)
{
    switch (bar)
    {
    case LightBar::Top:     return 0;
    case LightBar::Right:   return 15;
    case LightBar::Bottom:  return 21;
    case LightBar::Left:    return 35;
    }
    return 0;
}

// slot of one side LED; the LEDs are chained clockwise (top left to right, right top to bottom,
// bottom right to left, left bottom to top); verified for the bottom bar
int sideSlot(SideLed led)
{
    const int count = ledCount(led.bar());
    switch (led.bar())
    {
    case LightBar::Top:
    case LightBar::Right:
        return firstSideSlot(led.bar()) + led.index();
    case LightBar::Bottom:
    case LightBar::Left:
        return firstSideSlot(led.bar()) + count - 1 - led.index();
    }
    return 0;
}

Report command(std::initializer_list<quint8> bytes)
{
    Report report {};
    std::copy(bytes.begin(), bytes.end(), report.begin());
    return report;
}

} // namespace

int keyIndex(int key_value, Layout layout)
{
    switch (layout)
    {
    case Layout::Tkl87: return lookupKeyIndex(kLayout87, key_value);
    case Layout::Iso88: return lookupKeyIndex(kLayout88, key_value);
    case Layout::Jis91: return lookupKeyIndex(kLayout91, key_value);
    }
    return -1;
}

bool isKnownKey(int key_value)
{
    return keyIndex(key_value, Layout::Tkl87) >= 0
        || keyIndex(key_value, Layout::Iso88) >= 0
        || keyIndex(key_value, Layout::Jis91) >= 0;
}

std::optional<KeyPacket> encodeKeyAction(Key key, const KeyAction &action, Layout layout, Slot slot)
{
    std::optional<KeyPacket> packet = keyPacket(key, layout, slot);
    if (!packet)
        return std::nullopt;

    struct Visitor
    {
        KeyPacket  &packet;
        Key         key;

        void operator()(DefaultAction) {}
        void operator()(DisableAction) { std::memset(packet.data() + 4, 0xFF, packet.size() - 4); }
        void operator()(const ComboAction &combo)
        {
            // the knob always sends keyboard functions as combo keys
            if (!key.isKnob() && combo.first == Modifier::None && combo.second == Modifier::None)
                writeRemap(packet, combo.target.value());
            else
                writeCombo(packet, quint8(combo.first), quint8(combo.second), combo.target.value());
        }
        void operator()(MouseAction mouse) { writeCombo(packet, 0, 0, quint8(mouse)); }
        void operator()(MediaAction media) { writeRemap(packet, quint8(media)); }
    };
    std::visit(Visitor { *packet, key }, action);
    return packet;
}

// macro: a sequence of event bytes, each press/release followed by a delay byte
std::optional<KeyPacket> encodeMacro(Key key, int play_times, const QList<MacroStep> &steps, Layout layout, Slot slot)
{
    std::optional<KeyPacket> packet = keyPacket(key, layout, slot);
    if (!packet)
        return std::nullopt;
    KeyPacket &data = *packet;

    // quirk: overwritten by the first event when the macro is not empty
    data[4] = quint8(play_times);

    int pos = 4;
    int prev_type = 0;          // 0: press, 1: release
    bool need_delay = false;    // last event has no delay byte yet

    for (const MacroStep &step : steps)
    {
        if (step.type == MacroStepType::Delay)
        {
            data[pos++] = delayTime(prev_type, step.value);
            need_delay = false;
        }
        else
        {
            // key and mouse events alternate press/release, starting with KeyDown
            bool is_press = (int(step.type) - int(MacroStepType::KeyDown)) % 2 == 0;

            if (need_delay)
                data[pos++] = delayTime(prev_type, 0);
            data[pos++] = quint8(step.value);
            prev_type = is_press ? 0 : 1;
            need_delay = true;
        }

        if (pos > 252)
            break;
    }

    if (need_delay)
        data[pos] = delayTime(prev_type, 0);
    return packet;
}

Report encodeLighting(const LightingEffect &effect)
{
    const unsigned use_color = effect.useColor ? 1 : 0;
    const unsigned speed = unsigned(effect.speed.value());
    const unsigned brightness = unsigned(effect.brightness.value());

    Report report {};
    report[0] = 0x05;
    report[1] = 0xFE;
    report[2] = quint8(effect.mode);

    switch (effect.mode)
    {
    case LightMode::Static:
    case LightMode::Custom:
        report[3] = quint8((use_color << 7) | brightness);
        report[4] = effect.color.r;
        report[5] = effect.color.g;
        report[6] = effect.color.b;
        break;
    case LightMode::Spectrum:
    case LightMode::PowerGauge:
    case LightMode::Marquee:
        report[3] = quint8((speed << 4) | brightness);
        break;
    case LightMode::Rainbow:
        report[3] = quint8((speed << 4) | brightness);
        report[4] = quint8(effect.direction);
        break;
    case LightMode::Breathing:
    case LightMode::TwinklingStars:
    case LightMode::Reactive:
    case LightMode::Aurora:
        report[3] = quint8((speed << 4) | (use_color << 7) | brightness);
        report[4] = effect.color.r;
        report[5] = effect.color.g;
        report[6] = effect.color.b;
        break;
    }
    return report;
}

LedPacket encodeLedColors(const QList<LedColor> &colors, Layout layout)
{
    LedPacket packet {};
    packet[0] = 0xF3;
    packet[1] = 0x00;
    packet[2] = 0x00;

    // start of the side LED data, right after the keys of the layout, see firstSideSlot()
    int bars_offset = 0x109;
    switch (layout)
    {
    case Layout::Tkl87: packet[3] = 0x7F; bars_offset = 0x109; break;
    case Layout::Iso88: packet[3] = 0x80; bars_offset = 0x10C; break;
    case Layout::Jis91: packet[3] = 0x83; bars_offset = 0x115; break;
    }

    for (const LedColor &led : colors)
    {
        int offset = -1;
        int leds = 0;
        if (const Key *key = std::get_if<Key>(&led.target))
        {
            // the knob has no light
            int index = key->isKnob() ? -1 : keyIndex(key->value(), layout);
            if (index >= 0)
            {
                offset = 4 + index * 3;
                leds = 1;
            }
        }
        else if (const LightBar *bar = std::get_if<LightBar>(&led.target))
        {
            offset = bars_offset + firstSideSlot(*bar) * 3;
            leds = ledCount(*bar);
        }
        else
        {
            offset = bars_offset + sideSlot(std::get<SideLed>(led.target)) * 3;
            leds = 1;
        }

        for (int i = 0; i < leds; ++i)
            setLed(packet, offset + i * 3, led.color);
    }
    return packet;
}

Report encodeReportRate(ReportRate rate)
{
    return command({ 0x05, 0xF7, quint8(rate) });
}

Report encodeUsbSleep(SleepSeconds time)
{
    const int t = time.value();
    return command({ 0x05, 0xFE, 0x0E, 0x01, quint8(t >> 8), quint8(t) });
}

Report encodeWirelessSleep(SleepSeconds backlight, SleepSeconds wireless)
{
    const int b = backlight.value();
    const int w = wireless.value();
    return command({ 0x05, 0xFE, 0x0E, 0x02, quint8(b >> 8), quint8(b), quint8(w >> 8), quint8(w) });
}

Report encodeReset()
{
    return command({ 0x05, 0xF6 });
}

// quirk: one report per 6 bytes of the packet, so the first report's extra byte shifts the
// data: a 256-byte packet loses its last 3 bytes, a 408-byte one gets a 0 byte appended
QList<Report> chunk(QByteArrayView packet)
{
    const qsizetype steps = packet.size() / 6;
    QList<Report> reports;
    reports.reserve(steps);

    qsizetype pos = 0;
    for (qsizetype i = 0; i < steps; ++i)
    {
        Report report {};
        report[0] = 0x05;
        const int first = i == 0 ? 1 : 2;
        for (int j = first; j < int(report.size()); ++j, ++pos)
            report[j] = pos < packet.size() ? quint8(packet[pos]) : 0;
        reports.append(report);
    }
    return reports;
}

} // namespace drevo::protocol
