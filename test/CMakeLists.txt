#
# set up tests for use by ctest
#
# Tests expect to run from the test/ directory with executables in ../bin/
#

find_program(SH_PROGRAM sh)

if (SH_PROGRAM)
	add_test(NAME dbdSensors
		COMMAND ${SH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/dbdSensors
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

	add_test(NAME dbd2csv
		COMMAND ${SH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/dbd2csv
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

	add_test(NAME dbd2netCDF
		COMMAND ${SH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/dbd2netCDF
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

	add_test(NAME pd02netCDF
		COMMAND ${SH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/pd02netCDF
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

	# Legacy "make check" target for backwards compatibility
	add_custom_target(check
		COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
		DEPENDS dbd2netCDF dbd2csv dbdSensors pd02netCDF
	)
endif()
