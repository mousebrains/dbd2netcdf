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
  typedef std::vector<double> tColumn; // All records for one sensor
private:
  typedef std::vector<tColumn> tData;
  tData mData;       // mData[sensor_index][record_index]
  size_t mNRows;

  std::string mDelim;
public:
  Data() : mNRows(0), mDelim(" ") {}
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

  bool empty() const {return mNRows == 0;}
  size_t size() const {return mNRows;}          // Number of records
  size_t nColumns() const {return mData.size();} // Number of sensors

  // Column access (for NetCDF writer — contiguous memory)
  const tColumn& column(size_t j) const {return mData[j];}

  // Element access (for CSV writer)
  double operator()(size_t row, size_t col) const {return mData[col][row];}

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
