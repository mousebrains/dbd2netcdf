#ifndef INC_Sensor_H_
#define INC_Sensor_H_

// Jan-2012, Pat Welch, pat@mousebrains.com

#include <iosfwd>
#include <string>

class KnownBytes;

class Sensor {
private:
  std::string mName; // Sensor name
  std::string mUnits; // Units of data
  int mSize;       // Number of bytes for this sensor
  int mIndex;      // Index into sensor array
  bool mqAvailable; // This sensor is available in the data stream
  bool mqKeep;     // Should this sensor be output?
  bool mqCriteria; // Is this a criteria sensor to select on

  void procLine(const std::string& line);
public:
  explicit Sensor(std::istream& is);
  explicit Sensor(const std::string& line);

  const std::string& name() const {return mName;}
  const std::string& units() const {return mUnits;}
  int size() const {return mSize;}
  int index() const {return mIndex;}
  void index(const int i) {mIndex = i;}
  bool qAvailable() const {return mqAvailable;}
  bool qKeep() const {return mqKeep;}
  void qKeep(const bool q) {mqKeep = q;}
  bool qCriteria() const {return mqCriteria;}
  void qCriteria(const bool q) {mqCriteria = q;}

  double read(std::istream& is, const KnownBytes& kb) const;

  std::string toStr(const double value) const;

  void dump(std::ostream& os) const;

  friend std::ostream& operator << (std::ostream& os, const Sensor& hdr);
}; // Sensor

#endif // INC_Sensor_H_

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
