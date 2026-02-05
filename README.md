# dbd2netcdf

[![Build and Test](https://github.com/mousebrains/dbd2netcdf/actions/workflows/build-test.yml/badge.svg)](https://github.com/mousebrains/dbd2netcdf/actions/workflows/build-test.yml)
[![codecov](https://codecov.io/gh/mousebrains/dbd2netcdf/graph/badge.svg)](https://codecov.io/gh/mousebrains/dbd2netcdf)
[![Latest Release](https://img.shields.io/github/v/release/mousebrains/dbd2netcdf)](https://github.com/mousebrains/dbd2netcdf/releases/latest)

This software is designed to translate a set of Dinkum Binary Data files into
a netCDF file. See [Architecture Documentation](doc/ARCHITECTURE.md) for technical details.

Requires a C++17 compatible compiler and NetCDF/HDF5 development libraries.
See [INSTALL.md](INSTALL.md) for build instructions and platform-specific dependencies.

If a filename follows the TWR convention for compressed files, `*.?c?`, it will be treated as an LZ4 compressed file, and automatically decompressed.

*Sample commands:*
- `./dbd2netcdf --cache=/data/cache --output=foobar.nc *.d?d`
- `./dbd2csv --cache=/data/cache --output=foobar.csv *.e?d`
- `./decompressTWR *.?c?`

*Tested on:*
- macOS 13.x, 14.x, 15.x (Intel and ARM)
- Ubuntu 18.04, 20.04, 22.04, 24.04, 24.10 (Intel and ARM)
- AlmaLinux 8, Rocky Linux 9
- Cygwin, WSL
