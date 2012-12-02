// Nov-2012, Pat Welch, pat@mousebrains.com

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

// tokenize a string

#include "Tokenize.H"
#include <iostream>

dbd::tTokens
dbd::tokenize(const std::string& str,
              const std::string& delim)
{
  tTokens tokens;

  for (std::string::size_type offset(0), index(str.find_first_not_of(delim)); 
       index != str.npos; index = str.find_first_not_of(delim, offset)) {
    const std::string::size_type eindex(str.find_first_of(delim, index));
    if (eindex == str.npos) {
      tokens.push_back(str.substr(index));
      return tokens;
    }
    tokens.push_back(str.substr(index, eindex - index));
    offset = eindex;
  }

  return tokens;
}

std::ostream&
operator << (std::ostream& os,
             const dbd::tTokens& t)
{
  std::string delim;

  os << '{';

  for (dbd::tTokens::const_iterator it(t.begin()), et(t.end()); it != et; ++it) {
    os << delim << *it;
    delim = std::string(",");
  }
  os << '}';

  return os;
}
