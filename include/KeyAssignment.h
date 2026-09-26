#ifndef KEYASSIGNMENT_H
#define KEYASSIGNMENT_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include "DeviceDB.h"

// key / knob function assignment: stored in the database and written to the keyboard
class KeyAssignment : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // knob assignment (key values 500..503) instead of keyboard keys
    Q_PROPERTY(bool knob READ knob WRITE setKnob NOTIFY knobChanged)

    // functions that can be assigned: [{ text, icon, type, children: [{ text, value }] }]
    Q_PROPERTY(QVariantList functionTree READ functionTree NOTIFY knobChanged)
    // keys for the combo key selection: [{ text, value }]
    Q_PROPERTY(QVariantList sysKeys READ sysKeys CONSTANT)
    Q_PROPERTY(QVariantList allKeys READ allKeys CONSTANT)

public:
    enum SaveResult
    {
        NoKeySelected = 0,  // no key selected, nothing done
        NotSaved,           // incomplete assignment, nothing done
        Assigned,           // key has a custom function now
        Unassigned,         // key has its default function now
    };
    Q_ENUM(SaveResult)

    // function types of functionTree(), see KEY_DEFINE
    enum FunctionType
    {
        DefaultFunction = KEY_REALVALUE,
        DisableKey = KEY_DISABLE,
        KeyboardFunction = KEY_REDEFINEVALUE,
        Macro = KEY_MACRO,
        MouseFunction = KEY_MOUSE,
        MultiMedia = KEY_MULTIMEDIA,
        LinuxShortcut = KEY_LINUX,
    };
    Q_ENUM(FunctionType)

    explicit KeyAssignment(QObject *parent = nullptr);

    bool knob() const { return m_knob; }
    void setKnob(bool knob);

    QVariantList functionTree() const;
    QVariantList sysKeys() const;
    QVariantList allKeys() const;

    // key values with a stored assignment
    Q_INVOKABLE QVariantList assignedKeys() const;
    // current assignment of a key: { function, macro }
    Q_INVOKABLE QVariantMap describe(int key_value, const QString &key_name) const;

    // assign a function (macro_type / macro_value from functionTree) to a key
    Q_INVOKABLE SaveResult save(int key_value, int macro_type, int macro_value, const QString &macro_desc,
                                int new_key_index, int syskey1_index, int syskey2_index);
    // restore the default function of a key
    Q_INVOKABLE SaveResult restoreDefault(int key_value);

signals:
    void knobChanged();

private:
    bool        m_knob;
};

#endif // KEYASSIGNMENT_H
