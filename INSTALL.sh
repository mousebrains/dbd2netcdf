#!/bin/sh
#
# Build, test, and install dbd2netcdf
#
# Usage:
#   ./INSTALL.sh                     # configure, build, test (no install)
#   ./INSTALL.sh --prefix /usr/local # configure, build, test, install
#   ./INSTALL.sh --clean             # remove build directory and rebuild
#   ./INSTALL.sh --jobs 8            # parallel build with 8 jobs
#
# See INSTALL.md for prerequisite packages.

set -e

SRCDIR="$(cd "$(dirname "$0")" && pwd)"
BUILDDIR="$SRCDIR/build"
BUILD_TYPE=Release
PREFIX=""
CLEAN=false
JOBS=""

usage() {
  echo "Usage: $0 [options]"
  echo ""
  echo "Options:"
  echo "  --prefix DIR   Install to DIR (skip install if omitted)"
  echo "  --build-type T CMake build type: Release, Debug, RelWithDebInfo (default: Release)"
  echo "  --jobs N       Parallel build jobs (default: auto-detect)"
  echo "  --clean        Remove build directory before building"
  echo "  -h, --help     Show this help"
  exit 0
}

while [ $# -gt 0 ]; do
  case "$1" in
    --prefix)     PREFIX="$2";     shift 2 ;;
    --build-type) BUILD_TYPE="$2"; shift 2 ;;
    --jobs)       JOBS="$2";       shift 2 ;;
    --clean)      CLEAN=true;      shift   ;;
    -h|--help)    usage ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      ;;
  esac
done

# Detect parallel jobs if not specified
if [ -z "$JOBS" ]; then
  if command -v nproc >/dev/null 2>&1; then
    JOBS=$(nproc)
  elif command -v sysctl >/dev/null 2>&1; then
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 1)
  else
    JOBS=1
  fi
fi

# Clean if requested
if [ "$CLEAN" = true ] && [ -d "$BUILDDIR" ]; then
  echo "==> Removing $BUILDDIR"
  rm -rf "$BUILDDIR"
fi

# Configure
echo "==> Configuring ($BUILD_TYPE)"
cmake_args="-B $BUILDDIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE"
if [ -n "$PREFIX" ]; then
  cmake_args="$cmake_args -DCMAKE_INSTALL_PREFIX=$PREFIX"
fi
cmake -S "$SRCDIR" $cmake_args

# Build
echo "==> Building (jobs=$JOBS)"
cmake --build "$BUILDDIR" -j "$JOBS"

# Test
echo "==> Running tests"
ctest --test-dir "$BUILDDIR" --output-on-failure

# Install
if [ -n "$PREFIX" ]; then
  echo "==> Installing to $PREFIX"
  cmake --install "$BUILDDIR"
  echo "Installed: $PREFIX/bin/dbd2netCDF"
  echo "Installed: $PREFIX/bin/dbd2csv"
  echo "Installed: $PREFIX/bin/dbdSensors"
  echo "Installed: $PREFIX/bin/pd02netCDF"
  echo "Installed: $PREFIX/bin/decompressTWR"
else
  echo ""
  echo "Build and tests succeeded. Binaries are in $SRCDIR/bin/"
  echo "To install, re-run with --prefix, e.g.:"
  echo "  $0 --prefix /usr/local"
fi
