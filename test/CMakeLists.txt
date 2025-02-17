#
# set up test for use by ctest
#

enable_testing()

find_program (SH_PROGRAM sh)

if (SH_PROGRAM)
       	add_test(dbdSensors ${SH_PROGRAM} ${dbd2netcdf_SOURCE_DIR}/test/dbdSensors) 
	add_test(dbd2csv ${SH_PROGRAM} ${dbd2netcdf_SOURCE_DIR}/test/dbd2csv) 
	add_test(dbd2netCDF ${SH_PROGRAM} ${dbd2netcdf_SOURCE_DIR}/test/dbd2netCDF) 
	add_test(pd02netCDF ${SH_PROGRAM} ${dbd2netcdf_SOURCE_DIR}/test/pd02netCDF)
	
	add_custom_target(check
		COMMAND ${CMAKE_CTEST_COMMAND}
	       	DEPENDS dbd2netCDF dbd2csv dbdSensors
	)
endif (SH_PROGRAM)
