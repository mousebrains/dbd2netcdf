#
# set up test for use by ctest
#

enable_testing()

add_test(dbdSensors ${dbd2netcdf_SOURCE_DIR}/test/dbdSensors) 
add_test(dbd2csv ${dbd2netcdf_SOURCE_DIR}/test/dbd2csv) 
add_test(dbd2netCDF ${dbd2netcdf_SOURCE_DIR}/test/dbd2netCDF) 
add_test(pd02netCDF ${dbd2netcdf_SOURCE_DIR}/test/pd02netCDF) 

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} 
                  DEPENDS dbd2netCDF dbd2csv dbdSensors)
