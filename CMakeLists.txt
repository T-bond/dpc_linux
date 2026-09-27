cmake_minimum_required(VERSION 3.16)

project(DrevoPowerConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Network Qml Quick QuickControls2 Widgets LinguistTools)
qt_standard_project_setup(REQUIRES 6.5)
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()
set(CMAKE_AUTORCC ON)
# keep the QML module directory apart from the executable of the same name
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

option(BUILD_TESTING "Build the tests" ON)
if(BUILD_TESTING)
    enable_testing()
endif()

add_subdirectory(libdrevo)

qt_add_executable(DrevoPowerConsole
    src/main.cpp
    src/DeviceDB.cpp                include/DeviceDB.h
    src/QtKeyBoardKey.cpp           include/QtKeyBoardKey.h
    src/IconProvider.cpp            include/IconProvider.h
    src/SingleInstance.cpp          include/SingleInstance.h
    keyboard_layout.qrc
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(DrevoPowerConsole
    URI DrevoPowerConsole
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/TintedIcon.qml
        qml/Card.qml
        qml/Chip.qml
        qml/PageHeader.qml
        qml/NavButton.qml
        qml/KeyboardStage.qml
        qml/KeyboardView.qml
        qml/KeyShape.qml
        qml/AssignmentTree.qml
        qml/AssignmentPage.qml
        qml/CustomKeyPage.qml
        qml/CustomKnobPage.qml
        qml/LightingPage.qml
        qml/SettingsPage.qml
        qml/ProfileBar.qml
        qml/MessageBox.qml
    SOURCES
        src/DeviceManager.cpp       include/DeviceManager.h
        src/KeyboardModel.cpp       include/KeyboardModel.h
        src/KeyboardModelLayout.cpp
        src/KeyAssignment.cpp       include/KeyAssignment.h
        src/Lighting.cpp            include/Lighting.h
                                    include/LightModes.h
        src/KeyboardSettings.cpp    include/KeyboardSettings.h
        src/SystemTray.cpp          include/SystemTray.h
)

target_include_directories(DrevoPowerConsole PRIVATE include)

# translations, embedded as :/i18n/DrevoPowerConsole_<language>.qm;
# "cmake --build build --target update_translations" refreshes the .ts files from the sources
# (SOURCE_TARGETS avoids the deferred call, which breaks paths with spaces)
qt_add_translations(DrevoPowerConsole
    SOURCE_TARGETS DrevoPowerConsole
    TS_FILES i18n/DrevoPowerConsole_hu.ts
    RESOURCE_PREFIX /i18n
)
target_compile_definitions(DrevoPowerConsole PRIVATE QT_DEPRECATED_WARNINGS)

target_link_libraries(DrevoPowerConsole PRIVATE
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    drevo_keyboard
)

include(GNUInstallDirs)
install(TARGETS DrevoPowerConsole RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES drevo-power-console.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES image/drevo-power-console.png DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps)

option(INSTALL_UDEV_RULES "Install the udev rule for non-root device access" OFF)
if(INSTALL_UDEV_RULES)
    install(FILES udev/77-drevo-usb-allow-users.rules DESTINATION lib/udev/rules.d)
endif()
