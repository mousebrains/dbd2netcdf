#ifndef INC_Data_H_
#define INC_Data_H_

// Jan-2012, Pat Welch, pat@mousebrains.com

#include <string>
#include <iosfwd>
#include <vector>

class KnownBytes;
class Sensors;

class Data {
public:
  typedef std::vector<double> tRow;
private:
  typedef std::vector<tRow> tData;
  tData mData;

  std::string mDelim;
public:
  Data() : mDelim(" ") {}
  Data(std::istream& is,
		  const KnownBytes& kb,
		  const Sensors& sensors,
		  const bool qRepair,
		  const size_t nBytes);

  void load(std::istream& is,
		  const KnownBytes& kb,
		  const Sensors& sensors,
		  const bool qRepair,
		  const size_t nBytes);

  bool empty() const {return mData.empty();}
  tData::size_type size() const {return mData.size();}

  typedef tData::const_iterator const_iterator;
  const_iterator begin() const {return mData.begin();}
  const_iterator end() const {return mData.end();}

  const tRow& operator [] (const tData::size_type i) const {return mData[i];}

  void delim(const std::string& str) {mDelim = str;}


  friend std::ostream& operator << (std::ostream& os, const Data& sensors);
}; // Data

#endif // INC_Data_H_

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
