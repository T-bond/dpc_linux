#include "drevo/Keyboard.h"

#include "Describe.h"
#include "Protocol.h"
#include "UsbTransport.h"

namespace drevo
{

namespace
{

const quint16 kDrevoVendorId = 0x1a2c;

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
    return UsbTransport().detect(supportedDevices());
}

QList<DeviceInfo> Keyboard::wirelessReceivers()
{
    return {
        { Model::BladeMasterPro87, Layout::Tkl87, kDrevoVendorId, 0xb31f, QStringLiteral("BladeMaster PRO 87K 2.4G receiver") },
    };
}

ConnectionState Keyboard::open()
{
    const QList<DeviceInfo> devices = m_transport->detect(supportedDevices());
    if (devices.isEmpty())
    {
        close();
        m_device.reset();
        const QList<DeviceInfo> receivers = m_transport->detect(wirelessReceivers());
        if (!receivers.isEmpty())
        {
            m_receiver = receivers.first();
            m_state = ConnectionState::ReceiverOnly;
        }
        return m_state;
    }
    return open(devices.first());
}

ConnectionState Keyboard::open(const DeviceInfo &device)
{
    m_device = device;
    m_receiver.reset();
    m_state = m_transport->open(device);
    return m_state;
}

bool Keyboard::watchDevices(std::function<void()> changed)
{
    return m_transport->watch(std::move(changed));
}

void Keyboard::close()
{
    m_transport->close();
    m_state = ConnectionState::NotFound;
    m_receiver.reset();
}

Layout Keyboard::layout() const
{
    return m_device ? m_device->layout : Layout::Tkl87;
}

QList<Key> Keyboard::keys() const
{
    QList<Key> keys;
    for (int value : protocol::keyValues(layout()))
    {
        if (std::optional<Key> key = Key::fromValue(value))
            keys.append(*key);
    }
    return keys;
}

bool Keyboard::setKeyAction(Key key, const KeyAction &action, HardwareProfile profile)
{
    std::optional<protocol::KeyPacket> packet = protocol::encodeKeyAction(key, action, layout(), profile);
    if (!packet)
        return false;
    const QString what = QStringLiteral("Set %1 %2 (%3) to %4")
        .arg(key.isKnob() ? QStringLiteral("knob action") : QStringLiteral("key"),
             describe::key(key.value()).remove(QStringLiteral("knob ")),
             describe::enumName(profile), describe::action(action));
    return m_transport->send(protocol::chunk(QByteArrayView(packet->data(), packet->size())), what);
}

bool Keyboard::setLighting(const LightingEffect &effect)
{
    return m_transport->send(protocol::encodeLighting(effect), QStringLiteral("Set lighting to ") + describe::lighting(effect));
}

bool Keyboard::setLedColors(const QList<LedColor> &colors)
{
    protocol::LedPacket packet = protocol::encodeLedColors(colors, layout());
    return m_transport->send(protocol::chunk(QByteArrayView(packet.data(), packet.size())),
                             QStringLiteral("Set custom LED colors (%1 set, the rest dark)").arg(colors.size()));
}

bool Keyboard::setReportRate(ReportRate rate)
{
    return m_transport->send(protocol::encodeReportRate(rate),
                             QStringLiteral("Set report rate to %1 ms").arg(int(rate)));
}

bool Keyboard::setUsbSleep(SleepSeconds time)
{
    return m_transport->send(protocol::encodeUsbSleep(time),
                             QStringLiteral("Set USB sleep time to ") + describe::seconds(time));
}

bool Keyboard::setWirelessSleep(SleepSeconds backlight, SleepSeconds wireless)
{
    return m_transport->send(protocol::encodeWirelessSleep(backlight, wireless),
                             QStringLiteral("Set wireless sleep times: backlight %1, wireless %2")
                                 .arg(describe::seconds(backlight), describe::seconds(wireless)));
}

bool Keyboard::resetToFactory()
{
    return m_transport->send(protocol::encodeReset(), QStringLiteral("Restore factory settings"));
}

} // namespace drevo
