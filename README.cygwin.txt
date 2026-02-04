INSTALLATION OF DBD2NETCDF ON WINDOWS CYGWIN

::BEFORE INSTALLATION::
The following packages should be installed on Cygwin
(GCC 8+ is required for C++17 support):
  gcc-core (>=8)
  gcc-g++ (>=8)
  cmake
  make
  netcdf (includes ncdump)
  libnetcdf-devel
  hdf5
  libhdf5-devel
  diffutils (for tests)
  coreutils (for tests: tail, etc.)
  grep (for tests)
  file (for tests)
  tar

To install the above packages, run the Cygwin setup executable and
while in the packages selection screen, find the packages above
using the following searches:
  gcc
  make
  cmake
  netcdf
  hdf5
  diffutils
  coreutils
  grep
  file
  tar

Click the circle with arrows next to each appropriate package to install
the latest version. Then finish the Cygwin installation.


::INSTALLATION FROM SOURCE::

1. Download the source tarball from GitHub:
   https://github.com/mousebrains/dbd2netcdf/releases

   Save dbd2netcdf-#.#.#-Source.tar.gz to a directory under the Cygwin
   file system (e.g., /home/username/), where #.#.# is the version number.

NOTE: Lines below with a $ indicate a Cygwin prompt and the command to run.

2. From the Cygwin terminal, unpack the tarball:
   $ tar -zxvf dbd2netcdf-#.#.#-Source.tar.gz

3. Change into the directory that was just extracted:
   $ cd dbd2netcdf-#.#.#-Source

4. Configure and build:
   $ cmake -B build -DCMAKE_BUILD_TYPE=Release
   $ cmake --build build

5. Run tests:
   $ cd build && ctest --output-on-failure

6. Install (optional):
   $ cmake --install build --prefix /usr/local

If each command exits successfully, the install is complete.


::USAGE::

See the man pages for usage:
  $ man dbd2netCDF
  $ man dbd2csv
  $ man dbdSensors

Or use --help:
  $ dbd2netCDF --help


::TROUBLESHOOTING::

If tests fail with "$'\r': command not found" errors, the shell scripts
have Windows line endings. This can happen if git is configured with
core.autocrlf=true. The repository includes a .gitattributes file to
prevent this, but if needed you can convert manually:
  $ sed -i 's/\r$//' test/dbd2netCDF test/dbd2csv test/dbdSensors test/pd02netCDF
