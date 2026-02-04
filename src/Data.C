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

#include "Data.H"
#include "KnownBytes.H"
#include "Sensors.H"
#include "MyException.H"
#include "Logger.H"
#include <iostream>
#include <sstream>
#include <cmath>
#include <vector>

Data::Data(std::istream& is,
           const KnownBytes& kb,
           const Sensors& sensors,
	   const bool qRepair,
	   const size_t nBytes)
  : mDelim(" ")
{
  load(is, kb, sensors, qRepair, nBytes);
}

void
Data::load(std::istream& is,
           const KnownBytes& kb,
           const Sensors& sensors,
	   const bool qRepair,
	   const size_t nBytes)
{
  const size_t nSensors(sensors.size());
  const size_t nHeader((nSensors + 3) / 4);
  std::vector<int8_t> bits(nHeader);  // RAII - automatic cleanup
  const tRow empty(sensors.nToStore(), NAN);
  tRow prevValue(empty);
  tData::size_type nRows(0);
  const size_t dSize(2 * nBytes / (nHeader + 1) + 1); // 2 is for compression


  mData.resize(dSize, empty); // Initial allocation

  while (true) { // Walk through the file
    int8_t tag;
    if (!is.read(reinterpret_cast<char*>(&tag), 1)) { // EOF while reading tag byte
      break;
    }

    if (tag == 'X') { // End-of-data tag
        break;
    }

    if (tag != 'd') {
      // Not a data tag, so assume we've encountered garbage and look for a data tag
      const size_t pos = is.tellg(); // Where the bad tag was found
      bool qContinue = false;
      while (true) { // look for the next d
          int8_t c;
	  if (!is.read(reinterpret_cast<char*>(&c), 1)) { // EOF looking for the next 'd'
	    break;
	  }
	  if (c == 'd') {
	    qContinue = true;
	    break;
	  }
      }
      if (!qRepair || !qContinue) { // Not repairing or didn't find a 'd' to look at
        mData.resize(nRows); // Prune off unused rows
        std::ostringstream oss;
        oss << "Unknown data tag(0x"
            << std::hex << (tag & 0xff) << std::dec
            << ") '";
        if (tag >= 0x20 && tag <= 0x7E) {
          oss << static_cast<char>(tag & 0xff);
        } else {
          oss << "GotMe";
        }
        oss << "' should be either 'd' or 'X' at offset " << pos;
	oss << ", No more 'd' characters found";
        // bits vector automatically cleaned up
        throw(MyException(oss.str()));
      } // if !qContinue
      const size_t nPos = is.tellg();
      std::string tagStr = (tag >= 0x20 && tag <= 0x7E)
          ? std::string(1, static_cast<char>(tag & 0xff))
          : "GotMe";
      LOG_WARN("Skipped {} bytes due to bad data tag (0x{:02x}) '{}' at offset {}",
               nPos - pos, tag & 0xff, tagStr, pos);
    }

    if (!is.read(reinterpret_cast<char*>(bits.data()), nHeader)) {
      mData.resize(nRows); // Prune off unused rows
      std::ostringstream oss;
      oss << "EOF reading " << nHeader << " bytes for header bits";
      // bits vector automatically cleaned up
      throw(MyException(oss.str()));
    }

    if ((mData.size() - 1) <= nRows) mData.resize(mData.size()+dSize, empty);

    tRow& row(mData[nRows]);
    bool qKeep(false);

    for (size_t i(0); i < nSensors; ++i) {
      const size_t offIndex(i >> 2);
      if (offIndex >= nHeader) {
        mData.resize(nRows); // Prune off unused rows
        std::ostringstream oss;
        oss << "offIndex issue " << offIndex << " >= " << nHeader
            << " at " << is.tellg();
        // bits vector automatically cleaned up
        throw(MyException(oss.str()));
      }
      const size_t offBits(6 - ((i & 0x3) << 1));
      const unsigned int code((bits[offIndex] >> offBits) & 0x03);
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

  mData.resize(nRows); // Prune off unused rows
  // bits vector automatically cleaned up on function exit
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
