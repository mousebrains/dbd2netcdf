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
#include "MyException.H"
#include "Logger.H"
#include "Decompress.H"
#include "FileInfo.H"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <random>
#include <algorithm>
#include <cctype>

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
    std::ostringstream oss;
    oss << "Error opening '" << fn << "', " << strerror(errno);
    throw MyException(oss.str());
  }

  const std::string whiteSpace(", \t\n");

  for (std::string line; getline(is, line);) {
    for (std::string::size_type i(0), e(line.size()); i < e; ) {
      const std::string::size_type tokenStart(line.find_first_not_of(whiteSpace, i));
      if (tokenStart == line.npos) { // Nothing but whitespace left
        break;
      }
      const std::string::size_type ie(line.find_first_of(whiteSpace, tokenStart));
      if (ie == line.npos) { // Only a token left
        const std::string sensor(line.substr(tokenStart));
        names.insert(sensor);
        break;
      }
      const std::string sensor(line.substr(tokenStart, ie - tokenStart));
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
    codigo[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(codigo[i])));
  }
  return codigo;
}

std::string
Sensors::mkFilename(const std::string& dir) const
{
  const fs::path dirPath(dir);

  if (!fs::is_directory(dirPath)) {
    std::ostringstream oss;
    oss << "Unable to open directory '" << dir << "'";
    throw MyException(oss.str());
  }

  const std::string crc = crcLower();

  for (const auto& entry : fs::directory_iterator(dirPath)) {
    if (!entry.is_regular_file()) continue;

    const std::string name = entry.path().filename().string();
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower == crc) {
      return (dirPath / name).string();
    }
    if (lower == (crc + ".ccc")) {
      return (dirPath / name).string();
    }
    if (lower == (crc + ".cac")) {
      return (dirPath / name).string();
    }
  }

  return (dirPath / (crc + ".cac")).string();
}

namespace {
  // Generate a unique temporary filename suffix
  std::string uniqueTempSuffix() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    return std::to_string(dis(gen));
  }
}

bool
Sensors::dump(const std::string& dir) const
{
  if (dir.empty()) { // No directory to write to
    return true;
  }

  const fs::path dirPath(dir);

  if (!fs::is_directory(dirPath)) { // Not a directory, so see if I can make it
    std::error_code ec;
    if (!fs::create_directories(dirPath, ec)) {
      LOG_ERROR("Error creating directory '{}'", dir);
      return false;
    }
    LOG_DEBUG("Created directory '{}'", dir);
  }

  const std::string filename(mkFilename(dir));
  const fs::path filePath(filename);

  if (!fs::exists(filePath)) { // File doesn't exist, so dump it
    std::string str;
    { // Build output string
      std::ostringstream oss;
      for (tSensors::const_iterator it(mSensors.begin()), et(mSensors.end()); it != et; ++it) {
        it->dump(oss);
      }
      str = oss.str();
    }
    { // Now create a temporary file, which we'll move to the final filename
      const std::string tempfn = filename + "." + uniqueTempSuffix();

      std::ofstream ofs(tempfn, std::ios::binary);
      if (!ofs) {
        std::ostringstream oss;
        oss << "Error creating temporary file '" << tempfn << "'";
        throw MyException(oss.str());
      }

      ofs.write(str.c_str(), static_cast<std::streamsize>(str.size()));

      if (!ofs) {
        std::ostringstream oss;
        oss << "Error writing " << str.size() << " characters to '" << tempfn << "'";
        // Try to clean up the temp file
        std::error_code ec;
        fs::remove(tempfn, ec);
        throw MyException(oss.str());
      }

      ofs.close();

      // Rename temp file to final filename (atomic on most filesystems)
      std::error_code ec;
      fs::rename(tempfn, filePath, ec);
      if (ec) {
        std::ostringstream oss;
        oss << "Error renaming '" << tempfn << "' to '" << filename << "'";
        // Try to clean up the temp file
        fs::remove(tempfn, ec);
        throw MyException(oss.str());
      } else {
        LOG_DEBUG("Created cache file '{}'", filename);
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
    LOG_ERROR("Error opening '{}': {}", filename, strerror(errno));
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
