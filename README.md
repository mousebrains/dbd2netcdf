This software is designed to translate a set of Dinkum Binary Data files into
a netCDF file.

You must have installed hdf5 and netCDF,
prior to configuring, building, and installing this software!

It now requires a c++17 or higher compatible compiler

If a filename follows the TWR convention for compressed, `*.?c?`, it will be treated as an LZ4 compressed file, and automatically decompressed.

* Sample commands:
- `./dbd2netcdf --cache=/data/cache --output=foobar.nc *.d?d`
- `./dbd2csv --cache=/data/cache --output=foobar.csv *.e?d`
- `./decompressTWR *.?c?`


* To start afresh with cmake
- rm -rf `find . -name 'CMakeFiles' -print` `find . -name CMakeCache.txt -print`
