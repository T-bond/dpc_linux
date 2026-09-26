DREVO Power Console Linux
====

Built with CMake against Qt 6 and [libusb](https://libusb.info/) 1.0.

The whole application is built from source. The HID packet encoders that used to ship as the prebuilt
`lib/libhidkeyboard.a` are now implemented in `keyboarddata.cpp`, which also documents the packet formats.

Installation
--

#### Arch Linux:
Package available through [AUR](https://aur.archlinux.org/packages/drevo-power-console-git/)

#### Building Manually

###### Arch Linux package requirements
```bash
sudo pacman -S --needed base-devel cmake qt6-base libusb
```

###### Debian/Ubuntu package requirements
```bash
sudo apt install build-essential cmake qt6-base-dev libusb-1.0-0-dev
```

###### run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/DrevoPowerConsole
```

To install system-wide (optionally with the udev rule below), run
`cmake -B build -DINSTALL_UDEV_RULES=ON && sudo cmake --install build`.

###### Udev Fix
To be able to run the program with non root access you will need to copy the `udev` rule to your installation:

```bash
cp udev/77-drevo-usb-allow-wheel.rules /usr/lib/udev/rules.d/
```

Screenshots
--

![](https://github.com/lanyu7/dpc_linux/blob/master/picture/1.png)
![](https://github.com/lanyu7/dpc_linux/blob/master/picture/2.png)
![](https://github.com/lanyu7/dpc_linux/blob/master/picture/3.png)
![](https://github.com/lanyu7/dpc_linux/blob/master/picture/4.png)
![](https://github.com/lanyu7/dpc_linux/blob/master/picture/5.png)

support@drevo.net
