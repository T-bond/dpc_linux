// Programs one key or knob action with a raw mouse or media code, to find out on the keyboard what
// a code does. Not installed; built with the tests.
//
//   drevo_probe --key 0x1a --mouse 0xab       key W: mouse code 0xab
//   drevo_probe --knob backward --media 0xe9  knob backward: media code 0xe9
//   drevo_probe --key 0x1a --default          key W: back to its own function
//   drevo_probe --knob backward --raw "01 a8 81"   knob backward: these bytes after the packet header

#include "drevo/Keyboard.h"

#include "Protocol.h"
#include "UsbTransport.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QTextStream>

using namespace drevo;

namespace
{

int parseNumber(const QString &text, bool *ok)
{
    return text.startsWith(QLatin1String("0x")) ? text.mid(2).toInt(ok, 16) : text.toInt(ok, 0);
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream err(stderr);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Program one key of a DREVO keyboard with a raw code."));
    parser.addHelpOption();
    const QCommandLineOption key_option(QStringLiteral("key"), QStringLiteral("Key as HID usage, e.g. 0x1a (W)."), QStringLiteral("usage"));
    const QCommandLineOption knob_option(QStringLiteral("knob"), QStringLiteral("Knob action: click, doubleclick, forward, backward."), QStringLiteral("action"));
    const QCommandLineOption mouse_option(QStringLiteral("mouse"), QStringLiteral("Raw mouse code, sent like the mouse functions."), QStringLiteral("code"));
    const QCommandLineOption media_option(QStringLiteral("media"), QStringLiteral("Raw media code, sent like the media functions."), QStringLiteral("code"));
    const QCommandLineOption raw_option(QStringLiteral("raw"), QStringLiteral("Hex bytes written after the header of the key packet, e.g. \"01 a8 81\"."), QStringLiteral("bytes"));
    const QCommandLineOption default_option(QStringLiteral("default"), QStringLiteral("Restore the key's own function."));
    const QCommandLineOption profile_option(QStringLiteral("profile"), QStringLiteral("Hardware profile 1..3 (default 1)."), QStringLiteral("n"), QStringLiteral("1"));
    parser.addOptions({ key_option, knob_option, mouse_option, media_option, raw_option, default_option, profile_option });
    parser.process(app);

    std::optional<Key> key;
    bool ok = false;
    if (parser.isSet(key_option))
    {
        const int value = parseNumber(parser.value(key_option), &ok);
        key = ok ? Key::fromValue(value) : std::nullopt;
    }
    else if (parser.isSet(knob_option))
    {
        const QString action = parser.value(knob_option).toLower();
        if (action == QLatin1String("click"))            key = Key::knob(KnobAction::SingleClick);
        else if (action == QLatin1String("doubleclick")) key = Key::knob(KnobAction::DoubleClick);
        else if (action == QLatin1String("forward"))     key = Key::knob(KnobAction::Forward);
        else if (action == QLatin1String("backward"))    key = Key::knob(KnobAction::Backward);
    }
    if (!key)
    {
        err << "give a known --key or --knob\n";
        return 2;
    }

    const int profile = parser.value(profile_option).toInt(&ok);
    if (!ok || profile < 1 || profile > 3)
    {
        err << "--profile is 1..3\n";
        return 2;
    }
    QLoggingCategory::setFilterRules(QStringLiteral("drevo.packets.debug=true"));

    // raw bytes: the packet of the default function with the action bytes replaced
    if (parser.isSet(raw_option))
    {
        const QByteArray bytes = QByteArray::fromHex(parser.value(raw_option).toLatin1());
        UsbTransport transport;
        const QList<DeviceInfo> devices = transport.detect(Keyboard::supportedDevices());
        if (devices.isEmpty() || transport.open(devices.first()) != ConnectionState::Connected)
        {
            err << "no keyboard connected\n";
            return 1;
        }
        std::optional<protocol::KeyPacket> packet = protocol::encodeKeyAction(*key, DefaultAction {},
                                                                              devices.first().layout,
                                                                              HardwareProfile(profile - 1));
        if (!packet || bytes.isEmpty() || bytes.size() > int(packet->size()) - 4)
        {
            err << "the key is not on this keyboard, or the bytes are empty or too long\n";
            return 2;
        }
        std::copy(bytes.begin(), bytes.end(), packet->begin() + 4);
        const bool sent = transport.send(protocol::chunk(QByteArrayView(packet->data(), packet->size())),
                                         QStringLiteral("raw ") + bytes.toHex(' '));
        return sent ? 0 : 1;
    }

    // the codes are not checked: finding out what unknown ones do is the point
    std::optional<KeyAction> action;
    if (parser.isSet(default_option))
        action = DefaultAction {};
    else if (parser.isSet(mouse_option))
    {
        const int code = parseNumber(parser.value(mouse_option), &ok);
        if (ok && code >= 0 && code <= 0xFF)
            action = MouseAction(code);
    }
    else if (parser.isSet(media_option))
    {
        const int code = parseNumber(parser.value(media_option), &ok);
        if (ok && code >= 0 && code <= 0xFF)
            action = MediaAction(code);
    }
    if (!action)
    {
        err << "give --mouse <code>, --media <code>, --raw <bytes> or --default (codes 0..0xff)\n";
        return 2;
    }

    Keyboard keyboard;
    if (keyboard.open() != ConnectionState::Connected)
    {
        err << "no keyboard connected (state " << int(keyboard.state()) << ")\n";
        return 1;
    }
    if (!keyboard.setKeyAction(*key, *action, HardwareProfile(profile - 1)))
    {
        err << "sending failed\n";
        return 1;
    }
    return 0;
}
