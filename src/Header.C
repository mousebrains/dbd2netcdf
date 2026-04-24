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

#include "Header.H"
#include "MyException.H"
#include "Logger.H"
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <limits>

namespace {
  std::string tolower(std::string str) {
    for (std::string::size_type i(0), e(str.size()); i < e; ++i) {
      str[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(str[i])));
    }
    return str;
  }
} // Anonymous namespace

Header::Header(std::istream& is, const char *fn)
{
  size_t cnt = 0;
  // 14 is the typical ASCII header length in DBD files; num_ascii_tags (if
  // present in the first 14 lines) overrides this with the exact count.
  for (tRecords::size_type nLines(14); mRecords.size() < nLines;) {
    std::string line;
    if (!getline(is, line)) {
      break;
    }
    const std::string::size_type index(line.find(':'));
    ++cnt;
    if (index == line.npos) {
      LOG_WARN("Missing colon in '{}' on line {}: '{}'", fn, cnt, line.substr(0, 10));
      mRecords.clear();
      return;
    }
    const std::string key(trim(line.substr(0, index)));
    const std::string value(trim(line.substr(index + 1)));
    mRecords.insert(std::make_pair(key, value));
    if (key == "num_ascii_tags") {
      try {
        const int parsed = std::stoi(value);
        nLines = (parsed > 0 && parsed <= 10000)
                     ? static_cast<tRecords::size_type>(parsed)
                     : 0;
      } catch (const std::exception&) {
        nLines = 0;  // Default to 0 on parse error
      }
    }
  }
}

std::string
Header::find(const std::string& key) const
{
  tRecords::const_iterator it(mRecords.find(key));

  return (it == mRecords.end()) ? std::string() : it->second;
}

int
Header::findInt(const std::string& key) const
{
  const std::string value(find(key));

  if (value.empty()) return 0;

  try {
    return std::stoi(value);
  } catch (const std::exception&) {
    return 0;  // Default to 0 on parse error
  }
}

std::string
Header::trim(std::string str)
{
  const std::string whitespace(" \t\n");

  const std::string::size_type first(str.find_first_not_of(whitespace));
  if (first == str.npos) {
    return std::string();
  }

  const std::string::size_type last(str.find_last_not_of(whitespace));
  return str.substr(first, last - first + 1);
}

void
Header::addMission(std::string mission,
                   tMissions& missions)
{
  missions.insert(tolower(mission));
}

bool
Header::qProcessMission(const tMissions& toSkip,
                        const tMissions& toKeep) const
{
  if (toSkip.empty() && toKeep.empty()) {
    return true;
  }

  const std::string mission(tolower(find("mission_name")));

  if (!toSkip.empty() && (toSkip.find(mission) != toSkip.end())) {
    return false;
  }

  return toKeep.empty() || (toKeep.find(mission) != toKeep.end());
}

// Parse fileopen_time header value (format: "Day_Mon_DD_HH:MM:SS_YYYY") to UTC epoch.
// Assumes all timestamps are UTC (standard for Slocum glider data at sea).
time_t
Header::parseFileOpenTime(const std::string& timeStr)
{
  if (timeStr.empty()) return std::numeric_limits<time_t>::max();
  std::string s(timeStr);
  std::replace(s.begin(), s.end(), '_', ' ');
  struct tm tm = {};
  std::istringstream iss(s);
  iss >> std::get_time(&tm, "%a %b %d %H:%M:%S %Y");
  if (iss.fail()) return std::numeric_limits<time_t>::max();
#ifdef _WIN32
  return _mkgmtime(&tm);
#else
  return timegm(&tm);
#endif
}

std::ostream&
operator << (std::ostream& os,
             const Header& hdr)
{
  for (Header::tRecords::const_iterator it(hdr.mRecords.begin()), et(hdr.mRecords.end()); it != et; ++it) {
    os << it->first << " '" << it->second << "'" << std::endl;
  }

  return os;
}
