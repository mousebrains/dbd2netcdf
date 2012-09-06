#ifndef INC_SensorsMap_H_
#define INC_SensorsMap_H_

// Jan-2012, Pat Welch, pat@mousebrains.com

#include "Sensors.H"
#include <iosfwd>
#include <map>

class Header;

class SensorsMap {
private:
  const std::string mDir;

  typedef std::map<std::string, Sensors> tMap;
  tMap mMap;

  Sensors mAllSensors;
public:
  SensorsMap() {}

  explicit SensorsMap(const std::string& dir) : mDir(dir) {}

  const Sensors& find(const Header& hdr);
  void insert(std::istream& is, const Header& hdr, const bool qPosition);

  void setUpForData();
  const Sensors& allSensors() const {return mAllSensors;}

  void qKeep(const Sensors::tNames& names);
  void qCriteria(const Sensors::tNames& names);

  friend std::ostream& operator << (std::ostream& os, const SensorsMap& sensors);
}; // SensorsMap

#endif // INC_SensorsMap_H_

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
