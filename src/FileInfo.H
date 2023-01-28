#ifndef INC_FileInfo_H_
#define INC_FileInfo_H_

#include <string>

namespace fs {
  bool create_directory(const std::string& fn);
  bool rename(const std::string& src, const std::string& dest);
  bool exists(const std::string& fn);
  bool is_directory(const std::string& fn);
  size_t file_size(const std::string& filename);
  std::string dirname(const std::string& filename);
  std::string filename(const std::string& filename);
  std::string extension(const std::string& filename);
  std::string replace_extension(const std::string& filename, const std::string& ext);
};

#endif // INC_FileInfo_H_

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
