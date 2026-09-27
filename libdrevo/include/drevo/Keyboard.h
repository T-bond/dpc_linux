#ifndef DREVO_KEYBOARD_H
#define DREVO_KEYBOARD_H

#include "drevo/KeyAction.h"
#include "drevo/Lighting.h"
#include "drevo/Types.h"

#include <QList>

#include <memory>
#include <optional>

namespace drevo
{

class UsbTransport;

// connection to one DREVO keyboard; the only way the application talks to it.
// Packets are logged to the "drevo.packets" category (debug level), also when not connected.
// The setters return false if the keyboard is not connected, the key is not on its layout,
// or the transfer failed.
class Keyboard
{
public:
    Keyboard();
    ~Keyboard();

    Keyboard(const Keyboard &) = delete;
    Keyboard &operator=(const Keyboard &) = delete;

    // keyboards this library can configure
    static QList<DeviceInfo> supportedDevices();
    // supported keyboards that are plugged in
    static QList<DeviceInfo> detectedDevices();

    // open the first detected keyboard
    ConnectionState open();
    ConnectionState open(const DeviceInfo &device);
    void close();

    bool isConnected() const { return m_state == ConnectionState::Connected; }
    ConnectionState state() const { return m_state; }
    // keyboard found by the last open(), also if it could not be opened
    std::optional<DeviceInfo> device() const { return m_device; }
    // layout of device(), Tkl87 if there is none
    Layout layout() const;

    // program a key or knob action (hardware profile G1)
    bool setKeyAction(Key key, const KeyAction &action);
    bool resetKey(Key key) { return setKeyAction(key, DefaultAction {}); }

    bool setLighting(const LightingEffect &effect);
    // colors of the custom light mode; keys and bars not listed are dark
    bool setLedColors(const QList<LedColor> &colors);

    bool setReportRate(ReportRate rate);
    bool setUsbSleep(SleepSeconds time);
    bool setWirelessSleep(SleepSeconds backlight, SleepSeconds wireless);
    // restore the factory settings
    bool resetToFactory();

private:
    std::unique_ptr<UsbTransport>   m_transport;
    ConnectionState                 m_state;
    std::optional<DeviceInfo>       m_device;
};

} // namespace drevo

#endif // DREVO_KEYBOARD_H
