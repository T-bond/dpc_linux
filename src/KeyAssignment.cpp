#include "KeyAssignment.h"

#include "DeviceManager.h"
#include "QtKeyBoardKey.h"

namespace
{

QVariantMap treeItem(const QString &text, const QString &icon, int type, const QVariantList &children = QVariantList())
{
    return {
        { "text", text },
        { "icon", icon.isEmpty() ? QString() : QString("qrc:/image/icon/%1.png").arg(icon) },
        { "type", type },
        { "children", children },
    };
}

QVariantMap childItem(const QString &text, int value)
{
    return { { "text", text }, { "value", value } };
}

QVariantList toVariantList(const QVector<KeyInfo> &keys)
{
    QVariantList list;
    for (const KeyInfo &key : keys)
        list.append(QVariantMap { { "text", key.ket_text }, { "value", key.key_value } });
    return list;
}

} // namespace

KeyAssignment::KeyAssignment(QObject *parent)
    : QObject(parent)
{
    m_knob = false;
}

void KeyAssignment::setKnob(bool knob)
{
    if (m_knob == knob)
        return;
    m_knob = knob;
    emit knobChanged();
}

// functions that can be assigned
QVariantList KeyAssignment::functionTree() const
{
    QVariantList mouse = {
        childItem(tr("Left Click"), KEY_MOUSE_LEFT),
        childItem(tr("Middle Click"), KEY_MOUSE_MIDDLE),
        childItem(tr("Right Click"), KEY_MOUSE_RIGHT),
        childItem(tr("Scroll Up"), KEY_MOUSE_SCROLLUP),
        childItem(tr("Scroll Down"), KEY_MOUSE_SCROLLDOWN),
        childItem(tr("Mouse Button 4"), KEY_MOUSE_BUTTON4),
        childItem(tr("Mouse Button 5"), KEY_MOUSE_BUTTON5),
    };
    QVariantList media = {
        childItem(tr("Play/Pause"), KEY_MEDIA_PLAYPAUSE),
        childItem(tr("Stop"), KEY_MEDIA_STOP),
        childItem(tr("Prev"), KEY_MEDIA_PREVIOUS),
        childItem(tr("Next"), KEY_MEDIA_NEXT),
        childItem(tr("Volume +"), KEY_MEDIA_VOL_UP),
        // quirk (kept from the widget UI): the knob maps "Volume -" to volume up
        childItem(tr("Volume -"), m_knob ? KEY_MEDIA_VOL_UP : KEY_MEDIA_VOL_DOWN),
        childItem(tr("Mute"), KEY_MEDIA_VOL_SILENT),
    };
    QVariantList linux_keys = {
        childItem(tr("Terminal"), KEY_LINUX_TERMINAL),
        childItem(tr("Copy"), KEY_LINUX_COPY),
        childItem(tr("Paste"), KEY_LINUX_PASTE),
        childItem(tr("Cut"), KEY_LINUX_CUT),
    };

    return {
        treeItem(tr("Default"), "icon_custom_default", KEY_REALVALUE),
        treeItem(tr("Disable"), "icon_custom_disable", KEY_DISABLE),
        treeItem(tr("Keyboard function"), "icon_custom_keyboard", KEY_REDEFINEVALUE),
        treeItem(tr("Mouse function"), "icon_custom_mouse", KEY_MOUSE, mouse),
        treeItem(tr("Multi-media"), "icon_custom_multimedia", KEY_MULTIMEDIA, media),
        treeItem(tr("Linux shortcuts"), "icon_custom_linux", KEY_LINUX, linux_keys),
    };
}

QVariantList KeyAssignment::sysKeys() const
{
    QVector<KeyInfo> keys;
    QtKeyBoardKey::getSysKeyList(keys);
    return toVariantList(keys);
}

QVariantList KeyAssignment::allKeys() const
{
    QVector<KeyInfo> keys;
    QtKeyBoardKey::getAllKeyList(keys);
    return toVariantList(keys);
}

// key values with a stored assignment
QVariantList KeyAssignment::assignedKeys() const
{
    DeviceManager *device = DeviceManager::instance();

    QVector<KeyData*> vec_data;
    device->db()->queryKeyProfileInfo(device->currentProfile(), vec_data);

    QVariantList keys;
    for (KeyData *data : vec_data)
    {
        if (data)
            keys.append(data->key_value);
        delete data;
    }
    return keys;
}

// current assignment of a key
QVariantMap KeyAssignment::describe(int key_value, const QString &key_name) const
{
    DeviceManager *device = DeviceManager::instance();

    QString function = tr("Keyboard");
    QString macro = key_name;

    KeyData data;
    if (device->db()->queryKeyProfileInfo(device->currentProfile(), key_value, data))
    {
        switch (data.macro_type)
        {
        case KEY_REALVALUE:     function = tr("Keyboard");                                      break;
        case KEY_DISABLE:       function = tr("Disable");           macro = tr("Disable");      break;
        case KEY_REDEFINEVALUE: function = tr("Keyboard function"); macro = data.macro_name;    break;
        case KEY_MACRO:         function = tr("Macro");             macro = data.macro_name;    break;
        case KEY_MOUSE:         function = tr("Mouse function");    macro = data.macro_name;    break;
        case KEY_MULTIMEDIA:    function = tr("Multi-media");       macro = data.macro_name;    break;
        case KEY_LINUX:         function = tr("Linux shortcuts");   macro = data.macro_name;    break;
        default:                                                                        break;
        }
    }

    return { { "function", function }, { "macro", macro } };
}

// assign a function to a key
KeyAssignment::SaveResult KeyAssignment::save(int key_value, int macro_type, int macro_value, const QString &macro_desc,
                                              int new_key_index, int syskey1_index, int syskey2_index)
{
    if (key_value <= 0)
        return NoKeySelected;

    DeviceManager *device = DeviceManager::instance();

    KeyData data = {};
    data.profile = device->currentProfile();
    data.key_value = key_value;
    data.macro_type = macro_type;
    data.macro_name = macro_desc;

    if (macro_type == KEY_REALVALUE)
        data.macro_value = 0;
    else if (macro_type == KEY_DISABLE)
        data.macro_value = 1;
    else if (macro_type == KEY_REDEFINEVALUE)
    {
        QVector<KeyInfo> sys_keys;
        QtKeyBoardKey::getSysKeyList(sys_keys);
        QVector<KeyInfo> all_keys;
        QtKeyBoardKey::getAllKeyList(all_keys);

        if (new_key_index <= 0 || new_key_index >= all_keys.size())
            return NotSaved;
        if (syskey1_index < 0 || syskey1_index >= sys_keys.size() || syskey2_index < 0 || syskey2_index >= sys_keys.size())
            return NotSaved;

        data.macro_value = all_keys.at(new_key_index).key_value;
        QString key_name = all_keys.at(new_key_index).ket_text;
        data.macro_value1 = sys_keys.at(syskey1_index).key_value;
        QString syskey1_name = sys_keys.at(syskey1_index).ket_text;
        data.macro_value2 = sys_keys.at(syskey2_index).key_value;
        QString syskey2_name = sys_keys.at(syskey2_index).ket_text;

        // quirk (kept from the widget UI): any first modifier names both modifiers
        if (syskey1_index != 0)
            data.macro_name = syskey1_name + "+" + syskey2_name + "+" + key_name;
        else if (syskey2_index == 0)
            data.macro_name = key_name;
    }
    else if (macro_type == KEY_MOUSE)
    {
        switch (macro_value)
        {
        case KEY_MOUSE_LEFT:        data.macro_value = int(drevo::MouseAction::LeftClick);     break;
        case KEY_MOUSE_MIDDLE:      data.macro_value = int(drevo::MouseAction::MiddleClick);   break;
        case KEY_MOUSE_RIGHT:       data.macro_value = int(drevo::MouseAction::RightClick);    break;
        case KEY_MOUSE_SCROLLUP:    data.macro_value = int(drevo::MouseAction::Scroll);        break;
        case KEY_MOUSE_SCROLLDOWN:  data.macro_value = int(drevo::MouseAction::Scroll);        break;
        case KEY_MOUSE_BUTTON4:     data.macro_value = int(drevo::MouseAction::Button4);       break;
        case KEY_MOUSE_BUTTON5:     data.macro_value = int(drevo::MouseAction::Button5);       break;
        default:                                                                        break;
        }
    }
    else if (macro_type == KEY_MULTIMEDIA)
    {
        switch (macro_value)
        {
        case KEY_MEDIA_PLAYPAUSE:   data.macro_value = int(drevo::MediaAction::PlayPause);     break;
        case KEY_MEDIA_STOP:        data.macro_value = int(drevo::MediaAction::Stop);          break;
        case KEY_MEDIA_PREVIOUS:    data.macro_value = int(drevo::MediaAction::Previous);      break;
        case KEY_MEDIA_NEXT:        data.macro_value = int(drevo::MediaAction::Next);          break;
        case KEY_MEDIA_VOL_SILENT:  data.macro_value = int(drevo::MediaAction::Mute);          break;
        case KEY_MEDIA_VOL_UP:      data.macro_value = int(drevo::MediaAction::VolumeUp);      break;
        case KEY_MEDIA_VOL_DOWN:    data.macro_value = int(drevo::MediaAction::VolumeDown);    break;
        default:                                                                        break;
        }
    }
    else if (macro_type == KEY_LINUX)
    {
        switch (macro_value)
        {
        case KEY_LINUX_TERMINAL:
            data.macro_value1 = int(drevo::Modifier::LeftCtrl);
            data.macro_value2 = int(drevo::Modifier::LeftAlt);
            data.macro_value = 0x17;     // T
            break;
        case KEY_LINUX_COPY:
            data.macro_value1 = int(drevo::Modifier::LeftCtrl);
            data.macro_value = 0x06;     // C
            break;
        case KEY_LINUX_PASTE:
            data.macro_value1 = int(drevo::Modifier::LeftCtrl);
            data.macro_value = 0x19;     // V
            break;
        case KEY_LINUX_CUT:
            data.macro_value1 = int(drevo::Modifier::LeftCtrl);
            data.macro_value = 0x1B;     // X
            break;
        default:
            break;
        }
    }

    if (!device->db()->addKeyProfile(data))
        return Unassigned;

    // write data to keyboard
    device->writeKeyData(data);

    return data.macro_type == KEY_REALVALUE ? Unassigned : Assigned;
}

// restore the default function of a key
KeyAssignment::SaveResult KeyAssignment::restoreDefault(int key_value)
{
    if (key_value <= 0)
        return NoKeySelected;

    DeviceManager *device = DeviceManager::instance();

    KeyData data = {};
    data.profile = device->currentProfile();
    data.key_value = key_value;
    data.macro_type = KEY_REALVALUE;

    if (device->db()->addKeyProfile(data))
        device->writeKeyDefault(key_value);
    return Unassigned;
}
