#ifndef _INC_StackDumpExit_H_
#define _INC_StackDumpExit_H_

// Jan-2012, Pat Welch, pat@mousebrains.com

#include <string>

void stackDumpExit(int rc = 1, const std::string& msg = std::string());

#endif // _INC_StackDumpExit_H_

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
