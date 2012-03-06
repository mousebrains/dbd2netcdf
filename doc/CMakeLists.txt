# Install the documentation

set(dbd2netCDF_MANPAGES dbd2netCDF.1 dbd2csv.1 dbdSensors.1)

install(FILES ${dbd2netCDF_MANPAGES} DESTINATION man/man1)
