# TODO

## Suspected bugs kept from the original program

These are marked `quirk` in the code. Items that change what is sent to the keyboard
(wireless sleep time, knob "Volume -", combo keys) should be checked on the hardware before fixing.

- [ ] **Wireless sleep time**: the wireless mode packet sends the USB sleep time as the backlight
      time, so the "Backlighting" setting has no effect (`src/KeyboardSettings.cpp`,
      `sendKeyboardSleepTime()`).
- [ ] **Knob "Volume -"** is mapped to volume up (`src/KeyAssignment.cpp`, `functionTree()`).
- [ ] **Combo key names**: with only the first modifier, the stored name is "Left Ctrl+Null+A";
      with only the second modifier, it stays "Keyboard function" (`src/KeyAssignment.cpp`, `save()`).
- [ ] **Info box after Save** is not refreshed; it shows the previous assignment until the key is
      selected again (`qml/AssignmentPage.qml`).
- [ ] **Combo key packets** (`libdrevo/src/Protocol.cpp`, `writeCombo()`, `encodeMacro()`):
  - with one modifier, the target key is pressed twice and never released;
  - with only the second modifier, no key events are written;
  - in macros, the play count byte is overwritten by the first event.
- [ ] **"Cancel" on the assignment pages** restores the key's default function instead of
      cancelling (`qml/AssignmentPage.qml`); rename it or change its behavior.

## To verify on the keyboard

- [ ] **Custom color off**: the on-screen preview shows the stored color, but the keyboard may use
      its own default colors then (`src/Lighting.cpp`).
- [ ] **Per-key and side light colors** on the 88 key ISO keyboard, now that the detected layout
      is used for the RGB packet (`src/Lighting.cpp`, `sendKeyRGBData()`).

## Limitations

- [ ] **Hardware profile slots**: key assignments are always written to slot G1; G2/G3
      (Fn+Ctrl+F2/F3) are not used (`kSlot` in `libdrevo/src/Keyboard.cpp`).
- [ ] **Key assignments at startup** are not sent; the app relies on the keyboard's memory. If the
      keyboard was reset or programmed elsewhere, it differs from the app until a profile switch or
      save (`src/DeviceManager.cpp`).
- [ ] **2.4G dongle** "BM87 PRO 2.4G" (`1a2c:b31f`) is not in the device table and is ignored
      (`libdrevo/src/Keyboard.cpp`, `supportedDevices()`).
- [ ] **Stored macro names** are saved in the language active at the time and are not translated
      later.
- [ ] **Color dialog**: "Color" and "Hex" stay in English in Hungarian (missing from Qt's own
      translations).
- [ ] **Language selection**: the UI language follows `LANGUAGE` (e.g. the Plasma language), not
      only `LC_ALL`/`LANG`. Add an in-app choice (System default / English / Magyar) on the Settings
      page.

## Bugs
- [ ] Clicking on cancel on an assignment page multiple times create multiple Ok buttons in the dialog
