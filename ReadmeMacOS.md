# Installing dbd2netcdf on macOS

There are two approaches: using Homebrew (recommended) or building dependencies from source.

## Option 1: Using Homebrew (Recommended)

Homebrew is the easiest way to install dbd2netcdf on macOS.

### Install Prerequisites

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake netcdf hdf5
```

### Build dbd2netcdf

```bash
# Clone the repository
git clone https://github.com/mousebrains/dbd2netcdf.git
cd dbd2netcdf

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
cd build && ctest --output-on-failure && cd ..

# Install (optional)
sudo cmake --install build
```

### Update dbd2netcdf

```bash
cd dbd2netcdf
git pull
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

---

## Option 2: Building from Source

If you prefer not to use Homebrew, you can build all dependencies from source.

### Install Xcode Command Line Tools

```bash
xcode-select --install
```

### Install CMake

Download CMake from [cmake.org](https://cmake.org/download/) and install the macOS DMG.
Then add CMake to your PATH via Tools → "How to Install For Command Line Use".

### Install HDF5

```bash
# Download from https://www.hdfgroup.org/downloads/hdf5/source-code/
cd /usr/local/src  # or wherever you want to build
tar -xzf hdf5-*.tar.gz
cd hdf5-*

./configure --prefix=/usr/local
make -j$(sysctl -n hw.ncpu)
make check
sudo make install
```

### Install NetCDF

```bash
# Download from https://www.unidata.ucar.edu/software/netcdf/
cd /usr/local/src
tar -xzf netcdf-c-*.tar.gz
cd netcdf-c-*

./configure --prefix=/usr/local
make -j$(sysctl -n hw.ncpu)
make check
sudo make install
```

### Install dbd2netcdf

```bash
git clone https://github.com/mousebrains/dbd2netcdf.git
cd dbd2netcdf

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

cd build && ctest --output-on-failure && cd ..

sudo cmake --install build
```

---

## Usage

For command-line help:
```bash
dbd2netCDF --help
dbd2csv --help
dbdSensors --help
```

### Examples

Convert SBD files to NetCDF:
```bash
dbd2netCDF -o output.nc *.sbd
```

With sensor cache directory (recommended for multiple deployments):
```bash
dbd2netCDF -C /path/to/cache -o output.nc *.sbd
```

Convert to CSV:
```bash
dbd2csv -C /path/to/cache -o output.csv *.dbd
```

List sensors in a file:
```bash
dbdSensors file.dbd
```

---

## Troubleshooting

### Missing NetCDF library
If CMake can't find NetCDF, specify the path:
```bash
cmake -B build -DCMAKE_PREFIX_PATH=/usr/local
```

### Apple Silicon (M1/M2/M3)
Homebrew installs to `/opt/homebrew` on Apple Silicon. CMake should find it automatically,
but if not:
```bash
cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
```
