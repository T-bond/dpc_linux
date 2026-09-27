# DREVO keyboard access: device listing, packet encoding and USB transfers
add_library(drevo_keyboard STATIC
    include/drevo/Types.h
    include/drevo/KeyAction.h
    include/drevo/Lighting.h
    include/drevo/Keyboard.h
    src/Keyboard.cpp
    src/Protocol.cpp                src/Protocol.h
    src/UsbTransport.cpp            src/UsbTransport.h
)

target_include_directories(drevo_keyboard
    PUBLIC include
    PRIVATE src
)
target_link_libraries(drevo_keyboard
    PUBLIC Qt6::Core
    PRIVATE PkgConfig::LIBUSB
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
