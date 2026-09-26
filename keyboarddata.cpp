// Keyboard HID packet encoders.
//
// Clean-room reimplementation of the former prebuilt lib/libhidkeyboard.a
// (keyboarddata.o, GCC 7.4 / Qt 5.9). Every function produces byte-identical
// output to the original, including which bytes of the caller's buffer are
// left untouched, and including a few quirks that are marked "quirk" below.
//
// Packet formats (sent through DeviceComm::setDeviceData / setDeviceDatas):
//
//   8-byte commands, header 0x05:
//     05 F7 rate                       report rate
//     05 F6                            reset keyboard
//     05 FE 0E 01 tH tL                USB mode sleep time (big endian)
//     05 FE 0E 02 bH bL wH wL          wireless mode backlight / wireless sleep time
//     05 FE mode p0 p1 p2 p3 00        lighting mode, see hid_getRadiData()
//
//   256-byte key programming packets, header FF 00:
//     FF 00 aH aL <payload...>         aH/aL = flash address of the key,
//                                      see getKeyFlashAddr()
//
//   408-byte per-key RGB packet:
//     F3 00 00 layout {R G B} * n      one RGB triple per key index,
//                                      followed by the side light bars

#include "keyboarddata.h"

#include <cstring>
#include <utility>

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

// keyboard layouts (see kb_getKeyIndex)
const int KB_LAYOUT_87 = 1;
const int KB_LAYOUT_88 = 2;
const int KB_LAYOUT_91 = 4;

// lighting modes (see RadiLightWidget)
const int LM_STATIC         = 1;
const int LM_SPECTRUM       = 2;
const int LM_RAINBOW        = 3;
const int LM_POWER_GAUGE    = 4;
const int LM_BREATHING      = 5;
const int LM_TWINKLING      = 6;
const int LM_REACTIVE       = 7;
const int LM_MARQUEE        = 8;
const int LM_AURORA         = 9;
const int LM_CUSTOM         = 12;

// side light bars (key values above the regular keys)
const int KV_LIGHT_LEFT     = 600;
const int KV_LIGHT_TOP      = 601;
const int KV_LIGHT_RIGHT    = 602;
const int KV_LIGHT_BOTTOM   = 603;

const size_t KEY_PACKET_SIZE = 256;

// Encode a macro delay as one byte:
//   bit 7    : type of the preceding event (0 = press, 1 = release)
//   bits 6-5 : time unit
//   bits 4-0 : count
// quirk: the count is added, not masked, so it may carry into the unit bits.
uint8_t getDelayTime(int prev_type, int delay_ms)
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

    return (uint8_t)(((prev_type & 0xFF) << 7) + (unit << 5) + count);
}

// Flash address of a key: (profile * keys_per_profile + key_index) * 16,
// as a big-endian 16-bit value. Both bytes stay 0 for an unknown key.
void getKeyFlashAddr(int key_value, int kb_profile, int kb_layout, uint8_t &addr_hi, uint8_t &addr_lo)
{
    addr_hi = 0;
    addr_lo = 0;

    int index = kb_getKeyIndex(key_value, kb_layout);
    if (index < 0)
        return;

    int keys_per_profile = sizeof(kLayout87) / sizeof(kLayout87[0]);
    if (kb_layout == KB_LAYOUT_88)
        keys_per_profile = sizeof(kLayout88) / sizeof(kLayout88[0]);
    else if (kb_layout == KB_LAYOUT_91)
        keys_per_profile = sizeof(kLayout91) / sizeof(kLayout91[0]);

    int slot = kb_profile * keys_per_profile + index;
    addr_hi = (uint8_t)(slot / 16);
    addr_lo = (uint8_t)((unsigned)slot << 4);
}

// Clear a 256-byte key packet and write its header and key address.
void initKeyPacket(int src_key, int kb_layout, int kb_profile, uint8_t *kb_data)
{
    memset(kb_data, 0, KEY_PACKET_SIZE);
    kb_data[0] = 0xFF;
    kb_data[1] = 0x00;

    uint8_t addr_hi = 0;
    uint8_t addr_lo = 0;
    getKeyFlashAddr(src_key, kb_profile, kb_layout, addr_hi, addr_lo);
    kb_data[2] = addr_hi;
    kb_data[3] = addr_lo;
}

void setKeyRGB(uint8_t *kb_data, int offset, const RGBData *rgb)
{
    kb_data[offset]     = (uint8_t)rgb->r_value;
    kb_data[offset + 1] = (uint8_t)rgb->g_value;
    kb_data[offset + 2] = (uint8_t)rgb->b_value;
}

} // namespace

// key remap
void hid_keyRemapData(int src_key, int dst_key, int kb_layout, uint8_t* kb_data, int kb_profile)
{
    initKeyPacket(src_key, kb_layout, kb_profile, kb_data);

    kb_data[4] = 0x00;
    kb_data[5] = (uint8_t)dst_key;
    kb_data[6] = 0x00;
    kb_data[7] = (uint8_t)dst_key;
    kb_data[8] = 0x00;
}

// combo key: press/release events, 0x01 = press, 0x81 = release
void hid_combokeyData(int src_key, int dst_syskey1, int dst_syskey2, int dst_key, int kb_layout, uint8_t* kb_data, int kb_profile)
{
    initKeyPacket(src_key, kb_layout, kb_profile, kb_data);

    kb_data[4] = 0x01;
    if (dst_syskey1 == 0 && dst_syskey2 == 0)
    {
        kb_data[5] = (uint8_t)dst_key;
        kb_data[6] = 0x01;
        kb_data[7] = (uint8_t)dst_key;
        kb_data[8] = 0x81;
    }
    else if (dst_syskey1 != 0 && dst_syskey2 == 0)
    {
        // quirk: dst_key is pressed twice and never released
        kb_data[5]  = (uint8_t)dst_syskey1;
        kb_data[6]  = 0x01;
        kb_data[7]  = (uint8_t)dst_key;
        kb_data[8]  = 0x01;
        kb_data[9]  = (uint8_t)dst_key;
        kb_data[10] = 0x81;
        kb_data[11] = (uint8_t)dst_syskey1;
        kb_data[12] = 0x81;
    }
    else if (dst_syskey1 != 0 && dst_syskey2 != 0)
    {
        kb_data[5]  = (uint8_t)dst_syskey1;
        kb_data[6]  = 0x01;
        kb_data[7]  = (uint8_t)dst_syskey2;
        kb_data[8]  = 0x01;
        kb_data[9]  = (uint8_t)dst_key;
        kb_data[10] = 0x01;
        kb_data[11] = (uint8_t)dst_key;
        kb_data[12] = 0x81;
        kb_data[13] = (uint8_t)dst_syskey1;
        kb_data[14] = 0x81;
        kb_data[15] = (uint8_t)dst_syskey2;
        kb_data[16] = 0x81;
    }
    // quirk: only dst_syskey2 set writes no events
}

// key macro: a sequence of event bytes, each press/release followed by a delay byte
void hid_keymacro(int src_key, int play_times, QVector<KeyMacro*> &vec_data, int kb_layout, uint8_t* kb_data, int kb_profile)
{
    initKeyPacket(src_key, kb_layout, kb_profile, kb_data);

    // quirk: overwritten by the first event when the macro is not empty
    kb_data[4] = (uint8_t)play_times;

    int pos = 4;
    int prev_type = 0;          // 0: press, 1: release
    bool need_delay = false;    // last event has no delay byte yet

    for (const KeyMacro *macro : std::as_const(vec_data))
    {
        if (macro == NULL)
            continue;

        int type = macro->macro_type;
        if (type == MT_DELAYTIME)
        {
            kb_data[pos++] = getDelayTime(prev_type, macro->macro_value);
            need_delay = false;
        }
        else if (type >= MT_KEY_DOWN && type <= MT_MOUSE_SCROLL_UP)
        {
            // key and mouse events alternate press/release, starting with MT_KEY_DOWN
            bool is_press = (type - MT_KEY_DOWN) % 2 == 0;

            if (need_delay)
                kb_data[pos++] = getDelayTime(prev_type, 0);
            kb_data[pos++] = (uint8_t)macro->macro_value;
            prev_type = is_press ? 0 : 1;
            need_delay = true;
        }

        if (pos > 252)
            break;
    }

    if (need_delay)
        kb_data[pos] = getDelayTime(prev_type, 0);
}

// key default value
void hid_keyDefaultValue(int src_key, int kb_layout, uint8_t* kb_data, int kb_profile)
{
    initKeyPacket(src_key, kb_layout, kb_profile, kb_data);
}

// disable key
void hid_disablekeyValue(int src_key, int kb_layout, uint8_t* kb_data, int kb_profile)
{
    initKeyPacket(src_key, kb_layout, kb_profile, kb_data);
    memset(kb_data + 4, 0xFF, KEY_PACKET_SIZE - 4);
}

// get radi-light data(kb_data size is 8)
void hid_getRadiData(RadiData data, uint8_t* kb_data)
{
    memset(kb_data, 0, 8);
    kb_data[0] = 0x05;
    kb_data[1] = 0xFE;
    kb_data[2] = (uint8_t)data.mode;

    switch (data.mode)
    {
    case LM_STATIC:
    case LM_CUSTOM:
        kb_data[3] = (uint8_t)(((unsigned)data.rgb_mode << 7) | data.rgb_brightness);
        kb_data[4] = (uint8_t)data.r_value;
        kb_data[5] = (uint8_t)data.g_value;
        kb_data[6] = (uint8_t)data.b_value;
        break;
    case LM_SPECTRUM:
    case LM_POWER_GAUGE:
    case LM_MARQUEE:
        kb_data[3] = (uint8_t)(((unsigned)data.rgb_speed << 4) | data.rgb_brightness);
        break;
    case LM_RAINBOW:
        kb_data[3] = (uint8_t)(((unsigned)data.rgb_speed << 4) | data.rgb_brightness);
        kb_data[4] = (uint8_t)data.rgb_direction;
        break;
    case LM_BREATHING:
    case LM_TWINKLING:
    case LM_REACTIVE:
    case LM_AURORA:
        kb_data[3] = (uint8_t)(((unsigned)data.rgb_speed << 4) | ((unsigned)data.rgb_mode << 7) | data.rgb_brightness);
        kb_data[4] = (uint8_t)data.r_value;
        kb_data[5] = (uint8_t)data.g_value;
        kb_data[6] = (uint8_t)data.b_value;
        break;
    default:
        break;
    }
}

// get radi-light key rgb list data
void hid_getKeyRGBData(QVector<RGBData*> &vecdata, int kb_layout, uint8_t* kb_data)
{
    kb_data[0] = 0xF3;
    kb_data[1] = 0x00;
    kb_data[2] = 0x00;
    if (kb_layout == KB_LAYOUT_87)
        kb_data[3] = 0x7F;
    else if (kb_layout == KB_LAYOUT_88)
        kb_data[3] = 0x80;
    else if (kb_layout == KB_LAYOUT_91)
        kb_data[3] = 0x83;

    // start of the side light bar data
    int bars_offset = 0x109;
    if (kb_layout == KB_LAYOUT_88)
        bars_offset = 0x10C;
    else if (kb_layout == KB_LAYOUT_91)
        bars_offset = 0x115;

    for (const RGBData *rgb : std::as_const(vecdata))
    {
        if (rgb == NULL)
            continue;

        int offset = -1;
        int leds = 0;
        if (rgb->key_value < 500)
        {
            int index = kb_getKeyIndex(rgb->key_value, kb_layout);
            if (index >= 0)
            {
                offset = 4 + index * 3;
                leds = 1;
            }
        }
        else if (rgb->key_value == KV_LIGHT_TOP)
        {
            offset = bars_offset;
            leds = 14;
        }
        else if (rgb->key_value == KV_LIGHT_RIGHT)
        {
            offset = bars_offset + 15 * 3;
            leds = 5;
        }
        else if (rgb->key_value == KV_LIGHT_BOTTOM)
        {
            offset = bars_offset + 20 * 3;
            leds = 15;
        }
        else if (rgb->key_value == KV_LIGHT_LEFT)
        {
            offset = bars_offset + 35 * 3;
            leds = 6;
        }

        for (int led = 0; led < leds; ++led)
            setKeyRGB(kb_data, offset + led * 3, rgb);
    }
}

// get keyboard report rate
void hid_getKeyboardReportRate(int value, uint8_t* kb_data)
{
    kb_data[0] = 0x05;
    kb_data[1] = 0xF7;
    kb_data[2] = (uint8_t)value;
}

// usb mode sleep time
void hid_getUSBModeSleepTime(int time, uint8_t* kb_data)
{
    kb_data[0] = 0x05;
    kb_data[1] = 0xFE;
    kb_data[2] = 0x0E;
    kb_data[3] = 0x01;
    kb_data[4] = (uint8_t)(time >> 8);
    kb_data[5] = (uint8_t)time;
}

// wireless mode sleep time
void hid_getWirelessModeSleepTime(int backlight_time, int wireless_time, uint8_t* kb_data)
{
    kb_data[0] = 0x05;
    kb_data[1] = 0xFE;
    kb_data[2] = 0x0E;
    kb_data[3] = 0x02;
    kb_data[4] = (uint8_t)(backlight_time >> 8);
    kb_data[5] = (uint8_t)backlight_time;
    kb_data[6] = (uint8_t)(wireless_time >> 8);
    kb_data[7] = (uint8_t)wireless_time;
}

// reset keyboard data
void hid_getResetKeyboard(uint8_t* kb_data)
{
    kb_data[0] = 0x05;
    kb_data[1] = 0xF6;
}

// get key index(1: 87; 2: 88; 3:89; 4:91)
int kb_getKeyIndex(int key_value, int kb_layout)
{
    switch (kb_layout)
    {
    case KB_LAYOUT_87:  return kb_get87LayoutKeyIndex(key_value);
    case KB_LAYOUT_88:  return kb_get88LayoutKeyIndex(key_value);
    case KB_LAYOUT_91:  return kb_get91LayoutKeyIndex(key_value);
    default:            return -1;
    }
}

int kb_get87LayoutKeyIndex(int key_value)
{
    return lookupKeyIndex(kLayout87, key_value);
}

int kb_get88LayoutKeyIndex(int key_value)
{
    return lookupKeyIndex(kLayout88, key_value);
}

int kb_get91LayoutKeyIndex(int key_value)
{
    return lookupKeyIndex(kLayout91, key_value);
}
