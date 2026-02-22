# dbd2netcdf

[![Build and Test](https://github.com/mousebrains/dbd2netcdf/actions/workflows/build-test.yml/badge.svg)](https://github.com/mousebrains/dbd2netcdf/actions/workflows/build-test.yml)
[![Fuzz Testing](https://github.com/mousebrains/dbd2netcdf/actions/workflows/fuzz.yml/badge.svg)](https://github.com/mousebrains/dbd2netcdf/actions/workflows/fuzz.yml)
[![codecov](https://codecov.io/gh/mousebrains/dbd2netcdf/graph/badge.svg)](https://codecov.io/gh/mousebrains/dbd2netcdf)
[![Latest Release](https://img.shields.io/github/v/release/mousebrains/dbd2netcdf)](https://github.com/mousebrains/dbd2netcdf/releases/latest)
[![License: GPL v3](https://img.shields.io/github/license/mousebrains/dbd2netcdf)](https://github.com/mousebrains/dbd2netcdf/blob/main/License.txt)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/platform-linux%20%7C%20macOS%20%7C%20windows-lightgrey)]()

This software is designed to translate a set of Dinkum Binary Data files into
a netCDF file. See [Architecture Documentation](doc/ARCHITECTURE.md) for technical details.

Requires a C++17 compatible compiler and NetCDF/HDF5 development libraries.
See [INSTALL.md](INSTALL.md) for build instructions, platform-specific dependencies, and [cleaning/rebuilding](INSTALL.md#cleaning-and-rebuilding).

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
