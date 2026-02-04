# CMakeLists.txt for performance benchmarks using Catch2

option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)

if(BUILD_BENCHMARKS)
    message(STATUS "Building performance benchmarks")

    # Benchmark executable
    add_executable(benchmarks benchmark_main.cpp)

    target_link_libraries(benchmarks PRIVATE
        dbd_common
        Catch2::Catch2
    )

    set_target_properties(benchmarks PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Compiler flags - use Release optimization for benchmarks
    if(MSVC)
        target_compile_options(benchmarks PRIVATE /O2)
    else()
        target_compile_options(benchmarks PRIVATE -O3)
    endif()

    # Custom target to run benchmarks
    add_custom_target(run_benchmarks
        COMMAND ${CMAKE_BINARY_DIR}/bin/benchmarks --benchmark-samples=100
        DEPENDS benchmarks
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running performance benchmarks..."
    )
endif()
