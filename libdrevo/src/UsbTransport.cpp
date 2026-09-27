#include "UsbTransport.h"

#include "drevo/Keyboard.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSocketNotifier>

#include <fcntl.h>
#include <linux/hidraw.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>

#include <libusb-1.0/libusb.h>

namespace drevo
{

namespace
{

// the device of the list with these ids
std::optional<DeviceInfo> find(const QList<DeviceInfo> &known, quint16 vendor_id, quint16 product_id)
{
    for (const DeviceInfo &info : known)
    {
        if (info.vendorId == vendor_id && info.productId == product_id)
            return info;
    }
    return std::nullopt;
}

// interface of the keyboard with the vendor feature report the packets are sent as; it also sends
// the keys, the media keys and the mouse functions, so the kernel driver stays attached
const int kProgrammingInterface = 0;

} // namespace

UsbTransport::UsbTransport()
{
    m_context = nullptr;
    m_fd = -1;
    m_hotplug_handle = 0;
    if (libusb_init(&m_context) < 0)
    {
        qWarning() << "libusb_init failed";
        m_context = nullptr;
    }
}

UsbTransport::~UsbTransport()
{
    close();
    if (m_context && m_hotplug_handle)
    {
        libusb_hotplug_deregister_callback(m_context, m_hotplug_handle);
        libusb_set_pollfd_notifiers(m_context, nullptr, nullptr, nullptr);
    }
    m_notifiers.clear();
    if (m_context)
        libusb_exit(m_context);
}

bool UsbTransport::watch(std::function<void()> changed)
{
    if (!m_context || m_hotplug_handle || !libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG))
        return false;

    // all DREVO devices, the callback picks the supported ones
    const quint16 vendor_id = Keyboard::supportedDevices().first().vendorId;
    int res = libusb_hotplug_register_callback(m_context,
        libusb_hotplug_event(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
        LIBUSB_HOTPLUG_NO_FLAGS, vendor_id, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
        [](libusb_context *, libusb_device *device, libusb_hotplug_event, void *user_data) {
            static_cast<UsbTransport *>(user_data)->deviceChanged(device);
            return 0; // keep the callback registered
        },
        this, &m_hotplug_handle);
    if (res != LIBUSB_SUCCESS)
    {
        qWarning() << "libusb_hotplug_register_callback failed" << res;
        m_hotplug_handle = 0;
        return false;
    }
    m_changed = std::move(changed);

    // handle the events of libusb in the Qt event loop
    if (const libusb_pollfd **fds = libusb_get_pollfds(m_context))
    {
        for (const libusb_pollfd **fd = fds; *fd; fd++)
            addNotifier((*fd)->fd, (*fd)->events);
        libusb_free_pollfds(fds);
    }
    libusb_set_pollfd_notifiers(m_context,
        [](int fd, short events, void *user_data) { static_cast<UsbTransport *>(user_data)->addNotifier(fd, events); },
        [](int fd, void *user_data) { static_cast<UsbTransport *>(user_data)->removeNotifier(fd); },
        this);
    return true;
}

void UsbTransport::deviceChanged(libusb_device *device)
{
    // called while libusb handles events, also during a transfer; opening is left to the receiver
    libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(device, &desc) == 0 && m_changed
        && (find(Keyboard::supportedDevices(), desc.idVendor, desc.idProduct)
            || find(Keyboard::wirelessReceivers(), desc.idVendor, desc.idProduct)))
        m_changed();
}

void UsbTransport::addNotifier(int fd, short events)
{
    if (events & POLLIN)
    {
        auto notifier = std::make_unique<QSocketNotifier>(fd, QSocketNotifier::Read);
        QObject::connect(notifier.get(), &QSocketNotifier::activated, notifier.get(), [this]() { handleEvents(); });
        m_notifiers.push_back(std::move(notifier));
    }
    if (events & POLLOUT)
    {
        auto notifier = std::make_unique<QSocketNotifier>(fd, QSocketNotifier::Write);
        QObject::connect(notifier.get(), &QSocketNotifier::activated, notifier.get(), [this]() { handleEvents(); });
        m_notifiers.push_back(std::move(notifier));
    }
}

void UsbTransport::removeNotifier(int fd)
{
    // deleteLater: libusb may remove the descriptor while its notifier is being handled
    for (auto it = m_notifiers.begin(); it != m_notifiers.end();)
    {
        if ((*it)->socket() == fd)
        {
            (*it)->setEnabled(false);
            (*it).release()->deleteLater();
            it = m_notifiers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void UsbTransport::handleEvents()
{
    timeval zero = { 0, 0 };
    libusb_handle_events_timeout_completed(m_context, &zero, nullptr);
}

QList<DeviceInfo> UsbTransport::detect(const QList<DeviceInfo> &known)
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
        if (std::optional<DeviceInfo> info = find(known, desc.idVendor, desc.idProduct))
            devices.append(*info);
    }
    if (num_devs >= 0)
        libusb_free_device_list(dev_list, 1);
    return devices;
}

ConnectionState UsbTransport::open(const DeviceInfo &device)
{
    close();

    const QString node = hidrawNode(device);
    if (node.isEmpty())
        return ConnectionState::NotFound;

    m_fd = ::open(QFile::encodeName(node).constData(), O_RDWR | O_CLOEXEC);
    if (m_fd < 0)
    {
        const int error = errno;
        qWarning().noquote() << "cannot open" << node << ":" << qt_error_string(error);
        return error == EACCES || error == EPERM ? ConnectionState::NoAccess : ConnectionState::OpenFailed;
    }
    return ConnectionState::Connected;
}

void UsbTransport::close()
{
    if (m_fd < 0)
        return;
    ::close(m_fd);
    m_fd = -1;
}

QString UsbTransport::hidrawNode(const DeviceInfo &device)
{
    // /sys/class/hidraw/hidrawN/device is the HID device, its parent the USB interface, whose
    // parent is the USB device
    auto read = [](const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll().trimmed() : QByteArray();
    };
    const QByteArray vendor_id = QByteArray::number(device.vendorId, 16).rightJustified(4, '0');
    const QByteArray product_id = QByteArray::number(device.productId, 16).rightJustified(4, '0');

    const QDir hidraw_dir(QStringLiteral("/sys/class/hidraw"));
    for (const QString &name : hidraw_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
    {
        const QDir interface_dir(QFileInfo(hidraw_dir.filePath(name) + QStringLiteral("/device/..")).canonicalFilePath());
        const QString usb_device = interface_dir.filePath(QStringLiteral(".."));
        if (read(interface_dir.filePath(QStringLiteral("bInterfaceNumber"))).toInt(nullptr, 16) == kProgrammingInterface
            && read(usb_device + QStringLiteral("/idVendor")).toLower() == vendor_id
            && read(usb_device + QStringLiteral("/idProduct")).toLower() == product_id)
            return QStringLiteral("/dev/") + name;
    }
    return QString();
}

bool UsbTransport::send(const Report &report, const QString &what)
{
    const bool sent = transfer(report);
    log(report, sent, what);
    return sent;
}

bool UsbTransport::send(const QList<Report> &reports, const QString &what)
{
    if (reports.size() == 1)
        return send(reports.first(), what);

    // trailing reports with only zeros (the padding of a long packet) are logged as one line
    qsizetype padding = reports.size();
    while (padding > 1 && std::all_of(reports[padding - 1].begin() + 1, reports[padding - 1].end(),
                                      [](quint8 byte) { return byte == 0; }))
        padding--;
    if (reports.size() - padding < 2)
        padding = reports.size();

    // parts of a long packet are numbered
    bool ok = true;
    for (qsizetype i = 0; i < padding; i++)
        ok = send(reports[i], QStringLiteral("%1 (%2/%3)").arg(what).arg(i + 1).arg(reports.size())) && ok;

    if (padding < reports.size())
    {
        bool padding_sent = true;
        for (qsizetype i = padding; i < reports.size(); i++)
            padding_sent = transfer(reports[i]) && padding_sent;
        log(reports[padding], padding_sent,
            QStringLiteral("%1 (%2-%3/%3, zeros)").arg(what).arg(padding + 1).arg(reports.size()));
        ok = ok && padding_sent;
    }
    return ok;
}

bool UsbTransport::transfer(const Report &report)
{
    if (m_fd < 0)
        return false;

    // HID set_report of a feature report; the first byte is the report id (0: the device has none)
    Report data = report;
    return ioctl(m_fd, HIDIOCSFEATURE(data.size()), data.data()) >= 0;
}

void UsbTransport::log(const Report &report, bool sent, const QString &what) const
{
    const QByteArray hex = QByteArray::fromRawData(reinterpret_cast<const char *>(report.data()), report.size()).toHex(' ');
    if (m_fd < 0)
        qCDebug(lcPackets).noquote() << hex << "(not connected)" << "#" << what;
    else if (!sent)
        qCDebug(lcPackets).noquote() << hex << "(failed)" << "#" << what;
    else
        qCDebug(lcPackets).noquote() << hex << "#" << what;
}

} // namespace drevo
