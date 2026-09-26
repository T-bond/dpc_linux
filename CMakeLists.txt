cmake_minimum_required(VERSION 3.16)

project(DrevoPowerConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt 6 is preferred, Qt 5 still works
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

add_executable(DrevoPowerConsole
    main.cpp
    mainwindow.cpp              mainwindow.h            mainwindow.ui
    qkeyboardctrl.cpp           qkeyboardctrl.h
    qkeyboardctrl_init.cpp
    customkeywidget.cpp         customkeywidget.h       customkeywidget.ui
    custombutton.cpp            custombutton.h
    customknobwidget.cpp        customknobwidget.h      customknobwidget.ui
    keysetwidget.cpp            keysetwidget.h          keysetwidget.ui
    radilightwidget.cpp         radilightwidget.h       radilightwidget.ui
    devicecomm.cpp              devicecomm.h
    devicedb.cpp                devicedb.h
    keyboarddata.cpp            keyboarddata.h
    qtkeyboardkey.cpp           qtkeyboardkey.h
    keyboard_layout.qrc
)

target_compile_definitions(DrevoPowerConsole PRIVATE QT_DEPRECATED_WARNINGS)

target_link_libraries(DrevoPowerConsole PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    PkgConfig::LIBUSB
)

include(GNUInstallDirs)
install(TARGETS DrevoPowerConsole RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

option(INSTALL_UDEV_RULES "Install the udev rule for non-root device access" OFF)
if(INSTALL_UDEV_RULES)
    install(FILES udev/77-drevo-usb-allow-wheel.rules DESTINATION lib/udev/rules.d)
endif()
