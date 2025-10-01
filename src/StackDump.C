// Jan-2012, Pat Welch, pat@mousebrains.com

/*
    This file is part of dbd2netCDF.

    dbd2netCDF is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    dbd2netCDF is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with dbd2netCDF.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StackDump.H"
#include "config.h"
#include <cstdlib>
#include <iostream>

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif // HAVE_EXECINFO_H

void
stackDumpExit(int rc,
              const std::string& msg)
{
  if (!msg.empty()) {
    std::cerr << msg << std::endl;
  }

#ifdef HAVE_BACKTRACE
  constexpr size_t MAX_STACK_FRAMES = 10;  // Maximum number of stack frames to capture
  void *array[MAX_STACK_FRAMES];
  size_t size(backtrace(array, MAX_STACK_FRAMES));

  // RAII wrapper for backtrace_symbols - automatically calls free()
  struct BacktraceRAII {
    char** ptr;
    explicit BacktraceRAII(void** array, size_t size) : ptr(backtrace_symbols(array, size)) {}
    ~BacktraceRAII() { if (ptr) free(ptr); }
    operator bool() const { return ptr != nullptr; }
    char* operator[](size_t i) const { return ptr[i]; }
  } messages(array, size);

  if (messages) {
    for (size_t i = 1; (i < size) && messages[i]; ++i) { // Skip my entry
      std::cout << messages[i] << std::endl;
    }
  }  // Automatic cleanup via ~BacktraceRAII()
#endif // HAVE_BACKTRACE

  exit(1);
}
