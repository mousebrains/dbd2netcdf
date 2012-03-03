bin_PROGRAMS = dbd2netCDF dbdSensors dbd2csv

EXTRA_DIST = Data.H DataColumn.H Header.H KnownBytes.H MyException.H Sensor.H Sensors.H SensorsMap.H StackDump.H Version.H

dbd2netCDF_SOURCES = Data.C DataColumn.C Header.C KnownBytes.C Sensor.C Sensors.C SensorsMap.C StackDump.C dbd2netCDF.C

dbd2csv_SOURCES = Data.C DataColumn.C Header.C KnownBytes.C Sensor.C Sensors.C SensorsMap.C dbd2csv.C StackDump.C

dbdSensors_SOURCES = Header.C KnownBytes.C Sensor.C Sensors.C SensorsMap.C dbdSensors.C StackDump.C

dist_man_MANS = dbd2netCDF.1 dbd2csv.1 dbdSensors.1

AM_MAKEINFOFLAGS = --no-split

export COPYFILE_DISABLE=true
