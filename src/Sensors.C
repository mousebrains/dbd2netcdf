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

#include "config.h"
#include "Sensors.H"
#include "Header.H"
#include "StackDump.H"
#include "Decompress.H"
#include "FileInfo.H"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

Sensors::Sensors(std::istream& is,
                 const Header& hdr)
  : mCRC(hdr.crc())
  , mLength(0)
  , mnToStore(0)
{
  if (!hdr.qFactored()) {
    const std::streampos spos(is.tellg());

    for (int i(0), nSensors(hdr.nSensors()); i < nSensors; ++i) {
      const Sensor sensor(is);
      if (sensor.qAvailable()) {
        mSensors.push_back(sensor);
      }
    }

    mLength = is.tellg() - spos;
    mnToStore = mSensors.size();
  }
}

void
Sensors::loadNames(const char *fn,
                   tNames& names)
{
  std::ifstream is(fn);
  if (!is) {
    std::cerr << "Error opening '" << fn << "', " << strerror(errno) << std::endl;
    exit(1);
  }

  const std::string whiteSpace(", \t\n");

  for (std::string line; getline(is, line);) {
    for (std::string::size_type i(0), e(line.size()); i < e; ) {
      const std::string::size_type is(line.find_first_not_of(whiteSpace, i));
      if (is == line.npos) { // Nothing but whitespace left
        break;
      }
      const std::string::size_type ie(line.find_first_of(whiteSpace, is));
      if (ie == line.npos) { // Only a token left
        const std::string sensor(line.substr(is));
        names.insert(sensor);
        break;
      }
      const std::string sensor(line.substr(is, ie - is));
      names.insert(sensor);
      i = ie + 1; // Point to first character after white space that was found
    }
  }
}

void
Sensors::qKeep(const tNames& names)
{
  mnToStore = 0;

  for (tSensors::iterator it(mSensors.begin()), et(mSensors.end()); it != et; ++it) {
    const bool q(names.find(it->name()) != names.end());
    mnToStore += q;
    it->qKeep(q);
  }
}

void
Sensors::qCriteria(const tNames& names)
{
  for (tSensors::iterator it(mSensors.begin()), et(mSensors.end()); it != et; ++it) {
    it->qCriteria(names.find(it->name()) != names.end());
  }
}

std::string
Sensors::crcLower() const
{
  std::string codigo(crc());
  for (std::string::size_type i(0), e(codigo.size()); i < e; ++i) {
    codigo[i] = tolower(codigo[i]);
  }
  return codigo;
}

std::string
Sensors::mkFilename(const std::string& dir) const
{
  DIR *directory = opendir(dir.c_str());

  if (directory == NULL) {
    std::cerr << "Unable to open directory '" << dir << "', " 
	    << strerror(errno) << std::endl;
    exit(1);
  }

  struct dirent *entry;
  const std::string crc = crcLower();

  while ((entry = readdir(directory)) != NULL) {
    const std::string name(entry->d_name);
    std::string lower(name);
    for (std::string::size_type i(0), e(lower.size()); i < e; ++i) {
      lower[i] = tolower(lower[i]);
    }
    if (lower == crc) {
      if (closedir(directory)) {
        std::cerr << "Error closing '" << dir << "', " << strerror(errno) << std::endl;
      }
      return dir + "/" + name;
    }
    if (lower == (crc + ".ccc")) {
      if (closedir(directory)) {
        std::cerr << "Error closing '" << dir << "', " << strerror(errno) << std::endl;
      }
      return dir + "/" + name;
    }
    if (lower == (crc + ".cac")) {
      if (closedir(directory)) {
        std::cerr << "Error closing '" << dir << "', " << strerror(errno) << std::endl;
      }
      return dir + "/" + name;
    }
  }

  if (closedir(directory)) {
    std::cerr << "Error closing '" << dir << "', " << strerror(errno) << std::endl;
  }

  return dir + "/" + crc + ".cac";
}

bool
Sensors::dump(const std::string& dir) const
{
  if (dir.empty()) { // No directory to write to
    return true;
  }

  if (!fs::is_directory(dir)) { // Not a directory, so see if I can make it
    if (!fs::create_directory(dir)) {
      std::cerr << "Error creating '" << dir << "', " << strerror(errno) << std::endl;
      return false;
    }
    std::cerr << "Created directory '" << dir << "'" << std::endl;
  }

  const std::string filename(mkFilename(dir));

  if (!fs::exists(filename)) { // File doesn't exist, so dump it
    std::string str;
    { // Build output string
      std::ostringstream oss;
      for (tSensors::const_iterator it(mSensors.begin()), et(mSensors.end()); it != et; ++it) {
        it->dump(oss);
      }
      str = oss.str();
    }
    { // Now create a temporary file, which we'll move to the final filename, which is atomic
      char tempfn[2048];
      snprintf(tempfn, sizeof(tempfn), "%s.XXXXXXXX", filename.c_str());
      int fd(mkstemp(tempfn));

      if (fd == -1) {
        std::cerr << "Error creating temporary filename for '" << tempfn << "', " 
                  << strerror(errno) << std::endl;
        exit(1);
      }

      const int n(str.size());

      if (write(fd, str.c_str(), n) != n) {
        std::cerr << "Error writing " << n << " characters to '" << tempfn << "', "
                  << strerror(errno) << std::endl;
        exit(1);
      }

      if (close(fd)) {
        std::cerr << "Error closing '" << tempfn << "', "
                  << strerror(errno) << std::endl;
        exit(1);
      }

      if (rename(tempfn, filename.c_str())) {
        std::cerr << "Error renaming '" << tempfn 
                  << "' to '" << filename << "', "
                  << strerror(errno) << std::endl;
        exit(1);
      } else {
        std::cerr << "Created '" << filename << "'" << std::endl;
      }
    }
  }
 
  return true;
}

bool
Sensors::load(const std::string& dir,
              const Header& hdr)
{
  if (dir.empty()) { // No directory to load from
    return false;
  }

  mCRC = hdr.crc();

  const std::string filename(mkFilename(dir));

  if (!fs::exists(filename)) { // no file exists, so nothing to load
    return false;
  }

  DecompressTWR is(filename, qCompressed(filename));
  if (!is) {
    std::cerr << "Error opening '" << filename << "', " << strerror(errno) << std::endl;
    return false;
  }

  for (std::string line; getline(is, line);) {
    const Sensor sensor(line);
    if (sensor.qAvailable()) {
      mSensors.push_back(sensor);
    }
  }

  mnToStore = mSensors.size();

  return true;
}

std::ostream&
operator << (std::ostream& os,
             const Sensors& sen)
{
  for (Sensors::size_type i(0), e(sen.size()); i < e; ++i)
    os << sen[i] << std::endl;

  return os;
}
