// the encoders must produce the same packets as the former libhidkeyboard (golden.txt)

#include "Describe.h"
#include "Protocol.h"

#include "drevo/Keyboard.h"

#include <QFile>
#include <QHash>
#include <QLoggingCategory>
#include <QTest>

using namespace drevo;
using namespace drevo::protocol;

namespace
{

template <size_t N>
QByteArray hex(const std::array<quint8, N> &packet)
{
    return QByteArray(reinterpret_cast<const char *>(packet.data()), N).toHex();
}

QByteArray hex(const QList<Report> &reports)
{
    QByteArray result;
    for (const Report &report : reports)
        result += hex(report);
    return result;
}

Key key(int value)
{
    std::optional<Key> k = Key::fromValue(value);
    Q_ASSERT(k);
    return *k;
}

Usage usage(int value)
{
    std::optional<Usage> u = Usage::fromValue(value);
    Q_ASSERT(u);
    return *u;
}

QByteArray keyPacket(int src, const KeyAction &action, Layout layout)
{
    std::optional<KeyPacket> packet = encodeKeyAction(key(src), action, layout, HardwareProfile::G1);
    return packet ? hex(*packet) : QByteArray();
}

const Layout kLayouts[] = { Layout::Tkl87, Layout::Iso88, Layout::Jis91 };

} // namespace

class TestProtocol : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void keyActions();
    void hardwareProfiles();
    void ledColors();
    void sideLeds();
    void lighting();
    void commands();
    void validation();
    void descriptions();
    void packetLog();

private:
    void compare(const char *name, const QByteArray &actual);

    QHash<QByteArray, QByteArray>   m_golden;
};

void TestProtocol::initTestCase()
{
    QFile file(GOLDEN_FILE);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    while (!file.atEnd())
    {
        const QByteArray line = file.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        const QList<QByteArray> fields = line.split(' ');
        QCOMPARE(fields.size(), 2);
        m_golden.insert(fields[0], fields[1]);
    }
}

void TestProtocol::compare(const char *name, const QByteArray &actual)
{
    QVERIFY2(m_golden.contains(name), name);
    QVERIFY2(actual == m_golden.value(name), name);
}

void TestProtocol::keyActions()
{
    for (Layout layout : kLayouts)
    {
        const QByteArray l = QByteArray::number(int(layout));
        compare("remap_" + l, keyPacket(0x04, ComboAction { usage(0x05) }, layout));
        compare("combo10_" + l, keyPacket(0x04, ComboAction { usage(0x05), Modifier::LeftCtrl }, layout));
        compare("combo11_" + l, keyPacket(0x04, ComboAction { usage(0x05), Modifier::LeftCtrl, Modifier::LeftAlt }, layout));
        // only the second modifier is the same as only the first one (the original wrote no events)
        QCOMPARE(keyPacket(0x04, ComboAction { usage(0x05), Modifier::None, Modifier::LeftAlt }, layout),
                 keyPacket(0x04, ComboAction { usage(0x05), Modifier::LeftAlt }, layout));
        compare("knob_" + l, keyPacket(500, ComboAction { usage(0x05) }, layout));
        compare("knobmouse_" + l, keyPacket(0x1F7, MouseAction::LeftClick, layout));
        compare("media_" + l, keyPacket(0xE4, MediaAction::PlayPause, layout));
        compare("default_" + l, keyPacket(0x2C, DefaultAction {}, layout));
        compare("disable_" + l, keyPacket(0x2C, DisableAction {}, layout));

        std::optional<KeyPacket> disable = encodeKeyAction(key(0x2C), DisableAction {}, layout, HardwareProfile::G1);
        compare("chunks_disable_" + l, hex(chunk(QByteArrayView(disable->data(), disable->size()))));
    }

    // a mouse function on a regular key is a combo without modifiers
    std::optional<KeyPacket> mouse = encodeKeyAction(key(0x04), MouseAction::Button4, Layout::Tkl87, HardwareProfile::G1);
    QCOMPARE(hex(*mouse).mid(8, 10), QByteArray("01a901a981"));

    // a macro: play count, then each event with its delay byte, like a combo key
    // (the original overwrote the play count with the first event)
    std::optional<KeyPacket> macro = encodeMacro(key(0x04), 3,
        { { MacroStepType::KeyDown, 0x05 }, { MacroStepType::Delay, 50 }, { MacroStepType::KeyUp, 0x05 } },
        Layout::Tkl87, HardwareProfile::G1);
    QVERIFY(macro);
    QCOMPARE(hex(*macro).mid(8, 10), QByteArray("0305050580")); // 05 = press, 50 ms; 80 = release, no delay
}

// G2 and G3 use the same packets as G1 at the next blocks of keysPerProfile() keys
// (not verified on the keyboard yet, see TODO.md)
void TestProtocol::hardwareProfiles()
{
    const HardwareProfile profiles[] = { HardwareProfile::G1, HardwareProfile::G2, HardwareProfile::G3 };
    for (Layout layout : kLayouts)
    {
        const QList<int> values = keyValues(layout);
        QCOMPARE(values.size(), keysPerProfile(layout));
        QVERIFY(values.contains(500));
        for (int i = 0; i < values.size(); ++i)
            QCOMPARE(keyIndex(values.at(i), layout), i);

        for (int value : { 0x29, 0x04, 0x28, 0x1F7 })
        {
            const std::optional<KeyPacket> g1 = encodeKeyAction(key(value), DisableAction {}, layout, HardwareProfile::G1);
            QVERIFY(g1);
            for (HardwareProfile profile : profiles)
            {
                const std::optional<KeyPacket> packet = encodeKeyAction(key(value), DisableAction {}, layout, profile);
                QVERIFY(packet);
                // flash address: byte 2 = address / 16, high nibble of byte 3 = address % 16
                const int address = (*packet)[2] * 16 + ((*packet)[3] >> 4);
                QCOMPARE(address, int(profile) * keysPerProfile(layout) + keyIndex(value, layout));
                QCOMPARE((*packet)[0], quint8(0xFF));
                QVERIFY(std::equal(packet->begin() + 4, packet->end(), g1->begin() + 4));
            }
        }
    }

    // byte 2 of the address of the last key of G3 does not overflow
    const int last = 3 * keysPerProfile(Layout::Jis91) - 1;
    QVERIFY(last / 16 <= 0xFF);
}

void TestProtocol::ledColors()
{
    const QList<int> keys = { 0x29, 0x04, 0x4F, 0x31 };
    const LightBar bars[] = { LightBar::Left, LightBar::Top, LightBar::Right, LightBar::Bottom };

    QList<LedColor> colors;
    int i = 0;
    for (int value : keys)
        colors.append({ key(value), Rgb { quint8(0x10 + i), quint8(0x80 + i), quint8(0xF0 + i) } }), ++i;
    for (LightBar bar : bars)
        colors.append({ bar, Rgb { quint8(0x10 + i), quint8(0x80 + i), quint8(0xF0 + i) } }), ++i;

    for (Layout layout : kLayouts)
    {
        const QByteArray l = QByteArray::number(int(layout));
        LedPacket packet = encodeLedColors(colors, layout);
        compare("rgb_" + l, hex(packet));
        compare("chunks_rgb_" + l, hex(chunk(QByteArrayView(packet.data(), packet.size()))));
    }
}

void TestProtocol::sideLeds()
{
    QVERIFY(SideLed::make(LightBar::Top, 13));
    QVERIFY(!SideLed::make(LightBar::Top, 14));
    QVERIFY(SideLed::make(LightBar::Bottom, 13));
    QVERIFY(!SideLed::make(LightBar::Bottom, 14));
    QVERIFY(!SideLed::make(LightBar::Left, 5));
    QVERIFY(!SideLed::make(LightBar::Right, -1));

    // same bytes as the whole bar when every LED of it has the bar's color
    for (Layout layout : kLayouts)
    {
        for (LightBar bar : { LightBar::Left, LightBar::Top, LightBar::Right, LightBar::Bottom })
        {
            QList<LedColor> leds;
            for (int i = 0; i < ledCount(bar); ++i)
                leds.append({ *SideLed::make(bar, i), Rgb { 1, 2, 3 } });
            QCOMPARE(encodeLedColors(leds, layout), encodeLedColors({ { bar, Rgb { 1, 2, 3 } } }, layout));
        }
    }

    // clockwise slots: first LED of each bar (leftmost / topmost)
    const int bars_offset = 0x109;      // 87 keys
    auto slotOf = [&](LightBar bar, int index) {
        LedPacket packet = encodeLedColors({ { *SideLed::make(bar, index), Rgb { 0xAA, 0xBB, 0xCC } } }, Layout::Tkl87);
        for (int slot = 0; slot < 41; ++slot)
        {
            if (packet[bars_offset + slot * 3] == 0xAA)
                return slot;
        }
        return -1;
    };
    QCOMPARE(slotOf(LightBar::Top, 0), 0);
    QCOMPARE(slotOf(LightBar::Top, 13), 13);
    QCOMPARE(slotOf(LightBar::Right, 0), 15);
    QCOMPARE(slotOf(LightBar::Bottom, 0), 34);
    QCOMPARE(slotOf(LightBar::Bottom, 13), 21);
    QCOMPARE(slotOf(LightBar::Left, 0), 39);
    QCOMPARE(slotOf(LightBar::Left, 4), 35);
}

void TestProtocol::lighting()
{
    // the golden inputs: brightness 11, speed 3, color 12 34 56, without / with use color (_0 / _1)
    const Brightness brightness = Brightness::clamped(11);
    const Speed speed = Speed::clamped(3);
    const Rgb color { 0x12, 0x34, 0x56 };

    for (bool use_color : { false, true })
    {
        const QByteArray c = use_color ? "_1" : "_0";
        compare("light_1" + c, hex(encodeLighting(StaticEffect { brightness, color, use_color })));
        compare("light_5" + c, hex(encodeLighting(BreathingEffect { brightness, speed, color, use_color })));
        compare("light_6" + c, hex(encodeLighting(TwinklingStarsEffect { brightness, speed, color, use_color })));
        compare("light_7" + c, hex(encodeLighting(ReactiveEffect { brightness, speed, color, use_color })));
        compare("light_9" + c, hex(encodeLighting(AuroraEffect { brightness, speed, color, use_color })));

        // these modes have no color setting, the old encoder ignored it
        compare("light_2" + c, hex(encodeLighting(SpectrumEffect { brightness, speed })));
        compare("light_4" + c, hex(encodeLighting(PowerGaugeEffect { brightness, speed })));
        compare("light_8" + c, hex(encodeLighting(MarqueeEffect { brightness, speed })));
        compare("light_3" + c, hex(encodeLighting(RainbowEffect { brightness, speed, RainbowDirection::LeftToRight })));
    }
    // the custom mode has no use color setting (light_12_1 set that bit)
    compare("light_12_0", hex(encodeLighting(CustomEffect { brightness, color })));

    compare("lightsoff", hex(encodeLighting(StaticEffect::off())));
    QCOMPARE(hex(encodeLighting(StaticEffect::off())), QByteArray("05fe018000000000"));

    // the direction is the byte after speed / brightness
    QCOMPARE(hex(encodeLighting(RainbowEffect { Brightness(), Speed(), RainbowDirection::UpToDown })),
             QByteArray("05fe030003000000"));

    QCOMPARE(lightMode(LightingEffect(MarqueeEffect {})), LightMode::Marquee);
    QCOMPARE(lightMode(LightingEffect(AuroraEffect {})), LightMode::Aurora);
}

void TestProtocol::commands()
{
    compare("rate_1", hex(encodeReportRate(ReportRate::Ms1)));
    compare("rate_2", hex(encodeReportRate(ReportRate::Ms2)));
    compare("rate_4", hex(encodeReportRate(ReportRate::Ms4)));
    compare("rate_8", hex(encodeReportRate(ReportRate::Ms8)));
    compare("usbsleep", hex(encodeUsbSleep(SleepSeconds::clamped(300))));
    compare("wirelesssleep", hex(encodeWirelessSleep(SleepSeconds::clamped(300), SleepSeconds::clamped(1000))));
    compare("reset", hex(encodeReset()));
}

void TestProtocol::validation()
{
    QVERIFY(Key::fromValue(0x04));
    QVERIFY(Key::fromValue(500));
    QVERIFY(Key::fromValue(500)->isKnob());
    QVERIFY(!Key::fromValue(0));
    QVERIFY(!Key::fromValue(600));
    QVERIFY(!Usage::fromValue(0));
    QVERIFY(!Usage::fromValue(0xA5));
    QVERIFY(Usage::fromValue(0xE0));
    QVERIFY(!Brightness::make(16));
    QCOMPARE(Brightness::clamped(99).value(), 15);
    QCOMPARE(SleepSeconds::clamped(-1).value(), 0);

    // the knob gets media functions as press/release events: 01 e9 01 e9 81
    std::optional<KeyPacket> knob_media = encodeKeyAction(Key::knob(KnobAction::Backward), MediaAction::VolumeDown,
                                                          Layout::Tkl87, HardwareProfile::G1);
    QVERIFY(knob_media);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(knob_media->data()) + 4, 5).toHex(), "01e901e981");

    // scrolling: only the press (up) or only the release (down) of the wheel code
    for (auto [mouse, events] : { std::pair { MouseAction::ScrollUp, "01a801" }, std::pair { MouseAction::ScrollDown, "01a881" } })
    {
        for (Key key : { *Key::fromValue(0x1A), Key::knob(KnobAction::Backward) })
        {
            std::optional<KeyPacket> packet = encodeKeyAction(key, mouse, Layout::Tkl87, HardwareProfile::G1);
            QVERIFY(packet);
            QCOMPARE(QByteArray(reinterpret_cast<const char *>(packet->data()) + 4, 4).toHex(), QByteArray(events) + "00");
        }
    }

    // JIS keys are not on the 87 key layout
    QVERIFY(!encodeKeyAction(key(0x89), DefaultAction {}, Layout::Tkl87, HardwareProfile::G1));
    QVERIFY(encodeKeyAction(key(0x89), DefaultAction {}, Layout::Jis91, HardwareProfile::G1));
}

void TestProtocol::descriptions()
{
    namespace d = drevo::describe;
    QCOMPARE(d::key(0x1A), "W");
    QCOMPARE(d::key(0x27), "0");
    QCOMPARE(d::key(0x2C), "Space");
    QCOMPARE(d::key(0x65), "Menu");
    QCOMPARE(d::key(0xE0), "Left Ctrl");
    QCOMPARE(d::key(0xFE), "Fn");
    QCOMPARE(d::key(503), "knob Backward");
    QCOMPARE(d::key(0xA0), "0xa0");

    QCOMPARE(d::action(DefaultAction {}), "Default");
    QCOMPARE(d::action(DisableAction {}), "Disabled");
    QCOMPARE(d::action(ComboAction { *Usage::fromValue(0x04) }), "A");
    QCOMPARE(d::action(ComboAction { *Usage::fromValue(0x04), Modifier::None, Modifier::LeftShift }), "Left Shift+A");
    QCOMPARE(d::action(ComboAction { *Usage::fromValue(0x04), Modifier::LeftCtrl, Modifier::RightAlt }), "Left Ctrl+Right Alt+A");
    QCOMPARE(d::action(MouseAction::LeftClick), "Mouse Left Click");
    QCOMPARE(d::action(MediaAction::VolumeDown), "Media Volume Down");

    QCOMPARE(d::lighting(RainbowEffect { Brightness::clamped(10), Speed::clamped(2), RainbowDirection::LeftToRight }),
             "Rainbow (brightness 10, speed 2, Left To Right)");
    QCOMPARE(d::lighting(BreathingEffect { Brightness::clamped(5), Speed::clamped(1), Rgb { 255, 0, 16 }, true }),
             "Breathing (brightness 5, speed 1, #ff0010)");
    QCOMPARE(d::lighting(TwinklingStarsEffect { Brightness::clamped(5), Speed::clamped(1) }),
             "Twinkling Stars (brightness 5, speed 1, own colors)");
    QCOMPARE(d::lighting(CustomEffect { Brightness::clamped(15), Rgb { 0, 0, 0 } }), "Custom (brightness 15, #000000)");

    QCOMPARE(d::seconds(SleepSeconds::clamped(0)), "never");
    QCOMPARE(d::seconds(SleepSeconds::clamped(300)), "300 s");
}

namespace
{
QStringList g_log;
}

void TestProtocol::packetLog()
{
    QLoggingCategory::setFilterRules(QStringLiteral("drevo.packets.debug=true"));
    g_log.clear();
    QtMessageHandler previous = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &msg) {
        g_log.append(msg);
    });

    Keyboard keyboard; // not opened: the packets are logged as not sent
    keyboard.setKeyAction(key(0x1A), MouseAction::LeftClick, HardwareProfile::G2);
    keyboard.setKeyAction(Key::knob(KnobAction::Backward), MediaAction::VolumeDown, HardwareProfile::G1);
    keyboard.setLighting(CustomEffect { Brightness::clamped(15), Rgb {} });
    keyboard.resetToFactory();

    qInstallMessageHandler(previous);
    QLoggingCategory::setFilterRules(QString());

    QVERIFY(g_log.size() > 4);
    QVERIFY2(g_log.first().endsWith(QStringLiteral("(not connected) # Set key W (G2) to Mouse Left Click (1/42)")), qPrintable(g_log.first()));
    // the padding of the key packet is one line
    QVERIFY2(g_log.at(2).endsWith(QStringLiteral("05 00 00 00 00 00 00 00 (not connected) # Set key W (G2) to Mouse Left Click (3-42/42, zeros)")),
             qPrintable(g_log.at(2)));
    QVERIFY(!g_log.filter(QRegularExpression(QStringLiteral("# Set knob action Backward \\(G1\\) to Media Volume Down \\(1/\\d+\\)$"))).isEmpty());
    QVERIFY(!g_log.filter(QRegularExpression(QStringLiteral("# Set lighting to Custom \\(brightness 15, #000000\\)$"))).isEmpty());
    QVERIFY(g_log.last().endsWith(QStringLiteral("# Restore factory settings")));
}

QTEST_APPLESS_MAIN(TestProtocol)

#include "tst_protocol.moc"
