AC_PREREQ(2.61)
AC_INIT([dbd2netCDF],[1.0],[pat@mousebrains.com])
AC_CONFIG_SRCDIR([Data.C])
AC_CONFIG_HEADER([config.h])

AC_LANG(C++)

# Checks for programs.
AC_PROG_CXX
AC_PATH_PROG([NCCONFIG], [nc-config], [AC_MSG_ERROR([nc-config not found])])

# Checks for libraries.

# Checks for header files.
AC_HEADER_DIRENT
AC_CHECK_HEADERS([arpa/inet.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_HEADER_STDC
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([floor mkdir realpath regcomp rmdir strdup strerror])

# Check for the c++4 library for netcdf

SAVED_LDFLAGS=$LDFLAGS
MYLDFLAGS="`$NCCONFIG --libs` -lnetcdf_c++4"
LDFLAGS="$LDFLAGS $MYLDFLAGS"
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <netcdf>],
                   [netCDF::NcFile()],)],
  [LIBS="$LIBS $MYLDFLAGS"],
  [AC_MSG_ERROR([netcdf_c++4 library not found])])
LDFLAGS=$SAVED_LDFLAGS

# Check for hdf5 library

AC_CHECK_LIB([hdf5], [H5close], [], [AC_MSG_ERROR([hdf5 library not found])])

AM_INIT_AUTOMAKE

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
