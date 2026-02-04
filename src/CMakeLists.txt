# Define source file lists for reuse
set(COMMON_SOURCES
	SensorsMap.C
	Sensors.C
	Sensor.C
	Header.C
	KnownBytes.C
	Decompress.C
	lz4.c
)

add_executable(sgMergeNetCDF
	sgMergeNetCDF.C
	SGMerge.C
)

add_executable(dbd2netCDF
	dbd2netCDF.C
	${COMMON_SOURCES}
	MyNetCDF.C
	Data.C
)

add_executable(pd02netCDF
	pd02netCDF.C
	PD0.C
	MyNetCDF.C
)

add_executable(dbd2csv
	dbd2csv.C
	${COMMON_SOURCES}
	Data.C
)

add_executable(dbdSensors
	dbdSensors.C
	${COMMON_SOURCES}
)

add_executable(decompressTWR
	decompressTWR.C
	Decompress.C
	lz4.c
)

set_target_properties(sgMergeNetCDF dbd2netCDF dbd2csv dbdSensors pd02netCDF decompressTWR
	PROPERTIES
	  RUNTIME_OUTPUT_DIRECTORY ${dbd2netcdf_SOURCE_DIR}/bin
	  LINKER_LANGUAGE CXX
	  CXX_STANDARD 17
	  CXX_STANDARD_REQUIRED ON
	  CXX_EXTENSIONS OFF
)

# Set compiler flags for all targets
set(ALL_TARGETS sgMergeNetCDF dbd2netCDF dbd2csv dbdSensors pd02netCDF decompressTWR)

foreach(target ${ALL_TARGETS})
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    # Disable warnings for third-party code (lz4)
    set_source_files_properties(lz4.c PROPERTIES COMPILE_FLAGS "/W0")
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
  endif()
  # Link CLI11 to all targets
  target_link_libraries(${target} PRIVATE CLI11::CLI11)
endforeach()

# Find NetCDF - use different methods for different platforms
if(WIN32 AND NOT CYGWIN)
  # On Windows, use find_package (works with vcpkg)
  find_package(netCDF CONFIG REQUIRED)
  set(NETCDF_INCLUDE_DIRS "")
  set(NETCDF_LIBRARIES netCDF::netcdf)
  message(STATUS "Found NetCDF via CMake config")
else()
  # On Unix-like systems, use nc-config
  execute_process(COMMAND nc-config --includedir
    OUTPUT_VARIABLE NCINCDIR
    RESULT_VARIABLE RETURNCODE
    ERROR_VARIABLE ERROROUTPUT
  )
  if (RETURNCODE)
    message(FATAL_ERROR "Error finding NetCDF include directory ${ERROROUTPUT}")
  else()
    string(STRIP ${NCINCDIR} NCINCDIR)
    message(STATUS "NetCDF include directory ${NCINCDIR}")
    set(NETCDF_INCLUDE_DIRS ${NCINCDIR})
  endif()

  execute_process(COMMAND nc-config --libdir
    OUTPUT_VARIABLE NCLIBDIR
    RESULT_VARIABLE RETURNCODE
    ERROR_VARIABLE ERROROUTPUT
  )
  if (RETURNCODE)
    message(FATAL_ERROR "Error finding NetCDF lib directory ${ERROROUTPUT}")
  else()
    string(STRIP ${NCLIBDIR} NCLIBDIR)
    message(STATUS "NetCDF lib directory ${NCLIBDIR}")
    link_directories(${NCLIBDIR})
  endif()

  find_library(NETCDF_LIBRARIES NAMES netcdf PATHS ${NCLIBDIR})
  if (NOT NETCDF_LIBRARIES)
    message(FATAL_ERROR "Unable to locate libnetcdf")
  endif()
endif()

# Add NetCDF include directories
if(NETCDF_INCLUDE_DIRS)
  include_directories(${NETCDF_INCLUDE_DIRS})
endif()

include_directories(${dbd2netcdf_SOURCE_DIR}/bin) #  ${dbd2netcdf_SOURCE_DIR}/src)
include_directories(${CMAKE_INSTALL_PREFIX}/include)

if(APPLE)
  message(STATUS "Building for macOS, ARCH: ${CMAKE_OSX_ARCHITECTURES}")
endif()

# Handle rpath
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
  SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

get_property(_system_include_dirs GLOBAL PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
message("System: ${_system_include_dirs}")

include(CheckIncludeFile)
check_include_file(execinfo.h HAVE_EXECINFO_H)
check_include_file(unistd.h HAVE_UNISTD_H)

include(CheckFunctionExists)
check_function_exists(backtrace HAVE_BACKTRACE)

include(CheckTypeSize)
CHECK_TYPE_SIZE(int8_t HAVE_INT8_T)
CHECK_TYPE_SIZE(int16_t HAVE_INT16_T)
CHECK_TYPE_SIZE(int32_t HAVE_INT32_T)
CHECK_TYPE_SIZE(mode_t HAVE_mode_T)
CHECK_TYPE_SIZE(off_t HAVE_off_T)
CHECK_TYPE_SIZE(size_t HAVE_size_T)

# Write out config.h as derived from config.h.in
configure_file(${dbd2netcdf_SOURCE_DIR}/src/config.h.in
               ${dbd2netcdf_SOURCE_DIR}/src/config.h)

# Link NetCDF libraries to targets that need it
target_link_libraries(sgMergeNetCDF PRIVATE ${NETCDF_LIBRARIES})
target_link_libraries(dbd2netCDF PRIVATE ${NETCDF_LIBRARIES})
target_link_libraries(pd02netCDF PRIVATE ${NETCDF_LIBRARIES})

# Link stdc++fs for GCC 8.x (required for filesystem support)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(dbd2netCDF PRIVATE stdc++fs)
    target_link_libraries(pd02netCDF PRIVATE stdc++fs)
    target_link_libraries(dbd2csv PRIVATE stdc++fs)
    target_link_libraries(dbdSensors PRIVATE stdc++fs)
    target_link_libraries(decompressTWR PRIVATE stdc++fs)
    target_link_libraries(sgMergeNetCDF PRIVATE stdc++fs)
endif()

# Install locally

install(TARGETS sgMergeNetCDF dbd2netCDF pd02netCDF dbd2csv dbdSensors decompressTWR 
	DESTINATION bin)
