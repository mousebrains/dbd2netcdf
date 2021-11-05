#ifndef INC_Variables_H_
#define INC_Variables_H_

// Nov-2012, Pat Welch, pat@mousebrains.com

#include "MyNetCDF.H"
#include <iosfwd>
#include <string>
#include <map>

class Variables {
private:
  typedef std::map<std::string, std::string> tReMap;
  tReMap mReMap;

  void remapLoad(const std::string& fn);
  std::string remap(const std::string& name) const;

  struct TypeInfo {
    nc_type nctype;
    std::string units;
    std::string prefix;
    std::string dimName;
    TypeInfo() : nctype(-1) {}
    TypeInfo(const nc_type t, const std::string& u, const std::string& p,
             const std::string& dName) 
      : nctype(t), units(u), prefix(p), dimName(dName) {}
  }; // TypeInfo

  typedef std::map<std::string, struct TypeInfo> tTypeInfo;
  tTypeInfo mTypeInfo;

  void typeInfoLoad(const std::string& fn);

  typedef std::map<std::string, int> tVars;
  tVars mVars;
public:
  Variables() {}
  Variables(const std::string& mapfn, const std::string& typfn);

  void remap(const std::string& src, const std::string& tgt) {
    mReMap.insert(std::make_pair(src, tgt));
  }

  void typeInfo(const std::string& name, const nc_type t, 
                const std::string& units, const std::string& prefix,
                const std::string& dimName);

  void setupVars(NetCDF& nc);

  int varNum(const std::string& name) const;

  friend std::ostream& operator << (std::ostream& os, const Variables& v);
}; // Variables

std::ostream& operator << (std::ostream& os, const Variables& v);

#endif // INC_Variables_H_

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
