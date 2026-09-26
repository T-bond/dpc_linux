DREVO Power Console Linux
====

Built with CMake against Qt 6.5 or newer and [libusb](https://libusb.info/) 1.0. The user interface is written in
QML (Qt Quick Controls, in `qml/`); the C++ side (`src/`, headers in `include/`) talks to the keyboard and stores the settings.

Installation
--

#### Arch Linux package

`packaging/arch/PKGBUILD` builds a `drevo-power-console-qt6-git` package from this repository, so it does
not have to be in the AUR. It installs the program, the desktop entry and the udev rule (so no root is
needed to use the keyboard; your user has to be in the `users` group).

The package is built from the **latest commit** of your checkout: commit your changes first, uncommitted
changes are not included.

With [yay](https://github.com/Jguer/yay), from the repository root:

```bash
yay -Bi packaging/arch
```

`-B` builds the PKGBUILD in the given directory (installing the build dependencies `cmake` and `qt6-tools`
if needed) and `-i` installs the package. Run the same command again after new commits to update; the
package version is taken from git (`r<commit count>.<commit hash>`).

Without yay, the same with makepkg:

```bash
cd packaging/arch
makepkg -si
```

The package replaces the AUR package `drevo-power-console-git` (which builds the original project) if it
is installed; it has a different name so that `yay -Syu` does not replace it with the AUR one. To build
from GitHub instead of the local checkout, change the `source` line as described at the top of the
PKGBUILD. Uninstall with `sudo pacman -R drevo-power-console-qt6-git`.

#### Building Manually

###### Arch Linux package requirements
```bash
sudo pacman -S --needed base-devel cmake qt6-base qt6-declarative qt6-tools libusb
```

###### run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j $(nproc)
./build/DrevoPowerConsole
```

To install system-wide (optionally with the udev rule below), run
`cmake -B build -DINSTALL_UDEV_RULES=ON && sudo cmake --install build`.

#### Command line options

- `--debug`: print every packet sent to the keyboard (as hex bytes, marked `(not connected)` when no
  keyboard was found, or `(failed)` when the transfer failed). Without it, the program prints no packet
  data. The output uses the `drevo.packets` logging category; when the program is not started from a
  terminal, Qt may send it to the system journal instead (set `QT_FORCE_STDERR_LOGGING=1` to keep it on
  the console).
- `--help`: list the options.

Only one instance can use the keyboard: starting the program again brings the running window to the
front instead (on Wayland, the desktop may only highlight it when started from a terminal).

#### System tray

With *Settings → Window → Close to the system tray*, closing the window keeps the program running in the
tray. Clicking the tray icon shows the window again; its menu selects the profile, switches the keyboard
lights off (without changing the profile; selecting it again, or any lighting change, turns them back on)
and exits the program.

#### Translations

The program uses the system language (e.g. `LANG=hu_HU.UTF-8`); available translations: English, Hungarian.
Translations live in `i18n/DrevoPowerConsole_<language>.ts` and can be edited with Qt Linguist.
After changing texts in the sources, refresh the `.ts` files with `cmake --build build --target update_translations`.
To add a language, add its `.ts` file to `qt_add_translations()` in `CMakeLists.txt` and run the same command.

Settings (key assignments, lighting, the keyboard layout) are stored in `~/.config/DrevoPowerConsole/DrevoPowerConsole.conf`.

###### Udev Fix
To be able to run the program with non root access you will need to copy the `udev` rule to your installation:

```bash
cp udev/77-drevo-usb-allow-users.rules /usr/lib/udev/rules.d/
```
