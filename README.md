# dbd2netcdf

**Version: 1.6.5**

This software is designed to translate a set of Dinkum Binary Data files into
a netCDF file.

You must have installed hdf5 and netCDF,
prior to configuring, building, and installing this software!

It now requires a c++17 or higher compatible compiler

If a filename follows the TWR convention for compressed files, `*.?c?`, it will be treated as an LZ4 compressed file, and automatically decompressed.

*Sample commands:*
- `./dbd2netcdf --cache=/data/cache --output=foobar.nc *.d?d`
- `./dbd2csv --cache=/data/cache --output=foobar.csv *.e?d`
- `./decompressTWR *.?c?`


*To start afresh with cmake:*
- `find . -name 'CMakeFiles' -print -delete`
- `find . -name 'CMakeCache.txt' -print -delete`

*This version was tested on the following systems:*
- MacOS 13.x, 14.x, 15.x Intel and ARM
- MacOS with homebrew
- Ubuntu 18.04, 20.04, 22.04, 24.04, 24.10, Intel and ARM
- Alma Linux 8
- Cygwin
- WSL

* Dependencies:
- cmake
- C++ compiler, like gcc-c++
- NetCDF and hdf5 development packages (May need libsz.so.2)
- powertools on RHEL/Alma
-
