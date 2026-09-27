#include "UsbTransport.h"

#include "drevo/Keyboard.h"

#include <QDebug>

#include <libusb-1.0/libusb.h>

namespace drevo
{

namespace
{

// the supported keyboard with these ids
std::optional<DeviceInfo> findSupported(quint16 vendor_id, quint16 product_id)
{
    for (const DeviceInfo &info : Keyboard::supportedDevices())
    {
        if (info.vendorId == vendor_id && info.productId == product_id)
            return info;
    }
    return std::nullopt;
}

} // namespace

UsbTransport::UsbTransport()
{
    m_context = nullptr;
    m_handle = nullptr;
    m_interface = 0;
    if (libusb_init(&m_context) < 0)
    {
        qWarning() << "libusb_init failed";
        m_context = nullptr;
    }
}

UsbTransport::~UsbTransport()
{
    close();
    if (m_context)
        libusb_exit(m_context);
}

QList<DeviceInfo> UsbTransport::detect()
{
    QList<DeviceInfo> devices;
    if (!m_context)
        return devices;

    libusb_device **dev_list = nullptr;
    ssize_t num_devs = libusb_get_device_list(m_context, &dev_list);
    for (ssize_t i = 0; i < num_devs; i++)
    {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev_list[i], &desc) < 0)
            continue;
        if (std::optional<DeviceInfo> info = findSupported(desc.idVendor, desc.idProduct))
            devices.append(*info);
    }
    if (num_devs >= 0)
        libusb_free_device_list(dev_list, 1);
    return devices;
}

ConnectionState UsbTransport::open(const DeviceInfo &device)
{
    close();
    if (!m_context)
        return ConnectionState::NotFound;

    libusb_device **dev_list = nullptr;
    ssize_t num_devs = libusb_get_device_list(m_context, &dev_list);
    if (num_devs < 0)
        return ConnectionState::NotFound;

    ConnectionState state = ConnectionState::NotFound;
    for (ssize_t i = 0; i < num_devs; i++)
    {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev_list[i], &desc) < 0
            || desc.idVendor != device.vendorId || desc.idProduct != device.productId)
            continue;

        if (libusb_open(dev_list[i], &m_handle) < 0 || !m_handle)
        {
            m_handle = nullptr;
            state = ConnectionState::NoAccess;
            break;
        }

        if (libusb_kernel_driver_active(m_handle, m_interface) == 1
            && libusb_detach_kernel_driver(m_handle, m_interface) < 0)
        {
            qWarning() << "Unable to detach Kernel Driver";
            state = ConnectionState::DetachFailed;
        }
        else if (int res = libusb_claim_interface(m_handle, m_interface); res < 0)
        {
            qWarning() << "can't claim interface" << m_interface << res;
            state = res == LIBUSB_ERROR_BUSY ? ConnectionState::Busy : ConnectionState::ClaimFailed;
        }
        else
        {
            state = ConnectionState::Connected;
        }

        if (state != ConnectionState::Connected)
        {
            libusb_close(m_handle);
            m_handle = nullptr;
        }
        break;
    }
    libusb_free_device_list(dev_list, 1);
    return state;
}

void UsbTransport::close()
{
    if (!m_handle)
        return;
    libusb_release_interface(m_handle, m_interface);
    libusb_close(m_handle);
    m_handle = nullptr;
}

bool UsbTransport::send(const Report &report)
{
    const QByteArray hex = QByteArray::fromRawData(reinterpret_cast<const char *>(report.data()), report.size()).toHex(' ');

    if (!m_handle)
    {
        qCDebug(lcPackets).noquote() << hex << "(not connected)";
        return false;
    }

    // a report id of 0 is not sent
    const int report_id = report[0];
    const int skip = report_id == 0 ? 1 : 0;
    Report data = report;

    int res = libusb_control_transfer(m_handle,
        LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
        0x09/*HID set_report*/,
        (3/*HID feature*/ << 8) | report_id,
        0,
        data.data() + skip, quint16(data.size() - skip),
        1000/*timeout millis*/);

    qCDebug(lcPackets).noquote() << hex << (res < 0 ? "(failed)" : "");
    return res >= 0;
}

bool UsbTransport::send(const QList<Report> &reports)
{
    bool ok = true;
    for (const Report &report : reports)
        ok = send(report) && ok;
    return ok;
}

} // namespace drevo
