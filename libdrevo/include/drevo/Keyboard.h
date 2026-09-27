#ifndef DREVO_KEYBOARD_H
#define DREVO_KEYBOARD_H

#include "drevo/KeyAction.h"
#include "drevo/Lighting.h"
#include "drevo/Types.h"

#include <QList>

#include <functional>
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
    // 2.4G receivers of supported keyboards; they are recognized, but cannot be programmed
    static QList<DeviceInfo> wirelessReceivers();

    // open the first detected keyboard; ReceiverOnly if there is none, but a 2.4G receiver
    ConnectionState open();
    ConnectionState open(const DeviceInfo &device);
    void close();
    // call changed when a supported keyboard is plugged in or removed; needs a Qt event loop in
    // this thread. The keyboard is not reopened; changed may be called during a transfer, so it
    // should only schedule that. False if the platform cannot report it.
    bool watchDevices(std::function<void()> changed);

    bool isConnected() const { return m_state == ConnectionState::Connected; }
    ConnectionState state() const { return m_state; }
    // keyboard found by the last open(), also if it could not be opened
    std::optional<DeviceInfo> device() const { return m_device; }
    // receiver found by the last open() when no keyboard was (state() is ReceiverOnly)
    std::optional<DeviceInfo> receiver() const { return m_receiver; }
    // layout of device(), Tkl87 if there is none
    Layout layout() const;

    // keys and knob actions each hardware profile stores on the layout
    QList<Key> keys() const;
    // program a key or knob action of a hardware profile
    bool setKeyAction(Key key, const KeyAction &action, HardwareProfile profile);
    bool resetKey(Key key, HardwareProfile profile) { return setKeyAction(key, DefaultAction {}, profile); }

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
    std::optional<DeviceInfo>       m_receiver;
};

} // namespace drevo

#endif // DREVO_KEYBOARD_H
