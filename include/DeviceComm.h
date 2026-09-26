#ifndef DEVICECOMM_H
#define DEVICECOMM_H

#include <QLoggingCategory>
#include <QString>
#include "libusb-1.0/libusb.h"

// packets sent to the keyboard, as debug messages (enabled by --debug)
Q_DECLARE_LOGGING_CATEGORY(lcPackets)

class DeviceComm
{
public:
    DeviceComm();
    ~DeviceComm();

    // find drevo device
    QString enumerateDevice();

    // get keyboard layout
    int getKeyboardLayout() const { return m_kb_layout; }

    //  set device data( length < 8)
    int setDeviceData(const unsigned char *data, size_t length);
    // set device datas ( length > 8)
    int setDeviceDatas(const unsigned char *data, size_t length);

private:

    QString IsExsitDevice(uint16_t dev_vid, uint16_t dev_pid);

    libusb_device_handle*       m_hid_device;
    bool                                       m_isconnect;
    int                                          m_interface; //default 0
    int                                          m_kb_layout;
};

#endif // DEVICECOMM_H
