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

#include <StackDump.H>
#include <iostream>
#include <execinfo.h>

void
stackDumpExit(int rc,
              const std::string& msg)
{
  if (!msg.empty()) {
    std::cerr << msg << std::endl;
  }

  const size_t n(10);
  void *array[n];
  size_t size(backtrace(array, n));
  char **messages(backtrace_symbols(array, size));
  if (messages) {
    for (size_t i = 1; (i < size) && messages[i]; ++i) { // Skip my entry
      std::cout << messages[i] << std::endl;
    }
    free(messages);
  }
  exit(1);
}
