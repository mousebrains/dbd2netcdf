#ifndef INC_Header_H_
#define INC_Header_H_

// Jan-2012, Pat Welch, pat@mousebrains.com

#include <iosfwd>
#include <string>
#include <map>
#include <set>

class Header {
private:
  typedef std::map<std::string, std::string> tRecords;
  tRecords mRecords;
public:
  Header(std::istream& is, const char *fn);

  bool empty() const {return mRecords.empty();}

  std::string find(const std::string& key) const;
  int findInt(const std::string& key) const;

  std::string crc() const {return find("sensor_list_crc");}
  int nSensors() const {return findInt("total_num_sensors");}
  bool qFactored() const {return findInt("sensor_list_factored");}

  static std::string trim(std::string str);

  typedef std::set<std::string> tMissions;
  static void addMission(std::string name, tMissions& missionList);
  bool qProcessMission(const tMissions& toSkip, const tMissions& toProcess) const;

  friend std::ostream& operator << (std::ostream& os, const Header& hdr);
}; // Header

#endif // INC_Header_H_

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
