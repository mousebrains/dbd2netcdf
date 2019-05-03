To install this software on a Mac, you will need the following prerequisites:

# Setup /usr/local

* This software is designed to be installed into /usr/local
* Log into an admin user, if you are not one
* cd /usr/local
* chown -R USERNAME:staff # Where USERNAME is the account username you will be installing software from
* Log out of the admin user and back into your account, if they are different

# INSTALL XCode
* Install XCode, which can be gotten from the Mac App store by searching for xcode
* Start XCode
* One used to have to install the command line tools, but I don't think one does anymore
* Exit XCode

# INSTALL CMake
* Get CMake from cmake.org under the Download table select the Mac OS download
* Click on the DMG file and follow the install instructions.
* Launch CMake and under the Tools menu click on "How to Install For Command Line Use" and follow the appropriate set of instructions.

# INSTALL HDF5
* Download HDF5 source from hdfgroup.org
* Unpack the HDF5 into a directory you want to work. I typically use /usr/local/src
* cd into the HDF5 directory
* Look at release_docs/INSTALL for how to install the software, I use:
* ./configure --prefix=/usr/local
* make -j4
* make -j4 check
* make -j4 install
* make -j4 check-install
* make -j4 distclean

# INSTALL NetCDF
* Download NetCDF-C source from https://www.unidata.ucar.edu/software/netcdf
* Unpack the NetCDF source into a directory you want to build it in
* cd into the directory where you unpacked the source code
* Read the install instructions, INSTALL.md
* Here is what I do:
* ./configure --prefix=/usr/local
* make -j4
* make -j4 check
* make -j4 install
* make -j4 distclean

# INSTALL dbd2netcdf
* cd to the directory where you want to build dbd2netcdf
* Create a clone of the dbd2netcdf package using GIT
* git clone https://github.com/mousebrains/dbd2netcdf.git
* cd dbd2netcdf
* cmake .
* make -j4
* make -j4 check
* Many times the pd02netCDF test fails, I have not sorted out exactly why yet
* make -j4 install

# Run dbd2netcdf
* At this point you should have dbd2netcdf installed.
* For command line help issue the command:
* dbd2netcdf -h
* Let's say you have a directory foo with a lot of SBD files in it, then to create a NetCDF file, foo.nc, from all the SBD files I would do:
* dbd2netcdf -o foo.nc foo/\*.SBD
* Cache file management can be an issue if none of the SBD files contain the appropriate cache file. I typically create a directory, with all the cache files for all my deployments. For example, let's say that directory is cache. Then the above command will be:
* dbd2netcdf -C cache -o foo.nc foo/\*.SBD
* You can convert all the DBD files into a single NetCDF, but when working in Matlab, the number of variables slows things down a lot. So I build three NetCDF files from the DBD files, one with all the m\_ and c\_ sensors, another with all the sci\_ sensors, and the third with everything else.
* You can get the list of sensors using the dbdSensors command.

# UPDATE dbd2netCDF
* To keep your version of dbd2netcdf up to date do the following in the dbd2netcdf directory:
* git pull
* cmake .
* make
* make install
