#ifndef INC_Sensors_H_
#define INC_Sensors_H_

// Jan-2012, Pat Welch, pat@mousebrains.com

#include "Sensor.H"
#include <iosfwd>
#include <unordered_set>
#include <vector>

class Header;

class Sensors {
private:
  typedef std::vector<Sensor> tSensors;
  tSensors mSensors;

  std::string mCRC; // sensor block CRC, as given by the header
  size_t mLength; // number of bytes in file the sensor block occupies
  size_t mnToStore; // Number of entries to store in data arry

  std::string mkFilename(const std::string& dir) const;
  std::string crcLower() const;
public:
  Sensors() : mLength(0), mnToStore(0) {}

  Sensors(std::istream& is, const Header& hdr);

  bool dump(const std::string& dir) const;
  bool load(const std::string& dir, const Header& hdr);

  const std::string& crc() const {return mCRC;}
  size_t length() const {return mLength;}
  size_t nToStore() const {return mnToStore;}
  void nToStore(const size_t i) {mnToStore = i;}

  typedef tSensors::size_type size_type;
  typedef tSensors::const_iterator const_iterator;
  typedef tSensors::iterator iterator;

  const_iterator begin() const {return mSensors.begin();}
  const_iterator end() const {return mSensors.end();}

  iterator begin() {return mSensors.begin();}
  iterator end() {return mSensors.end();}

  size_type size() const {return mSensors.size();}
  bool empty() const {return mSensors.empty();}

  const Sensor& operator [] (const size_type index) const {return mSensors[index];}
  Sensor& operator [] (const size_type index) {return mSensors[index];}

  void clear() {mSensors.clear();}
  void insert(const Sensor& sensor) {mSensors.push_back(sensor);}

  typedef std::unordered_set<std::string> tNames;
  static void loadNames(const char *fn, tNames& names);
  void qKeep(const tNames& names);
  void qCriteria(const tNames& names);

  typedef std::pair<size_type, bool> tItem;
  typedef std::vector<tItem> tMap;
  tMap mapSensors(const Sensors& others);

  friend std::ostream& operator << (std::ostream& os, const Sensors& sensors);
}; // Sensors

#endif // INC_Sensors_H_

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
