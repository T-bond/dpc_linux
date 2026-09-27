#ifndef DREVO_KEYACTION_H
#define DREVO_KEYACTION_H

#include "drevo/Types.h"

#include <variant>

namespace drevo
{

// the key does what is printed on it
struct DefaultAction
{
};

// the key does nothing
struct DisableAction
{
};

// the key sends another key, optionally with up to two modifiers;
// without modifiers it is a plain remap (the knob always sends it as a combo)
struct ComboAction
{
    Usage       target;
    Modifier    first = Modifier::None;
    Modifier    second = Modifier::None;
};

// what a key or knob action is programmed to do
using KeyAction = std::variant<DefaultAction, DisableAction, ComboAction, MouseAction, MediaAction>;

} // namespace drevo

#endif // DREVO_KEYACTION_H
