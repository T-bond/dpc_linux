#include "KeyboardModel.h"

#include "DeviceManager.h"

#include <QPoint>
#include <QtMath>

#include "LightModes.h"

KeyboardModel::KeyboardModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // key geometry of the connected keyboard
    drevo::Layout kb_layout = DeviceManager::instance() ? DeviceManager::instance()->keyboardLayout() : drevo::Layout::Tkl87;
    if (kb_layout == drevo::Layout::Iso88)
        m_kbimagetype = DT_KB_PRO_88;
    else if (kb_layout == drevo::Layout::Jis91)
        m_kbimagetype = DT_KB_PRO_91;
    else
        m_kbimagetype = DT_KB_PRO_87;

    // init keyboard keys
    initKeyboardKey();
}

int KeyboardModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_keys.size();
}

QVariant KeyboardModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return QVariant();

    const KeyboardKey &key = m_keys.at(index.row());
    switch (role)
    {
    case KeyTextRole:       return key.key_text;
    case KeyValueRole:      return key.key_value;
    case KeyXRole:          return key.key_rect.x();
    case KeyYRole:          return key.key_rect.y();
    case KeyWidthRole:      return key.key_rect.width();
    case KeyHeightRole:     return key.key_rect.height();
    case KeyColorRole:      return key.key_color;
    case KeyCheckedRole:    return key.key_check;
    case KeySideLedRole:    return sideLedFromValue(key.key_value).has_value();
    default:                return QVariant();
    }
}

QHash<int, QByteArray> KeyboardModel::roleNames() const
{
    return {
        { KeyTextRole,      "keyText" },
        { KeyValueRole,     "keyValue" },
        { KeyXRole,         "keyX" },
        { KeyYRole,         "keyY" },
        { KeyWidthRole,     "keyWidth" },
        { KeyHeightRole,    "keyHeight" },
        { KeyColorRole,     "keyColor" },
        { KeyCheckedRole,   "keyChecked" },
        { KeySideLedRole,   "keySideLed" },
    };
}

// index of the key at a position of the keyboard image, -1 for none;
// the side LEDs around the image only with side_leds
int KeyboardModel::indexAt(qreal x, qreal y, bool side_leds) const
{
    QPoint pos(qFloor(x), qFloor(y));
    for (int i = 0; i < m_keys.size(); i++)
    {
        if (!side_leds && sideLedFromValue(m_keys.at(i).key_value))
            continue;
        if (m_keys.at(i).key_rect.contains(pos))
            return i;
    }
    return -1;
}

QString KeyboardModel::keyText(int index) const
{
    if (index < 0 || index >= m_keys.size())
        return QString();
    return m_keys.at(index).key_text;
}

int KeyboardModel::keyValue(int index) const
{
    if (index < 0 || index >= m_keys.size())
        return 0;
    return m_keys.at(index).key_value;
}

// set key check
void KeyboardModel::setKeyCheck(int key_value, bool key_check)
{
    for (int i = 0; i < m_keys.size(); i++)
    {
        if (m_keys.at(i).key_value == key_value)
        {
            m_keys[i].key_check = key_check;
            QModelIndex changed = index(i);
            emit dataChanged(changed, changed, { KeyCheckedRole });
            break;
        }
    }
}

// set check of all keys
void KeyboardModel::setAllKeyCheck(bool key_check)
{
    for (KeyboardKey &key : m_keys)
        key.key_check = key_check;
    keysChanged(KeyCheckedRole);
}

// check only the key at index
void KeyboardModel::checkOnly(int index)
{
    for (int i = 0; i < m_keys.size(); i++)
        m_keys[i].key_check = (i == index);
    keysChanged(KeyCheckedRole);
}

// check or uncheck the key at index, other keys keep their check
void KeyboardModel::setCheck(int index, bool key_check)
{
    if (index < 0 || index >= m_keys.size())
        return;

    m_keys[index].key_check = key_check;
    QModelIndex changed = this->index(index);
    emit dataChanged(changed, changed, { KeyCheckedRole });
}

bool KeyboardModel::isChecked(int index) const
{
    return index >= 0 && index < m_keys.size() && m_keys.at(index).key_check;
}

// key values of the LEDs of a side light bar (drevo::LightBar value)
QVariantList KeyboardModel::sideLedValues(int bar) const
{
    QVariantList values;
    std::optional<drevo::LightBar> light_bar = drevo::enumFromValue<drevo::LightBar>(bar);
    if (!light_bar)
        return values;
    for (int i = 0; i < drevo::ledCount(*light_bar); i++)
        values.append(sideLedValue(*drevo::SideLed::make(*light_bar, i)));
    return values;
}

// set key color
void KeyboardModel::setKeyColor(int key_value, const QColor &color)
{
    for (int i = 0; i < m_keys.size(); i++)
    {
        if (m_keys.at(i).key_value == key_value)
        {
            m_keys[i].key_color = color;
            QModelIndex changed = index(i);
            emit dataChanged(changed, changed, { KeyColorRole });
            break;
        }
    }
}

// set color of the checked keys
void KeyboardModel::setCheckKeyColor(const QColor &color)
{
    for (KeyboardKey &key : m_keys)
    {
        if (key.key_check)
            key.key_color = color;
    }
    keysChanged(KeyColorRole);
}

// set color of all keys
void KeyboardModel::setAllKeysColor(const QColor &color)
{
    for (KeyboardKey &key : m_keys)
        key.key_color = color;
    keysChanged(KeyColorRole);
}

void KeyboardModel::keysChanged(int role)
{
    if (m_keys.isEmpty())
        return;
    emit dataChanged(index(0), index(m_keys.size() - 1), { role });
}
