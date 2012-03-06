#ifndef INC_DataColumn_H_
#define INC_DataColumn_H_

// Jan-2012, Pat Welch, pat@mousebrains.com

#include <iosfwd>
#include <vector>
#include <stdint.h>

class KnownBytes;

class DataColumn {
private:
  size_t mId;

  typedef std::vector<char> tqSet;
  tqSet mqSet;

  typedef std::vector<double> tDouble;
  typedef std::vector<float> tFloat;
  typedef std::vector<int16_t> tInt16;
  typedef std::vector<int8_t> tInt8;

  tDouble mDouble;
  tFloat mFloat;
  tInt16 mInt16;
  tInt8 mInt8;
public:
  DataColumn(const size_t id, const size_t sz);

  size_t id() const {return mId;}

  typedef tqSet::size_type  size_type;
  size_type size() const {return mqSet.size();}

  bool qSet(const size_t index) const {return mqSet[index];}

  double getDouble(const size_t index) const;
  float getFloat(const size_t index) const;
  int16_t getInt16(const size_t index) const;
  int8_t getInt8(const size_t index) const;

  void setDouble(const size_t index, const double value);
  void setFloat(const size_t index, const float value);
  void setInt16(const size_t index, const int16_t value);
  void setInt8(const size_t index, const int8_t value);

  void set(const size_t lhsIndex, const size_t rhsIndex, const DataColumn& rhs);

  void read(const size_t index, std::istream& is, const KnownBytes& kb);

  void resize(const size_t sz);

  std::string toStr(const size_t index) const;
}; // DataColumn

std::ostream& operator << (std::ostream& os, const DataColumn& d);

#endif // INC_DataColumn_H_

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
