INSTALLATION OF DBD2NETCDF ON WINDOWS 7 CYGWIN

::BEFORE INSTALLATION::
the following packages should be installed on Cygwin:
  gcc4-core
  gcc4-g++
  cmake
  make
  netCDF
  libnetCDF latest
  libnetCDF-devel
  hdf5
  libhdf5 latest
  libhdf5-devel

to install the above packages, run the setup.exe installer for Cygwin
and while in the packages selection screen, find the packages above
using the following searches:
  gcc
  make
  netCDF
  hdf5

click the circle with arrows next to each appropriate package to install
the latest version.  Now finish the cygwin installation.


::INSTALLATION::
1. download and save the source tarball dbd2netcdf-#.#.#-Source.tar.gz to
/usr/local/ under the Cygwin file system from 
http://sourceforge.net/projects/dbd2netcdf/files/,
where #.#.# is replaced by the version number downloaded.
On a typical Cygwin install \usr\local\ would be found at
C:\cygwin\usr\local\ using the Windows file system.

NOTE: lines below with a $ indicate a Cygwin prompt and the command to run
i.e.
$ command

2. from the Cygwin terminal unpack the tarball:
$ tar -zxvf dbd2netcdf-#.#.#-Source.tar.gz

3. change into the directory that was just extracted:
$ cd dbd2netcdf-#.#.#-Source

4. run the make steps to build and install:
$ cmake .

$ make

$ make test

$ make install

If each of the 4 commands above exit successfully, the install is complete.
dbd2netCDF is now on the Cygwin path and is accessible from any directory.
See:
$ man dbd2netCDF
for how to use dbd2netCDF.
