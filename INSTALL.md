# Building dbd2netcdf from Source

## Prerequisites

Install NetCDF and HDF5 development packages before building.

**macOS (Homebrew):**
```sh
brew install netcdf hdf5
```

**Ubuntu/Debian:**
```sh
sudo apt install cmake g++ libnetcdf-dev libhdf5-dev netcdf-bin
```

**RHEL/Alma/Rocky 8:**
```sh
sudo dnf install epel-release
sudo dnf config-manager --set-enabled powertools
sudo dnf install cmake gcc-c++ netcdf-devel hdf5-devel
```

**RHEL/Rocky 9:**
```sh
sudo dnf install epel-release
sudo dnf config-manager --set-enabled crb
sudo dnf install cmake gcc-c++ netcdf-devel hdf5-devel
```

**Cygwin:**
```sh
# Install via Cygwin setup: cmake, gcc-g++, libnetcdf-devel, libhdf5-devel
```

## Build Steps

1. Clone the repository:
   ```sh
   git clone https://github.com/mousebrains/dbd2netcdf.git
   cd dbd2netcdf
   ```

2. Configure with CMake:
   ```sh
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```

3. Build:
   ```sh
   cmake --build build
   ```

4. Run tests:
   ```sh
   ctest --test-dir build --output-on-failure
   ```

5. Install (optional):
   ```sh
   cmake --install build
   ```

   Or specify a custom prefix:
   ```sh
   cmake --install build --prefix /usr/local
   ```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build type (Debug, Release, RelWithDebInfo) |
| `BUILD_TESTING` | ON | Build unit tests |
| `BUILD_FUZZ_TESTS` | OFF | Build fuzz tests (requires Clang with libFuzzer) |

Example with options:
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
```

## Cleaning and Rebuilding

Clean before building (incremental clean):
```sh
cmake --build build --clean-first
```

Clean only (remove compiled objects):
```sh
cmake --build build --target clean
```

Full clean (remove entire build directory):
```sh
rm -rf build
```

To change build options, reconfigure and rebuild:
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --clean-first
```
