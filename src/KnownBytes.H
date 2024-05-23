#ifndef INC_KnownBytes_H_
#define INC_KnownBytes_H_

// Jan-2012, Pat Welch, pat@mousebrains.com

#include <iosfwd>
#include <cstddef>
#include <stdint.h> // Use this instead of cstdint

class KnownBytes {
private:
  bool mFlip;
public:
  KnownBytes(std::istream& is);

  size_t length() const {return 16;}

  int8_t read8(std::istream& is) const;
  int16_t read16(std::istream& is) const;
  float read32(std::istream& is) const;
  double read64(std::istream& is) const;
}; // KnownBytes

#endif // INC_KnownBytes_H_

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
