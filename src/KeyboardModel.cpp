#include "KeyboardModel.h"

#include "DeviceManager.h"

#include <QPoint>
#include <QtMath>

KeyboardModel::KeyboardModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // key geometry of the connected keyboard
    int kb_layout = DeviceManager::instance() ? DeviceManager::instance()->keyboardLayout() : 1;
    if (kb_layout == 2)
        m_kbimagetype = DT_KB_PRO_88;
    else if (kb_layout == 4)
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
    };
}

// index of the key at a position of the keyboard image, -1 for none
int KeyboardModel::indexAt(qreal x, qreal y) const
{
    QPoint pos(qFloor(x), qFloor(y));
    for (int i = 0; i < m_keys.size(); i++)
    {
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

// add the key at index to the checked keys
void KeyboardModel::addCheck(int index)
{
    if (index < 0 || index >= m_keys.size())
        return;

    m_keys[index].key_check = true;
    QModelIndex changed = this->index(index);
    emit dataChanged(changed, changed, { KeyCheckedRole });
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
