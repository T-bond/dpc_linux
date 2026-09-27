#ifndef DREVO_PROTOCOL_H
#define DREVO_PROTOCOL_H

#include "drevo/KeyAction.h"
#include "drevo/Lighting.h"
#include "drevo/Types.h"

#include <QByteArrayView>
#include <QList>
#include <QLoggingCategory>

#include <array>
#include <optional>

// packets sent to the keyboard, as debug messages
Q_DECLARE_LOGGING_CATEGORY(lcPackets)

// packet encoders, see Protocol.cpp for the formats
namespace drevo::protocol
{

// HID feature report, the unit of every transfer
using Report = std::array<quint8, 8>;
// key programming packet
using KeyPacket = std::array<quint8, 256>;
// per-key colors of the custom light mode
using LedPacket = std::array<quint8, 408>;

// macro steps (not used by the application yet)
enum class MacroStepType
{
    Delay = 1,      // value: ms
    KeyDown,        // value: HID usage
    KeyUp,
    MouseLeftDown,
    MouseLeftUp,
    MouseMiddleDown,
    MouseMiddleUp,
    MouseRightDown,
    MouseRightUp,
    MouseButton4Down,
    MouseButton4Up,
    MouseButton5Down,
    MouseButton5Up,
    MouseScrollDown,
    MouseScrollUp,
};

struct MacroStep
{
    MacroStepType   type;
    int             value;
};

// physical key index of a key on a layout, -1 if the layout does not have it
int keyIndex(int key_value, Layout layout);
// the key exists on at least one layout
bool isKnownKey(int key_value);
// keys and knob actions stored per hardware profile on a layout, in key index order
QList<int> keyValues(Layout layout);
// keys stored per hardware profile in flash
int keysPerProfile(Layout layout);

// empty if the key is not on the layout
std::optional<KeyPacket> encodeKeyAction(Key key, const KeyAction &action, Layout layout, HardwareProfile profile);
std::optional<KeyPacket> encodeMacro(Key key, int play_times, const QList<MacroStep> &steps, Layout layout,
                                     HardwareProfile profile);

Report encodeLighting(const LightingEffect &effect);
LedPacket encodeLedColors(const QList<LedColor> &colors, Layout layout);

Report encodeReportRate(ReportRate rate);
Report encodeUsbSleep(SleepSeconds time);
Report encodeWirelessSleep(SleepSeconds backlight, SleepSeconds wireless);
Report encodeReset();

// split a long packet into reports: 05 d0..d6, then 05 00 d7..d12, ...
QList<Report> chunk(QByteArrayView packet);

} // namespace drevo::protocol

namespace drevo
{
using protocol::Report;
}

#endif // DREVO_PROTOCOL_H
