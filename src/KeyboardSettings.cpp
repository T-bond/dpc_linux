#include "KeyboardSettings.h"

#include "DeviceManager.h"

#include <QDebug>

KeyboardSettings::KeyboardSettings(QObject *parent)
    : QObject(parent)
{
    m_current_profile = DeviceManager::instance()->currentProfile();
    m_report_rate = 0;
    m_usb_sleep = 0;
    m_backlight_sleep = 0;
    m_wireless_sleep = 0;

    connect(DeviceManager::instance(), &DeviceManager::currentProfileChanged, this, [this]() {
        m_current_profile = DeviceManager::instance()->currentProfile();
        loadConfigData();
    });
}

void KeyboardSettings::componentComplete()
{
    // load config data
    loadConfigData();
}

void KeyboardSettings::setReportRate(int value)
{
    m_report_rate = value;
    emit reportRateChanged();

    updateConfigValue("report_rate", value);
    sendReportRate();
}

void KeyboardSettings::setUsbSleep(int value)
{
    if (m_usb_sleep != value)
    {
        m_usb_sleep = value;
        emit usbSleepChanged();
    }

    updateConfigValue("delay_usbwiresleep", value);
    sendKeyboardSleepTime(false);
}

void KeyboardSettings::setBacklightSleep(int value)
{
    if (m_backlight_sleep != value)
    {
        m_backlight_sleep = value;
        emit backlightSleepChanged();
    }

    updateConfigValue("delay_closelight", value);
    sendKeyboardSleepTime(true);
}

void KeyboardSettings::setWirelessSleep(int value)
{
    if (m_wireless_sleep != value)
    {
        m_wireless_sleep = value;
        emit wirelessSleepChanged();
    }

    updateConfigValue("delay_wirelesssleep", value);
    sendKeyboardSleepTime(true);
}

// restore the keyboard to its factory settings
void KeyboardSettings::resetKeyboard()
{
    DeviceManager::instance()->keyboard()->resetToFactory();
}

// load config data
void KeyboardSettings::loadConfigData()
{
    QString value;
    m_report_rate = readConfigValue("report_rate", value);
    m_usb_sleep = readConfigValue("delay_usbwiresleep", value);
    m_backlight_sleep = readConfigValue("delay_closelight", value);
    m_wireless_sleep = readConfigValue("delay_wirelesssleep", value);

    emit reportRateChanged();
    emit usbSleepChanged();
    emit backlightSleepChanged();
    emit wirelessSleepChanged();

    sendReportRate();
    // usb-mode
    sendKeyboardSleepTime(false);
    // wireless-mode
    sendKeyboardSleepTime(true);
}

// value is kept between reads, like the original loader: a missing key reuses the previous value
int KeyboardSettings::readConfigValue(const QString &key, QString &value)
{
    DeviceManager::instance()->db()->readConfigData(m_current_profile, key, value);
    return value.toInt();
}

void KeyboardSettings::updateConfigValue(const QString &key, int value)
{
    DeviceManager::instance()->db()->updateConfigData(m_current_profile, key, QString::number(value));
}

// send keyboard report rate
void KeyboardSettings::sendReportRate()
{
    std::optional<drevo::ReportRate> rate = drevo::enumFromValue<drevo::ReportRate>(m_report_rate);
    if (!rate)
    {
        qWarning() << "invalid report rate, not sent:" << m_report_rate;
        return;
    }
    DeviceManager::instance()->keyboard()->setReportRate(*rate);
}

// send sleep time
void KeyboardSettings::sendKeyboardSleepTime(bool wireless_mode)
{
    drevo::Keyboard *keyboard = DeviceManager::instance()->keyboard();
    if (wireless_mode)
    {
        // quirk (kept from the widget UI): the backlight time sent is the USB sleep time
        keyboard->setWirelessSleep(drevo::SleepSeconds::clamped(m_usb_sleep), drevo::SleepSeconds::clamped(m_wireless_sleep));
    }
    else
    {
        keyboard->setUsbSleep(drevo::SleepSeconds::clamped(m_usb_sleep));
    }
}
