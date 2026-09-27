// the encoders must produce the same packets as the former libhidkeyboard (golden.txt)

#include "Protocol.h"

#include <QFile>
#include <QHash>
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
    std::optional<KeyPacket> packet = encodeKeyAction(key(src), action, layout, Slot::G1);
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
    void ledColors();
    void sideLeds();
    void lighting();
    void commands();
    void validation();

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
        compare("combo01_" + l, keyPacket(0x04, ComboAction { usage(0x05), Modifier::None, Modifier::LeftAlt }, layout));
        compare("knob_" + l, keyPacket(500, ComboAction { usage(0x05) }, layout));
        compare("knobmouse_" + l, keyPacket(0x1F7, MouseAction::LeftClick, layout));
        compare("media_" + l, keyPacket(0xE4, MediaAction::PlayPause, layout));
        compare("default_" + l, keyPacket(0x2C, DefaultAction {}, layout));
        compare("disable_" + l, keyPacket(0x2C, DisableAction {}, layout));

        std::optional<KeyPacket> disable = encodeKeyAction(key(0x2C), DisableAction {}, layout, Slot::G1);
        compare("chunks_disable_" + l, hex(chunk(QByteArrayView(disable->data(), disable->size()))));
    }

    // a mouse function on a regular key is a combo without modifiers
    std::optional<KeyPacket> mouse = encodeKeyAction(key(0x04), MouseAction::Button4, Layout::Tkl87, Slot::G1);
    QCOMPARE(hex(*mouse).mid(8, 10), QByteArray("01a901a981"));
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
    const LightMode modes[] = {
        LightMode::Static, LightMode::Spectrum, LightMode::Rainbow, LightMode::PowerGauge, LightMode::Breathing,
        LightMode::TwinklingStars, LightMode::Reactive, LightMode::Marquee, LightMode::Aurora, LightMode::Custom,
    };
    for (LightMode mode : modes)
    {
        for (bool use_color : { false, true })
        {
            LightingEffect effect;
            effect.mode = mode;
            effect.useColor = use_color;
            effect.speed = Speed::clamped(3);
            effect.brightness = Brightness::clamped(11);
            effect.color = { 0x12, 0x34, 0x56 };
            effect.direction = drevo::RainbowDirection::LeftToRight;
            const QByteArray name = "light_" + QByteArray::number(int(mode)) + "_" + QByteArray::number(use_color ? 1 : 0);
            compare(name, hex(encodeLighting(effect)));
        }
    }
    compare("lightsoff", hex(encodeLighting(LightingEffect::off())));

    // the direction is the byte after speed / brightness
    LightingEffect rainbow;
    rainbow.mode = LightMode::Rainbow;
    rainbow.direction = drevo::RainbowDirection::UpToDown;
    QCOMPARE(hex(encodeLighting(rainbow)), QByteArray("05fe030003000000"));
    QCOMPARE(hex(encodeLighting(LightingEffect::off())), QByteArray("05fe018000000000"));
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

    // JIS keys are not on the 87 key layout
    QVERIFY(!encodeKeyAction(key(0x89), DefaultAction {}, Layout::Tkl87, Slot::G1));
    QVERIFY(encodeKeyAction(key(0x89), DefaultAction {}, Layout::Jis91, Slot::G1));
}

QTEST_APPLESS_MAIN(TestProtocol)

#include "tst_protocol.moc"
