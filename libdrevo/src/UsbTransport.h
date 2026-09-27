#ifndef DREVO_USBTRANSPORT_H
#define DREVO_USBTRANSPORT_H

#include "drevo/Types.h"
#include "Protocol.h"

#include <QList>

#include <functional>
#include <memory>
#include <vector>

class QSocketNotifier;

struct libusb_context;
struct libusb_device;

namespace drevo
{

// access to the keyboard: enumeration and plugging with libusb, HID feature reports through the
// kernel's hidraw device (the kernel driver stays attached, so media and mouse functions keep working)
class UsbTransport
{
public:
    UsbTransport();
    ~UsbTransport();

    // devices of the list that are plugged in
    QList<DeviceInfo> detect(const QList<DeviceInfo> &known);

    // call changed when a supported keyboard or its receiver is plugged in or removed, from the Qt event loop of
    // this thread; false if the platform cannot report it
    bool watch(std::function<void()> changed);

    // open the hidraw device of the keyboard's programming interface
    ConnectionState open(const DeviceInfo &device);
    void close();

    // send one 8-byte report; what it does is logged after it
    bool send(const Report &report, const QString &what);
    // send reports in order; true if all were sent
    bool send(const QList<Report> &reports, const QString &what);

private:
    libusb_context          *m_context;
    int                     m_fd;       // hidraw device, -1 if not open
    // hotplug watching: libusb reports the events while its file descriptors are handled
    int                     m_hotplug_handle;
    std::function<void()>   m_changed;
    std::vector<std::unique_ptr<QSocketNotifier>> m_notifiers;

    // a DREVO device was plugged in or removed
    void deviceChanged(libusb_device *device);
    void addNotifier(int fd, short events);
    void removeNotifier(int fd);
    void handleEvents();
    // /dev/hidrawN of the programming interface of a plugged in keyboard, empty if there is none
    static QString hidrawNode(const DeviceInfo &device);
    // send one report without logging it
    bool transfer(const Report &report);
    // log a report and what it does; marked when it was not sent
    void log(const Report &report, bool sent, const QString &what) const;
};

} // namespace drevo

#endif // DREVO_USBTRANSPORT_H
