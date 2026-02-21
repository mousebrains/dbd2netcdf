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

#include "SensorsMap.H"
#include "Header.H"
#include "MyException.H"
#include <unordered_map>
#include <iostream>
#include <cstdio>

const Sensors&
SensorsMap::find(const Header& hdr)
{
  tMap::const_iterator it(mMap.find(hdr.crc()));

  if (it == mMap.end()) {
    Sensors sensors;
    if (sensors.load(mDir, hdr)) {
      it = mMap.insert(std::make_pair(sensors.crc(), sensors)).first;
      return it->second;
    }
    char buffer[2048];
    snprintf(buffer, sizeof(buffer), "Known sensors do not include '%s'", hdr.crc().c_str());
    throw(MyException(buffer));
  }

  return it->second;
}

void
SensorsMap::insert(std::istream& is,
                   const Header& hdr,
                   const bool qPosition)
{
  const std::string crc(hdr.crc());

  tMap::const_iterator it(mMap.find(crc));

  if (it == mMap.end()) {
    Sensors sensors(is, hdr);

    if (!sensors.empty()) {
      sensors.dump(mDir);
    } else { // sensors is empty, so try and load from cache
      sensors.load(mDir, hdr);
    }

    if (!sensors.empty()) {
      mMap.insert(std::make_pair(sensors.crc(), sensors));
    }

    return;
  }

  if (qPosition && !hdr.qFactored()) { // Read in nSensors worth of lines, but skip processing
    for (size_t i = hdr.nSensors(); i; --i) {
      std::string line;
      getline(is, line);
    }
  }
}

void
SensorsMap::setUpForData()
{
  mAllSensors.clear();

  if (mMap.empty())
    return;

  // Even if there is only one, we still do this to update indices

  typedef std::unordered_map<std::string, size_t> tNames;
  tNames names;

  for (tMap::iterator it(mMap.begin()), et(mMap.end()); it != et; ++it) {
    Sensors& sensors(it->second);
    for (Sensors::iterator jt(sensors.begin()), jet(sensors.end()); jt != jet; ++jt) {
      Sensor& sensor(*jt);
      if (sensor.qKeep()) {
        tNames::const_iterator nt(names.find(sensor.name()));
        if (nt != names.end()) { // Already known
          sensor.index(static_cast<int>(nt->second));
        } else { // Not seen yet
          sensor.index(static_cast<int>(names.size()));
          names.insert(std::make_pair(sensor.name(), static_cast<int>(names.size())));
          mAllSensors.insert(sensor);
        }
      }
    }
  }

  mAllSensors.nToStore(names.size());

  for (tMap::iterator it(mMap.begin()), et(mMap.end()); it != et; ++it) {
    Sensors& sensors(it->second);
    sensors.nToStore(names.size());
  }
}

void
SensorsMap::qKeep(const Sensors::tNames& names)
{
  if (!names.empty()) {
    for (tMap::iterator it(mMap.begin()), et(mMap.end()); it != et; ++it) {
      it->second.qKeep(names);
    }
  }
}

void
SensorsMap::qCriteria(const Sensors::tNames& names)
{
  if (!names.empty()) {
    for (tMap::iterator it(mMap.begin()), et(mMap.end()); it != et; ++it) {
      it->second.qCriteria(names);
    }
  }
}

std::ostream&
operator << (std::ostream& os,
             const SensorsMap& sen)
{
  for (SensorsMap::tMap::const_iterator it(sen.mMap.begin()), et(sen.mMap.end()); it != et; ++it) {
    os << "CRC " << it->first << std::endl;
    os << it->second << std::endl;
  }

  return os;
}
