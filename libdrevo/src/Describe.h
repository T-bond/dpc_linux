#ifndef DREVO_DESCRIBE_H
#define DREVO_DESCRIBE_H

#include "drevo/KeyAction.h"
#include "drevo/Lighting.h"
#include "drevo/Types.h"

#include <QString>

// readable text for what a packet does, appended to the logged packets (not translated)
namespace drevo::describe
{

// "W", "Left Ctrl", "knob Backward", "0xNN" for unknown usages
QString key(int key_value);
// "Default", "Disabled", "Left Ctrl+A", "Mouse Left Click", "Media Volume Down"
QString action(const KeyAction &action);
// "Rainbow (brightness 10, speed 2, Right To Left)"
QString lighting(const LightingEffect &effect);
// "Left Click", "Twinkling Stars": enum key with spaces between the words
template <typename Enum>
QString enumName(Enum value);

// "300 s", "never"
QString seconds(SleepSeconds time);

} // namespace drevo::describe

#include <QMetaEnum>

template <typename Enum>
QString drevo::describe::enumName(Enum value)
{
    const char *name = QMetaEnum::fromType<Enum>().valueToKey(int(value));
    if (!name)
        return QString::number(int(value));

    QString text;
    for (const char *c = name; *c; c++)
    {
        if (c != name && QChar::isUpper(*c) && !QChar::isUpper(c[-1]))
            text.append(' ');
        text.append(QChar::fromLatin1(*c));
    }
    return text;
}

#endif // DREVO_DESCRIBE_H
