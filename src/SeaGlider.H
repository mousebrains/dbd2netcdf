#ifndef INC_SeaGlider_H_
#define INC_SeaGlider_H_

// Nov-2012, Pat Welch, pat@mousebrains.com

#include <iosfwd>
#include <string>
#include <map>
#include <set>
#include <vector>


class SeaGlider {
private:
  std::string mOptions;
  bool mqVerbose;
  std::string mofn;
  std::string mRemapfn;
  std::string mTypefn;
  
  typedef std::vector<std::string> tFiles;
  tFiles mFiles;

  int usage(const char *argv0) const;
public:
  explicit SeaGlider(int argc, char **argv);

  bool qVerbose() const {return mqVerbose;}
  const std::string& ofn() const {return mofn;}
  const std::string& remapfn() const {return mRemapfn;}
  const std::string& typefn() const {return mTypefn;}

  void file(const std::string& fn) {mFiles.push_back(fn);}
  const std::string& file(const size_t i) const {return mFiles[i];}
  size_t nFiles() const {return mFiles.size();}

  static time_t mkTime(const std::string& str);
}; // SeaGlider

std::ostream& operator << (std::ostream& os, const SeaGlider& m);

#endif // INC_SeaGlider_H_

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
