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
  for (tRecords::size_type nLines(10); mRecords.size() < nLines;) {
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
        nLines = std::stoi(value);
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

  std::string::size_type index(str.find_first_not_of(whitespace));

  if (index != str.npos) {
    str = str.substr(index);
  }

  index = str.find_last_not_of(whitespace);

  if (index != str.npos) {
    str = str.substr(0, index + 1);
  }

  return str;
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

std::ostream&
operator << (std::ostream& os,
             const Header& hdr)
{
  for (Header::tRecords::const_iterator it(hdr.mRecords.begin()), et(hdr.mRecords.end()); it != et; ++it) {
    os << it->first << " '" << it->second << "'" << std::endl;
  }

  return os;
}
