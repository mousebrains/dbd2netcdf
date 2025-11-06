add_executable(sgMergeNetCDF
	sgMergeNetCDF.C
	SGMerge.C
	)

add_executable(dbd2netCDF
	dbd2netCDF.C
       	SensorsMap.C
       	Sensors.C
       	Sensor.C
       	Header.C
	MyNetCDF.C
       	Data.C
       	KnownBytes.C
       	Decompress.C
	lz4.c
	)

add_executable(pd02netCDF
	pd02netCDF.C
	PD0.C
	MyNetCDF.C
	)

add_executable(dbd2csv
	dbd2csv.C
       	SensorsMap.C
       	Sensors.C
       	Sensor.C
       	Header.C
       	Data.C
       	KnownBytes.C
	Decompress.C
       	lz4.c
	)

add_executable(dbdSensors
	dbdSensors.C
	SensorsMap.C
	Sensors.C
	Sensor.C
       	Header.C
	KnownBytes.C
	Decompress.C
	lz4.c
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
target_compile_options(sgMergeNetCDF PRIVATE -Wall -Wextra -pedantic)
target_compile_options(dbd2netCDF PRIVATE -Wall -Wextra -pedantic)
target_compile_options(dbd2csv PRIVATE -Wall -Wextra -pedantic)
target_compile_options(dbdSensors PRIVATE -Wall -Wextra -pedantic)
target_compile_options(pd02netCDF PRIVATE -Wall -Wextra -pedantic)
target_compile_options(decompressTWR PRIVATE -Wall -Wextra -pedantic)

# Get where NetCDF is installed using the nc-config command
execute_process(COMMAND nc-config --includedir 
	OUTPUT_VARIABLE NCINCDIR 
	RESULT_VARIABLE RETURNCODE
	ERROR_VARIABLE ERROROUTPUT
	)
if (RETURNCODE)
    message(FATAL_ERROR "Error finding NetCDF include directory ${ERROROUTPUT}")
else (NOT RETURNCODE)
    string(STRIP ${NCINCDIR} NCINCDIR)
    message(STATUS "NetCDF include directory ${NCINCDIR}")
    include_directories(${NCINCDIR}) # Add NetCDF include directory into the search path
endif (RETURNCODE)

execute_process(COMMAND nc-config --libdir 
	OUTPUT_VARIABLE NCLIBDIR 
	RESULT_VARIABLE RETURNCODE
	ERROR_VARIABLE ERROROUTPUT
	)
if (RETURNCODE)
    message(FATAL_ERROR "Error finding NetCDF lib directory ${ERROROUTPUT}")
else (NOT RETURNCODE)
    string(STRIP ${NCLIBDIR} NCLIBDIR)
    message(STATUS "NetCDF lib directory ${NCLIBDIR}")
    link_directories(${NCLIBDIR}) # Add NetCDF library directory into the search path
endif (RETURNCODE)

include_directories(${dbd2netcdf_SOURCE_DIR}/bin) #  ${dbd2netcdf_SOURCE_DIR}/src)
include_directories(${CMAKE_INSTALL_PREFIX}/include)

if(APPLE) # Define some settings for the Bundle
  message(STATUS "ARCH ${CMAKE_OSX_ARCHITECTURES}")
  
  # set(MACOSX_BUNDLE_BUNDLE_NAME dbd2netcdf)
  # set(MACOSX_BUNDLE_GUI_IDENTIFIER "dbd2netcdf")
  # set(MACOSX_BUNDLE_ICON_FILE dbd2netcdf.icns)
  # set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${dbd2netcdf_VERSION_MAJOR}.${dbd2netcdf_VERSION_MINOR}")
  # set(MACOSX_BUNDLE_LONG_VERSION_STRING "${MACOSX_BUNDLE_SHORT_VERSION_STRING}.${dbd2netcdf_VERSION_PATCH}" )
  # set(MACOSX_BUNDLE_BUNDLE_VERSION "${MACOSX_BUNDLE_LONG_VERSION_STRING}")
  # set(MACOSX_BUNDLE_COPYRIGHT "(C) 2012 Pat Welch")
  # set(MACOSX_BUNDLE_INFO_STRING ""${MACOSX_BUNDLE_LONG_VERSION_STRING}",
  #                               ${MACOSX_BUNDLE_COPYRIGHT}")
  # create a bundle with an icon too!
  # If you're not using Qt, uncomment the next line and comment the line below it:
  # add_executable(dbd2netcdf MACOSX_BUNDLE ${dbd2netcdf_SRC_FILES})
  # add_executable(dbd2netcdf MACOSX_BUNDLE ${SRC_FILES} ${MOC_SRCS} ${RC_SRC_FILES} ${UI_HDRS} )
                              
  # Allows for bundle re-creation just by running "make". Also installs bundle icon
  # add_custom_target( osx_bundle_dirs
                    # COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/dbd2netcdf.app/Contents/Resources
                    # COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/dbd2netcdf.app/Contents/MacOS
                    # COMMAND cp ${dbd2netcdf_SOURCE_DIR}/pixmaps/${MACOSX_BUNDLE_ICON_FILE}
                               # ${CMAKE_CURRENT_BINARY_DIR}/dbd2netcdf.app/Contents/Resources/${MACOSX_BUNDLE_ICON_FILE}

  # Qt translations - uncomment this line when you need to install them to the bundle
  #COMMAND cp *.qm ${CMAKE_CURRENT_BINARY_DIR}/dbd2netcdf.app/Contents/Resources/
  #                                                                   )
  # add_dependencies(dbd2netCDF osx_bundle_dirs)
  # add_dependencies(dbd2csv osx_bundle_dirs)
  # add_dependencies(dbdSensors osx_bundle_dirs)

  # This tells cmake where to place files inside the bundle
  # set_source_files_properties(${dbd2netcdf_RESOURCES} ${dbd2netcdf_TRANSLATIONS}
                              # PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
else(NOT APPLE)
  # Builds a binary for windows (without cmdline showing), regular for Linux
  # If you need the cmdline showing, remove the WIN32 attribute

  # If you're not using Qt, uncomment the next line and comment the line below it:
  # add_executable(dbd2netcdf WIN32 ${SRC_FILES})
  # add_executable(dbd2netcdf WIN32 ${SRC_FILES} ${MOC_SRCS} ${RC_SRC_FILES} ${UI_HDRS} )
endif(APPLE)

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

find_library(netCDF_libnetcdf NAMES netcdf)
if (NOT netCDF_libnetcdf)
    message(FATAL_ERROR "Unable to locate libnetcdf")
endif (NOT netCDF_libnetcdf)

# Write out config.h as derived from config.h.in

configure_file(${dbd2netcdf_SOURCE_DIR}/src/config.h.in
               ${dbd2netcdf_SOURCE_DIR}/src/config.h)

# We want netCDF libraries linked in

target_link_libraries(sgMergeNetCDF ${netCDF_libnetcdf})
target_link_libraries(dbd2netCDF ${netCDF_libnetcdf})
target_link_libraries(pd02netCDF ${netCDF_libnetcdf})

# Link stdc++fs for GCC 8.x (required for filesystem support)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(dbd2netCDF stdc++fs)
    target_link_libraries(dbd2csv stdc++fs)
    target_link_libraries(dbdSensors stdc++fs)
    target_link_libraries(decompressTWR stdc++fs)
endif()

# Install locally

install(TARGETS sgMergeNetCDF dbd2netCDF pd02netCDF dbd2csv dbdSensors decompressTWR 
	DESTINATION bin)
