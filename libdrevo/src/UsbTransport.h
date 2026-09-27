#ifndef DREVO_USBTRANSPORT_H
#define DREVO_USBTRANSPORT_H

#include "drevo/Types.h"
#include "Protocol.h"

#include <QList>

struct libusb_context;
struct libusb_device_handle;

namespace drevo
{

// libusb access to the keyboard: enumeration and HID feature reports
class UsbTransport
{
public:
    UsbTransport();
    ~UsbTransport();

    // supported keyboards that are plugged in
    QList<DeviceInfo> detect();

    // open and claim the interface of a keyboard
    ConnectionState open(const DeviceInfo &device);
    void close();

    // send one 8-byte report
    bool send(const Report &report);
    // send reports in order; true if all were sent
    bool send(const QList<Report> &reports);

private:
    libusb_context          *m_context;
    libusb_device_handle    *m_handle;
    int                     m_interface;
};

} // namespace drevo

#endif // DREVO_USBTRANSPORT_H
