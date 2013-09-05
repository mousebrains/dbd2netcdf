#ifndef INC_Tokenize_H_
#define INC_Tokenize_H_

// Nov-2012, Pat Welch, pat@mousebrains.com

#include <iosfwd>
#include <string>
#include <vector>

namespace dbd {
  typedef std::vector<std::string> tTokens;
  tTokens tokenize(const std::string& str, const std::string& delim = " \t\n\r");
} // namespace dbd

std::ostream& operator << (std::ostream& os, const dbd::tTokens& t);

#endif // INC_Tokenize_H_

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
