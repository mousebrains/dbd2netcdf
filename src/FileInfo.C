// Jan-2023, Pat Welch, pat@mousebrains.com

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

// some of the C++17 filesystem functionality for use in earlier versions

#include "FileInfo.H"
#include <sys/stat.h>
#include <cerrno>

namespace fs {
  mode_t getMode(const std::string& fn) {
    struct stat buffer;
    return (stat(fn.c_str(), &buffer) == 0) ? buffer.st_mode : (mode_t) 0;
  }

  bool exists(const std::string& fn) { return S_ISREG(getMode(fn)); }
  bool is_directory(const std::string& fn) { return S_ISDIR(getMode(fn)); }

  size_t file_size(const std::string& fn) {
    struct stat buffer;
    if (stat(fn.c_str(), &buffer) == 0) {
      return buffer.st_size;
    }
    return 0;
  }

  std::string dirname(const std::string& fn) {
    const std::string::size_type i(fn.rfind('/'));

    if (i == fn.npos) return "./";
    if (i != (fn.size() - 1)) return fn.substr(0, i + 1); // Not a trailing /
    if (fn.size() == 1) return fn; // Only a slash
    return dirname(fn.substr(0, fn.size() - 1)); // Recurse
  }

  std::string filename(const std::string& fn) {
    const std::string::size_type i(fn.rfind('/'));

    return i == fn.npos ? fn : fn.substr(i);
  }

  std::string extension(const std::string& fn) {
    const std::string::size_type i(fn.rfind('.'));

    return i == fn.npos ? std::string() : fn.substr(i);
  }

  std::string replace_extension(const std::string& fn, const std::string& ext) {
    const std::string cur(extension(fn));
    return cur.empty() ? (fn + ext) : (fn.substr(0, fn.size() - cur.size()) + ext);
  }

  bool create_directory(const std::string& fn) {
    const char *name(fn.c_str());

    if (!mkdir(name, 0777)) return true;
    if (errno == EEXIST) return is_directory(fn);
    if (errno != ENOENT) return false;
    if (!create_directory(dirname(fn))) return false; // Recurse
    return !mkdir(name, 0777);
  }
}
