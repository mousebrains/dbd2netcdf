AC_PREREQ(2.61)
AC_INIT([dbd2netCDF],[1.0],[pat@mousebrains.com])
AC_CONFIG_SRCDIR([Data.C])
AC_CONFIG_HEADER([config.h])

AC_LANG(C++)

# Checks for programs.
AC_PROG_CXX
AC_PATH_PROG([NCCONFIG], [nc-config], [AC_MSG_ERROR([nc-config not found])])

# Check for compiler option support

for opt in -pedantic -Wall; do
  AC_MSG_CHECKING([for $opt])
  SAVED_CXXFLAGS=$CXXFLAGS
  CXXFLAGS="$CXXFLAGS $opt"
  AC_COMPILE_IFELSE([int main(void){return 0;}], [USE_OPT="yes"], [USE_OPT="no"])
  CXXFLAGS=$SAVED_CXXFLAGS
  if test "x$USE_OPT" = "xyes"; then
    CXXFLAGS="$CXXFLAGS $opt"
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no])
  fi
done

# Checks for libraries.

# Check for the c++4 library for netcdf

AC_MSG_CHECKING([for libnetcdf_c++4])
SAVED_LDFLAGS=$LDFLAGS
MYLDFLAGS="`$NCCONFIG --libs` -lnetcdf_c++4"
LDFLAGS="$LDFLAGS $MYLDFLAGS"
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <netcdf>],
                   [netCDF::NcFile()],)],
  [LIBS="$LIBS $MYLDFLAGS"],
  [AC_MSG_ERROR([netcdf_c++4 library not found])])
LDFLAGS=$SAVED_LDFLAGS
AC_MSG_RESULT([yes])

# Check for hdf5 library

AC_CHECK_LIB([hdf5], [H5close], [], [AC_MSG_ERROR([hdf5 library not found])])

# Check for linkage options

for opt in -Wl,-rpath=`$NCCONFIG --prefix`/lib; do
  AC_MSG_CHECKING([for $opt])
  SAVED_LDFLAGS=$LDFLAGS
  LDFLAGS="$LDFLAGS $opt"
  AC_LINK_IFELSE([int main(void){return 0;}], [USE_OPT="yes"], [USE_OPT="no"])
  LDFLAGS=$SAVED_LDFLAGS
  if test "x$USE_OPT" = "xyes"; then
    LDFLAGS="$LDFLAGS $opt"
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no])
  fi
done

# Checks for header files.
AC_HEADER_DIRENT
AC_CHECK_HEADERS([arpa/inet.h unistd.h, netcdf])

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

AM_INIT_AUTOMAKE

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
