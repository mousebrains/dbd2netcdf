# CMakeLists.txt for unit tests using Catch2

# Find Catch2 (fetched from main CMakeLists.txt)
include(Catch)

# Build the unit test executable
add_executable(unit_tests
    test_main.cpp
    test_sensor.cpp
    test_sensors.cpp
    test_header.cpp
)

target_link_libraries(unit_tests PRIVATE
    dbd_common
    Catch2::Catch2
)

set_target_properties(unit_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Compiler flags
if(MSVC)
    target_compile_options(unit_tests PRIVATE /W4)
else()
    target_compile_options(unit_tests PRIVATE -Wall -Wextra -pedantic)
endif()

# Register tests with CTest
catch_discover_tests(unit_tests)
