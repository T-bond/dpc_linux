#ifndef KEYBOARDMODEL_H
#define KEYBOARDMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QRect>
#include <QString>
#include <QVector>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <drevo/Types.h>

// key on the keyboard image
struct KeyboardKey
{
    QString     key_text;
    int         key_value;
    QRect       key_rect;
    QColor      key_color;
    bool        key_check;
};

// keys of the keyboard image, with their geometry, color and check state
class KeyboardModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    // how the keyboard view draws and handles the keys
    enum ViewMode
    {
        LightStatic = 0,    // whole keyboard lit with one color, no interaction
        LightCustom,        // per-key and side LED colors, keys and side LEDs can be selected
                            // (Shift or Ctrl adds to the selection)
        CustomKey,          // key assignment, hover and select a single key
    };
    Q_ENUM(ViewMode)

    enum Roles
    {
        KeyTextRole = Qt::UserRole + 1,
        KeyValueRole,
        KeyXRole,
        KeyYRole,
        KeyWidthRole,
        KeyHeightRole,
        KeyColorRole,
        KeyCheckedRole,
        KeySideLedRole,     // side LED around the keyboard, not a key
    };

    explicit KeyboardModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QVector<KeyboardKey>& keys() const { return m_keys; }

    // index of the key at a position of the keyboard image, -1 for none;
    // the side LEDs around the image only with side_leds
    Q_INVOKABLE int indexAt(qreal x, qreal y, bool side_leds = false) const;
    Q_INVOKABLE QString keyText(int index) const;
    Q_INVOKABLE int keyValue(int index) const;

    // set key check
    Q_INVOKABLE void setKeyCheck(int key_value, bool key_check);
    // set check of all keys
    Q_INVOKABLE void setAllKeyCheck(bool key_check);
    // check only the key at index
    Q_INVOKABLE void checkOnly(int index);
    // check or uncheck the key at index, other keys keep their check
    Q_INVOKABLE void setCheck(int index, bool key_check);
    Q_INVOKABLE bool isChecked(int index) const;
    // key values of the LEDs of a side light bar (drevo::LightBar value)
    Q_INVOKABLE QVariantList sideLedValues(int bar) const;

    // set key color
    Q_INVOKABLE void setKeyColor(int key_value, const QColor &color);
    // set color of the checked keys
    Q_INVOKABLE void setCheckKeyColor(const QColor &color);
    // set color of all keys
    Q_INVOKABLE void setAllKeysColor(const QColor &color);

private:
    enum DeviceType
    {
        DT_KB_PRO_87 = 1,
        DT_KB_PRO_88 ,
        DT_KB_PRO_89,
        DT_KB_PRO_91,
        DT_KB_TE_87,
        DT_KB_TE_88,
        DT_KB_TE_89,
        DT_KB_TE_91,
        DT_KB_MAX,
    };

    // init keyboard keys (KeyboardModelLayout.cpp)
    void initKeyboardKey();
    // add key
    int addKeyItem(QString key_text, int key_value, QRect key_rect);
    // add the LEDs of a side light bar, dividing its rectangle
    void addSideLeds(drevo::LightBar bar, const QRect &rect);

    void keysChanged(int role);

    int                     m_kbimagetype;
    QVector<KeyboardKey>    m_keys;
};

#endif // KEYBOARDMODEL_H
