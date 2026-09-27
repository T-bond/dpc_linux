# TODO

## Suspected bugs kept from the original program

These are marked `quirk` in the code. Items that change what is sent to the keyboard
(wireless sleep time) should be checked on the hardware before fixing.

- [ ] **Wireless sleep time**: the wireless mode packet sends the USB sleep time as the backlight
      time, so the "Backlighting" setting has no effect (`src/KeyboardSettings.cpp`,
      `sendKeyboardSleepTime()`).

## To verify on the keyboard

- [ ] **Lighting and settings per hardware profile**: the lighting, report rate and sleep commands
      have no hardware profile; they still follow the profile shown in the app. Check whether the
      keyboard stores them per hardware profile (does Fn+Ctrl+F2 change the lighting?).

- [ ] **Selecting a hardware profile from the computer**: no command is known, so the app cannot
      switch the keyboard to the hardware profile it shows, or tell which one is active (a USB
      capture of the Windows software could show one).

## Limitations

- [ ] **Key assignments at startup** are not sent; the app relies on the keyboard's memory and on
      what it wrote to each hardware profile (`[hardware]` in the settings file). If the keyboard was
      reset or programmed elsewhere, it differs from the app until a profile is written to that
      hardware profile again (`src/DeviceManager.cpp`).
- [ ] **Hardware profile not known**: the first write to a hardware profile the app has not written
      yet (or after a failed transfer) sends all keys (about 90 packets), which blocks the window for
      a moment.
