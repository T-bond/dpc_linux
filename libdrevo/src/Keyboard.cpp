#include "drevo/Keyboard.h"

#include "Protocol.h"
#include "UsbTransport.h"

namespace drevo
{

namespace
{

const quint16 kDrevoVendorId = 0x1a2c;

// hardware profile the key assignments are written to
const protocol::Slot kSlot = protocol::Slot::G1;

} // namespace

std::optional<Key> Key::fromValue(int value)
{
    if (!protocol::isKnownKey(value))
        return std::nullopt;
    return Key(value);
}

Keyboard::Keyboard()
    : m_transport(std::make_unique<UsbTransport>())
{
    m_state = ConnectionState::NotFound;
}

Keyboard::~Keyboard() = default;

QList<DeviceInfo> Keyboard::supportedDevices()
{
    return {
        { Model::BladeMasterTE87,  Layout::Tkl87, kDrevoVendorId, 0xb51f, QStringLiteral("BladeMaster TE 87K") },
        { Model::BladeMasterTE88,  Layout::Iso88, kDrevoVendorId, 0xb58f, QStringLiteral("BladeMaster TE 88K") },
        { Model::BladeMasterTE91,  Layout::Jis91, kDrevoVendorId, 0xb5bf, QStringLiteral("BladeMaster TE 91K") },
        { Model::BladeMasterPro87, Layout::Tkl87, kDrevoVendorId, 0xb57e, QStringLiteral("BladeMaster PRO 87K") },
        { Model::BladeMasterPro88, Layout::Iso88, kDrevoVendorId, 0xb58e, QStringLiteral("BladeMaster PRO 88K") },
        { Model::BladeMasterPro91, Layout::Jis91, kDrevoVendorId, 0xb5be, QStringLiteral("BladeMaster PRO 91K") },
    };
}

QList<DeviceInfo> Keyboard::detectedDevices()
{
    return UsbTransport().detect();
}

ConnectionState Keyboard::open()
{
    const QList<DeviceInfo> devices = m_transport->detect();
    if (devices.isEmpty())
    {
        close();
        m_device.reset();
        return m_state;
    }
    return open(devices.first());
}

ConnectionState Keyboard::open(const DeviceInfo &device)
{
    m_device = device;
    m_state = m_transport->open(device);
    return m_state;
}

void Keyboard::close()
{
    m_transport->close();
    m_state = ConnectionState::NotFound;
}

Layout Keyboard::layout() const
{
    return m_device ? m_device->layout : Layout::Tkl87;
}

bool Keyboard::setKeyAction(Key key, const KeyAction &action)
{
    std::optional<protocol::KeyPacket> packet = protocol::encodeKeyAction(key, action, layout(), kSlot);
    if (!packet)
        return false;
    return m_transport->send(protocol::chunk(QByteArrayView(packet->data(), packet->size())));
}

bool Keyboard::setLighting(const LightingEffect &effect)
{
    return m_transport->send(protocol::encodeLighting(effect));
}

bool Keyboard::setLedColors(const QList<LedColor> &colors)
{
    protocol::LedPacket packet = protocol::encodeLedColors(colors, layout());
    return m_transport->send(protocol::chunk(QByteArrayView(packet.data(), packet.size())));
}

bool Keyboard::setReportRate(ReportRate rate)
{
    return m_transport->send(protocol::encodeReportRate(rate));
}

bool Keyboard::setUsbSleep(SleepSeconds time)
{
    return m_transport->send(protocol::encodeUsbSleep(time));
}

bool Keyboard::setWirelessSleep(SleepSeconds backlight, SleepSeconds wireless)
{
    return m_transport->send(protocol::encodeWirelessSleep(backlight, wireless));
}

bool Keyboard::resetToFactory()
{
    return m_transport->send(protocol::encodeReset());
}

} // namespace drevo
