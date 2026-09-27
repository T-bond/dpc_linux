#ifndef LIGHTING_H
#define LIGHTING_H

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QQmlParserStatus>
#include <QtQml/qqmlregistration.h>

#include "DeviceDB.h"
#include "KeyboardModel.h"

#include <drevo/Types.h>

// backlight effects (radi-light) of the keyboard
class Lighting : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    // keyboard shown on the lighting page, gets the per-key colors of the custom mode
    Q_PROPERTY(KeyboardModel* keyboard READ keyboard WRITE setKeyboard NOTIFY keyboardChanged)

    // light modes: [{ text, icon }], in list order
    Q_PROPERTY(QVariantList modes READ modes CONSTANT)
    Q_PROPERTY(int modeIndex READ modeIndex NOTIFY modeChanged)
    Q_PROPERTY(bool customMode READ customMode NOTIFY modeChanged)

    Q_PROPERTY(int brightness READ brightness NOTIFY settingsChanged)
    Q_PROPERTY(int speed READ speed NOTIFY settingsChanged)
    Q_PROPERTY(bool speedEnabled READ speedEnabled NOTIFY settingsChanged)
    // color button and its "Custom color" label
    Q_PROPERTY(bool colorVisible READ colorVisible NOTIFY settingsChanged)
    // the effect uses the color (not the keyboard's default colors)
    Q_PROPERTY(bool colorEnabled READ colorEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QColor color READ color NOTIFY settingsChanged)
    // custom color switch (rgb_mode), not shown in the custom mode, which always uses colors
    Q_PROPERTY(bool customColorVisible READ customColorVisible NOTIFY settingsChanged)
    Q_PROPERTY(bool customColor READ customColor NOTIFY settingsChanged)

    // rainbow direction selection, only in the rainbow mode
    Q_PROPERTY(bool directionVisible READ directionVisible NOTIFY modeChanged)
    // rainbow directions: [{ value, text }]
    Q_PROPERTY(QVariantList directions READ directions CONSTANT)
    // drevo::RainbowDirection value
    Q_PROPERTY(int direction READ direction NOTIFY settingsChanged)

    // color of the whole keyboard (all modes except custom)
    Q_PROPERTY(QColor lightColor READ lightColor NOTIFY lightColorChanged)

public:
    explicit Lighting(QObject *parent = nullptr);

    void classBegin() override {}
    void componentComplete() override;

    KeyboardModel* keyboard() const { return m_keyboard; }
    void setKeyboard(KeyboardModel *keyboard);

    QVariantList modes() const;
    int modeIndex() const;
    bool customMode() const;

    int brightness() const { return m_brightness; }
    int speed() const { return m_speed; }
    bool speedEnabled() const { return m_speed_enabled; }
    bool colorVisible() const { return m_color_visible; }
    bool colorEnabled() const { return m_color_enabled; }
    QColor color() const { return m_color; }
    bool customColorVisible() const { return m_custom_color_visible; }
    bool customColor() const { return m_custom_color; }
    bool directionVisible() const;
    QVariantList directions() const;
    int direction() const { return int(m_direction); }
    QColor lightColor() const { return m_light_color; }

    // select the light mode at a list index
    Q_INVOKABLE void selectModeIndex(int index);
    Q_INVOKABLE void setBrightness(int value);
    Q_INVOKABLE void setSpeed(int value);
    // set the rainbow direction (a drevo::RainbowDirection value, others are ignored)
    Q_INVOKABLE void setDirection(int value);
    // set the effect color and switch custom color on, or the color of the checked keys in custom mode
    Q_INVOKABLE void setColor(const QColor &color);
    Q_INVOKABLE void setCustomColor(bool custom_color);
    // remove all custom key colors
    Q_INVOKABLE void resetAllLeds();

signals:
    void keyboardChanged();
    void modeChanged();
    void settingsChanged();
    void lightColorChanged();

private:
    // select the light mode of the current profile
    void loadProfile();
    // set backlight mode
    void setBackLightMode(int light_mode);
    // query the data of the current mode
    bool currentRadiData(RadiData &data) const;
    // store the data of the current mode and send it to the keyboard
    void updateRadiData(RadiData &data);
    // send the key colors of the custom mode to the keyboard
    void sendKeyRGBData(int radi_id, bool update_keyboard);
    void setLightColor(const QColor &color);
    // send the effect of a light mode; it ends "lights off"
    void sendEffect(const RadiData &data);

    QPointer<KeyboardModel>     m_keyboard;

    int         m_current_mode;
    int         m_mode_index;
    int         m_current_profile;

    int         m_brightness;
    int         m_speed;
    bool        m_speed_enabled;
    bool        m_color_visible;
    bool        m_color_enabled;
    QColor      m_color;
    bool        m_custom_color_visible;
    bool        m_custom_color;
    drevo::RainbowDirection m_direction;
    QColor      m_light_color;
};

#endif // LIGHTING_H
