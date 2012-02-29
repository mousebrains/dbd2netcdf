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

#include <Data.H>
#include <KnownBytes.H>
#include <Sensors.H>
#include <MyException.H>
#include <iostream>
#include <cmath>

Data::Data(std::istream& is,
           const KnownBytes& kb,
           const Sensors& sensors)
  : mDelim(" ")
{
  load(is, kb, sensors);
}

void
Data::load(std::istream& is,
           const KnownBytes& kb,
           const Sensors& sensors)
{
  const size_t nSensors(sensors.size());
  const size_t nHeader((nSensors + 3) / 4);
  int8_t *bits((int8_t * ) malloc(nHeader));
  const tRow empty(sensors.nToStore(), NAN);
  tRow prevValue(empty);
  tData::size_type nRows(0);

  { // Get maximum number of records
    const std::streampos spos(is.tellg()); // Current position
    is.seekg(0, std::ios::end); // Move to end of file
    const size_t nFileSize(is.tellg() - spos); // Number of data bytes
    is.seekg(spos, std::ios::beg); // Move back to start of the data
    const size_t nRecords(nFileSize / (nHeader + 1) + 1);
    mData.resize(nRecords, empty);
  }

  while (true) { // Walk through the file
    int8_t tag;
    if (!is.read((char *) &tag, 1)) {
      throw(MyException("Error reading tag byte"));
    }
    if (tag != 'd') {
      if (tag == 'X') {
        break;
      }
      mData.resize(nRows); // Prune off unused rows
      char buffer[2048];
      const size_t pos(is.tellg()); // Current position
      snprintf(buffer, sizeof(buffer), "Unknown data tag(%x '%c') should be either 'd' or 'X' at offset %lu", 
               tag & 0xff, tag, pos);
      free(bits);
      throw(MyException(buffer));
    }

    if (!is.read((char *) bits, nHeader)) {
      mData.resize(nRows); // Prune off unused rows
      char buffer[2048];
      snprintf(buffer, sizeof(buffer), "Error reading %lu bytes for header bits", sizeof(bits));
      throw(MyException(buffer));
    }

    tRow& row(mData[nRows]);
    bool qKeep(false);

    for (size_t i(0); i < nSensors; ++i) {
      const size_t offIndex(i >> 2);
      const size_t offBits(6 - ((i & 0x3) << 1));
      const unsigned int code((bits[offIndex] >> offBits) & 0x03);
      if (offIndex >= nHeader) {
        mData.resize(nRows); // Prune off unused rows
        char buffer[2048];
        snprintf(buffer, sizeof(buffer), "offIndex issue %lu >= %lu at %lu", 
                 offIndex, nHeader, (size_t) is.tellg());
        throw(MyException(buffer));
      }
      if (code == 1) { // Repeat previous value
        const Sensor& sensor(sensors[i]);
        const size_t index(sensor.index());
        qKeep |= sensor.qCriteria();
        if (sensor.qKeep()) {
          row[index] = prevValue[index];
        }
      } else if (code == 2) { // New Value
        const Sensor& sensor(sensors[i]);
        const size_t index(sensor.index());
        const double value(sensor.read(is, kb));
        qKeep |= sensor.qCriteria();
        if (sensor.qKeep()) {
          row[index] = value;
          prevValue[index] = value;
        }
      }
    }

    if (qKeep)
      ++nRows;
  }

  free(bits);
  mData.resize(nRows); // Prune off unused rows

}

std::ostream&
operator << (std::ostream& os,
             const Data& data)
{
  for (Data::tData::size_type i(0), ie(data.mData.size()); i < ie; ++i) {
    std::string space;
    const Data::tRow& row(data.mData[i]);
    for (Data::tRow::size_type j(0), je(row.size()); j < je; ++j) {
      os << space << row[j];
      space = data.mDelim;
    }
    os << std::endl;
  }

  return os;
}
