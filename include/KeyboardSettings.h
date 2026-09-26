#ifndef KEYBOARDSETTINGS_H
#define KEYBOARDSETTINGS_H

#include <QObject>
#include <QQmlParserStatus>
#include <QString>
#include <QtQml/qqmlregistration.h>

// report rate and sleep times of the keyboard
class KeyboardSettings : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    // report interval in ms: 1, 2, 4 or 8
    Q_PROPERTY(int reportRate READ reportRate WRITE setReportRate NOTIFY reportRateChanged)
    // sleep times in s, 0 = never
    Q_PROPERTY(int usbSleep READ usbSleep WRITE setUsbSleep NOTIFY usbSleepChanged)
    Q_PROPERTY(int backlightSleep READ backlightSleep WRITE setBacklightSleep NOTIFY backlightSleepChanged)
    Q_PROPERTY(int wirelessSleep READ wirelessSleep WRITE setWirelessSleep NOTIFY wirelessSleepChanged)

public:
    explicit KeyboardSettings(QObject *parent = nullptr);

    void classBegin() override {}
    void componentComplete() override;

    int reportRate() const { return m_report_rate; }
    void setReportRate(int value);

    int usbSleep() const { return m_usb_sleep; }
    void setUsbSleep(int value);

    int backlightSleep() const { return m_backlight_sleep; }
    void setBacklightSleep(int value);

    int wirelessSleep() const { return m_wireless_sleep; }
    void setWirelessSleep(int value);

    // restore the keyboard to its factory settings
    Q_INVOKABLE void resetKeyboard();

signals:
    void reportRateChanged();
    void usbSleepChanged();
    void backlightSleepChanged();
    void wirelessSleepChanged();

private:
    // load config data
    void loadConfigData();
    int readConfigValue(const QString &key, QString &value);
    void updateConfigValue(const QString &key, int value);
    // send keyboard report rate
    void sendReportRate();
    // send sleep time
    void sendKeyboardSleepTime(bool wireless_mode);

    int         m_current_profile;
    int         m_report_rate;
    int         m_usb_sleep;
    int         m_backlight_sleep;
    int         m_wireless_sleep;
};

#endif // KEYBOARDSETTINGS_H
