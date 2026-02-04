# CMakeLists.txt for fuzz testing
# Fuzz tests require Clang with libFuzzer support

# Only build fuzz tests if requested and using Clang
option(BUILD_FUZZ_TESTS "Build fuzz tests (requires Clang with libFuzzer)" OFF)

if(BUILD_FUZZ_TESTS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(WARNING "Fuzz tests require Clang compiler with libFuzzer. "
                        "Current compiler: ${CMAKE_CXX_COMPILER_ID}")
        return()
    endif()

    message(STATUS "Building fuzz tests with libFuzzer")

    # Common fuzz test settings
    set(FUZZ_COMPILE_FLAGS -fsanitize=fuzzer,address,undefined -g -O1)
    set(FUZZ_LINK_FLAGS -fsanitize=fuzzer,address,undefined)

    # Fuzz target: Sensor parsing
    add_executable(fuzz_sensor fuzz_sensor.cpp)
    target_link_libraries(fuzz_sensor PRIVATE dbd_common)
    target_compile_options(fuzz_sensor PRIVATE ${FUZZ_COMPILE_FLAGS})
    target_link_options(fuzz_sensor PRIVATE ${FUZZ_LINK_FLAGS})
    set_target_properties(fuzz_sensor PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Fuzz target: Header utilities
    add_executable(fuzz_header fuzz_header.cpp)
    target_link_libraries(fuzz_header PRIVATE dbd_common)
    target_compile_options(fuzz_header PRIVATE ${FUZZ_COMPILE_FLAGS})
    target_link_options(fuzz_header PRIVATE ${FUZZ_LINK_FLAGS})
    set_target_properties(fuzz_header PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Create corpus directories for seeds
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/fuzz_corpus/sensor)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/fuzz_corpus/header)

    # Custom target to run fuzz tests
    add_custom_target(fuzz
        COMMAND ${CMAKE_COMMAND} -E echo "Run fuzz tests manually:"
        COMMAND ${CMAKE_COMMAND} -E echo "  ./bin/fuzz_sensor fuzz_corpus/sensor -max_total_time=60"
        COMMAND ${CMAKE_COMMAND} -E echo "  ./bin/fuzz_header fuzz_corpus/header -max_total_time=60"
        DEPENDS fuzz_sensor fuzz_header
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
