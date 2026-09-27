find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_protocol tst_protocol.cpp)
target_include_directories(tst_protocol PRIVATE ../src)
target_link_libraries(tst_protocol PRIVATE drevo_keyboard Qt6::Test)
target_compile_definitions(tst_protocol PRIVATE GOLDEN_FILE="${CMAKE_CURRENT_SOURCE_DIR}/golden.txt")

add_test(NAME tst_protocol COMMAND tst_protocol)
