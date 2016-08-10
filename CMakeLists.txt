# CMake configuration for dbd2netcdf package
#
# Mar-2012, Pat Welch, pat@mousebrains.com

cmake_minimum_required(VERSION 3.6)

project(dbd2netcdf)

set(dbd2netcdf_VERSION_MAJOR 1)
set(dbd2netcdf_VERSION_MINOR 2)
set(dbd2netcdf_VERSION_PATCH 2)
set(dbd2netcdf_MAINTAINER pat@mousebrains.com) # output to config.h

if (APPLE)
  set(CMAKE_OSX_ARCHITECTURES "x86_64")
else (APPLE)
endif (APPLE)

add_subdirectory(src)  # Source code location
add_subdirectory(test) # Tests
add_subdirectory(doc)  # Documentation

# Build a CPack installer package

include(InstallRequiredSystemLibraries)

# http://www.cmake.org/Wiki/CMake:CPackConfiguration

set(CPACK_PACKAGE_DESCRIPTION_FILE ${dbd2netcdf_SOURCE_DIR}/Readme.txt)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Dinkum Binary Data to netCDF or csv")

set(CPACK_PACKAGE_VENDOR "mousebrains")
set(CPACK_PACKAGE_VERSION_MAJOR ${dbd2netcdf_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${dbd2netcdf_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${dbd2netcdf_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT ${dbd2netcdf_MAINTAINER})

set(CPACK_RESOURCE_FILE_LICENSE ${dbd2netcdf_SOURCE_DIR}/License.txt)
set(CPACK_RESOURCE_FILE_README ${dbd2netcdf_SOURCE_DIR}/Readme.txt)
set(CPACK_RESOURCE_FILE_WELCOME ${dbd2netcdf_SOURCE_DIR}/INSTALL)

set(CPACK_SOURCE_GENERATOR "TGZ;ZIP") # Place here incase it is modified below

if (APPLE)
  message(STATUS "Packing for APPLE")
  set(CPACK_GENERATOR "TGZ")
  set(CPACK_OSX_PACKAGE_VERSION 10.7)
  set(CPACK_BUNDLE_NAME dbd2netcdf)
  set(CPACK_BUNDLE_PLIST "${dbd2netcdf_SOURCE_DIR}/osx.files/Info.plist")
  set(CPACK_BUNDLE_ICON "${dbd2netcdf_SOURCE_DIR}/pixmaps/dbd2netcdf.icns")
# set(CPACK_PACKAGE_DEFAULT_LOCATION "/Applications/dbd2netcdf")
# set(CPACK_PACKAGE_INSTALL_DIRECTORY dbd2netcdf ${dbd2netcdf_VERSION_MAJOR}.${dbd2netcdf_VERSION_MINOR}.${dbd2netcdf_VERSION_PATCH})
# set(CPACK_SET_DESTDIR "ON")
# set(CPACK_INSTALL_PREFIX "/Applications/dbd2netcdf")
elseif (CYGWIN)
  message(STATUS "Packing for CYGWIN")
  set(CPACK_GENERATOR "CygwinBinary")
  set(CPACK_SOURCE_GENERATOR "CygwinSource")
  SET(CPACK_CYGWIN_PATCH_FILE "${dbd2netcdf_SOURCE_DIR}/cygwin.files/patch.file")
  SET(CPACK_CYGWIN_PATCH_FILE "${dbd2netcdf_SOURCE_DIR}/cygwin.files/build.sh")
  message(WARNING "Building of CYGWIN Source packages does not work!")
elseif (UNIX)
  message(STATUS "Packing for UNIX")
  set(CPACK_GENERATOR "TGZ")
  find_program(dbd2netCDF_HAS_RPMBUILD "rpmbuild")
  if (NOT "${dbd2netCDF_HAS_RPMBUILD}" STREQUAL "dbd2netCDF_HAS_RPMBUILD-NOTFOUND")  
    list(APPEND CPACK_GENERATOR "RPM")
    message("rpmbuild found")
    set(CPACK_RPM_PACKAGE_LICENSE "GPLV3")
    set(CPACK_RPM_PACKAGE_GROUP "Sciences/Other")
  endif (NOT "${dbd2netCDF_HAS_RPMBUILD}" STREQUAL "dbd2netCDF_HAS_RPMBUILD-NOTFOUND")  

  find_program(dbd2netCDF_HAS_DEBBUILD "dpkg-buildpackage")
  if (NOT "${dbd2netCDF_HAS_DEBBUILD}" STREQUAL "dbd2netCDF_HAS_DEBBUILD-NOTFOUND")  
    list(APPEND CPACK_GENERATOR "DEB")
    message("dpkg-buildpackage found")
  endif (NOT "${dbd2netCDF_HAS_DEBBUILD}" STREQUAL "dbd2netCDF_HAS_DEBBUILD-NOTFOUND")  
elseif (WIN32)
  message(STATUS "Packing for Windows")
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_PACKAGE_EXECUTABLES dbd2netcdf;dbd2netcdf dbd2csv;dbd2csv dbdSensors;dbdSensors)
else (APPLE)
  message(STATUS "Unrecognized system to creat packaging for")
endif (APPLE)

# Create packaging for the source

set (CPACK_SOURCE_IGNORE_FILES 
     "\\\\.#\$" "/#" ".*~\$" "\\\\.swp\$"
     "/toy/" "/bin/"
     "/\\\\.git/" "\\\\.gitignore"
     "${PROJECT_NAME}-[0-9]+.[0-9]+.[0-9]+-.*"
     "config.h\$" "Info.plist\$"
     "\\\\.cmake\$" "Makefile" "install_manifest.txt"
     "/CMakeFiles/" "CMakeCache.txt"
     "/_CPack_Packages/"
     "/Testing/" 
     )

include(CPack)

configure_file(${dbd2netcdf_SOURCE_DIR}/osx.files/Info.plist.in
               ${dbd2netcdf_SOURCE_DIR}/osx.files/Info.plist)

# Enable dashboard scripting

# include(CTest)
# set(CTEST_PROJECT_NAME dbd2netcdf)
