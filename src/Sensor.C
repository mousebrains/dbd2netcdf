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

#include "Sensor.H"
#include "KnownBytes.H"
#include "MyException.H"
#include <iostream>
#include <sstream>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>

Sensor::Sensor(std::istream& is)
  : mqKeep(true)
  , mqCriteria(true)
{
  std::string line;

  if (!getline(is, line)) {
    std::ostringstream oss;
    oss << "Invalid getline while reading a sensor, " << strerror(errno);
    throw MyException(oss.str());
  }

  procLine(line);
}

Sensor::Sensor(const std::string& line)
  : mqKeep(true)
  , mqCriteria(true)
{
  procLine(line);
}

void
Sensor::procLine(const std::string& line)
{
  std::istringstream iss(line);

  std::string prefix;
  std::string qUsed;
  int index;

  if (!(iss >> prefix >> qUsed >> index >> mIndex >> mSize >> mName >> mUnits)) {
    std::ostringstream oss;
    oss << "Malformed sensor line '" << line << "'";
    throw MyException(oss.str());
  }

  if (prefix != "s:") {
    std::ostringstream oss;
    oss << "Malformed sensor line '" << line << "'";
    throw MyException(oss.str());
  }

  mqAvailable = qUsed == "T";
}

double
Sensor::read(std::istream& is,
             const KnownBytes& kb) const
{
  double val(NAN);

  switch (mSize) {
    case 1: val = (double) kb.read8(is); break;
    case 2: val = (double) kb.read16(is); break;
    case 4: val = (double) kb.read32(is); break;
    case 8: val = kb.read64(is); break;
    default:
      std::ostringstream oss;
      oss << "Unknown number of bytes(" << mSize << " for sensor " << mName;
      throw MyException(oss.str());
  }

  return val;
}

std::string
Sensor::toStr(const double value) const
{
  const char *format((mSize == 8) ? "%.15g" : ((mSize == 4) ? "%.7g" : "%.0f"));
  char buffer[256];
  snprintf(buffer, sizeof(buffer), format, value);
  return buffer;
}

void
Sensor::dump(std::ostream& os) const
{
  os << "s:"
     << " " << (mqAvailable ? "T" : "F")
     << " " << mIndex
     << " " << mIndex
     << " " << mSize
     << " " << mName
     << " " << mUnits
     << std::endl;
}

std::ostream&
operator << (std::ostream& os,
             const Sensor& sen)
{
  os << sen.mIndex << ' ' << sen.mSize << ' ' << sen.mName << ' ' << sen.mUnits;

  return os;
}
