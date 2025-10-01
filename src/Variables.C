// Nov-2012, Pat Welch, pat@mousebrains.com

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
   ) along with dbd2netCDF.  If not, see <http://www.gnu.org/licenses/>.
*/

// Handle mapping variable names to netCDF variable numbers

#include "Variables.H"
#include "MyException.H"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

Variables::Variables(const std::string& mapfn,
                     const std::string& typfn)
{
  if (!mapfn.empty())
    remapLoad(mapfn);

  if (!typfn.empty())
    typeInfoLoad(typfn);
}

void
Variables::remapLoad(const std::string& fn)
{
  std::ifstream is(fn.c_str());
  if (!is) {
    std::ostringstream oss;
    oss << "Error opening '" << fn << "', " << strerror(errno);
    throw MyException(oss.str());
  }
  for (std::string line; getline(is, line);) {
    std::istringstream iss(line);
    std::string name0, name1;
    if ((iss >> name0 >> name1)) {
      mReMap.insert(std::make_pair(name0, name1));
    }
  }
}

std::string
Variables::remap(const std::string& name) const
{
  tReMap::const_iterator it(mReMap.find(name));
  return (it == mReMap.end()) ? name : it->second;
}

void 
Variables::typeInfo(const std::string& name, 
                    const nc_type t, 
                    const std::string& units,
                    const std::string& prefix,
                    const std::string& dimName)
{
  const std::string key(remap(name));
  tTypeInfo::iterator it(mTypeInfo.find(key));

  if (it != mTypeInfo.end()) { // an entry already exists for key
    if ((t != -1) && (it->second.nctype != t)) { // Need to change ncytpe
      if (it->second.nctype != -1) { // Conflicting types
        std::ostringstream oss;
        oss << "typeInfo nc_type mismatch, original ("
            << NetCDF::typeToStr(it->second.nctype)
            << ") new (" << NetCDF::typeToStr(t) << ")";
        throw MyException(oss.str());
      }
      it->second.nctype = t;
    }
    if (!units.empty() && (it->second.units != units)) { // Need to change units
      if (!it->second.units.empty()) { // Conflicting units
        std::ostringstream oss;
        oss << "typeInfo units mismatch, original ("
            << it->second.units << ") new (" << units << ")";
        throw MyException(oss.str());
      }
      it->second.units = units;
    }
    if (!prefix.empty() && (it->second.prefix != prefix)) { // Need to change prefix
      if (!it->second.prefix.empty()) { // Conflicting prefix
        std::ostringstream oss;
        oss << "typeInfo prefix mismatch, original ("
            << it->second.prefix << ") new (" << prefix << ")";
        throw MyException(oss.str());
      }
      it->second.prefix = prefix;
    }
    if (!dimName.empty() && (it->second.dimName != dimName)) { // Need to change dimName
      if (!it->second.dimName.empty()) { // Conflicting dimName
        std::ostringstream oss;
        oss << "typeInfo dimension name mismatch, original ("
            << it->second.dimName << ") new (" << dimName << ")";
        throw MyException(oss.str());
      }
      it->second.dimName = dimName;
    }
  } else {
    mTypeInfo.insert(std::make_pair(key, TypeInfo(t, units, prefix, dimName)));
  }
}

void
Variables::typeInfoLoad(const std::string& fn)
{
  std::ifstream is(fn.c_str());
  if (!is) {
    std::ostringstream oss;
    oss << "Error opening '" << fn << "', " << strerror(errno);
    throw MyException(oss.str());
  }
  for (std::string line; getline(is, line);) {
    std::istringstream iss(line);
    std::string name, dataType, units;
    int nctype;
    if ((iss >> name >> dataType >> units)) {
      if (dataType == "double") {
        nctype = NC_DOUBLE;
      } else if (dataType == "float") {
        nctype = NC_FLOAT;
      } else if (dataType == "uint32") {
        nctype = NC_UINT;
      } else if (dataType == "int32") {
        nctype = NC_INT;
      } else if (dataType == "uint16") {
        nctype = NC_USHORT;
      } else if (dataType == "int16") {
        nctype = NC_SHORT;
      } else if (dataType == "uint8") {
        nctype = NC_UBYTE;
      } else if (dataType == "int8") {
        nctype = NC_BYTE;
      } else {
        std::ostringstream oss;
        oss << "Unrecognized data type '" << dataType << "' in " << fn;
        throw MyException(oss.str());
      }
      typeInfo(name, nctype, units, std::string(), std::string());
    }
  }
}

int
Variables::varNum(const std::string& name) const
{
  tVars::const_iterator it(mVars.find(remap(name)));

  return (it == mVars.end()) ? -1 : it->second;
}

void
Variables::setupVars(NetCDF& nc)
{
  typedef std::map<std::string, int> tDimMap;
  tDimMap dimMap;

  // Create dimension names map first
  for (tTypeInfo::const_iterator it(mTypeInfo.begin()), et(mTypeInfo.end()); 
       it != et; ++it) {
    const std::string key(it->first);
    const std::string dimName(it->second.dimName);
    if (!dimName.empty() && (dimMap.find(dimName) == dimMap.end())) {
      dimMap.insert(std::make_pair(dimName, nc.createDim(dimName)));
    }
  }

  // Create variables
  for (tTypeInfo::const_iterator it(mTypeInfo.begin()), et(mTypeInfo.end()); 
       it != et; ++it) {
    const std::string dimName(it->second.dimName);
    if (!dimName.empty()) {
      const std::string& key(it->first);
      const std::string& prefix(it->second.prefix);
      const std::string& units(it->second.units);
      const std::string& dimName(it->second.dimName);
      const nc_type typ(it->second.nctype);
      const int dVal(dimMap[dimName]);
      mVars.insert(std::make_pair(key, 
                                  nc.createVar(prefix + key, 
                                               typ == -1 ? NC_DOUBLE : typ, 
                                               dVal, units)));
    }
  }
}

std::ostream&
operator << (std::ostream& os,
             const Variables& v)
{
  for (Variables::tReMap::const_iterator it(v.mReMap.begin()), et(v.mReMap.end()); 
       it != et; ++it) {
    os << it->first << " -> " << it->second << std::endl;
  }

  os << std::endl;

  for (Variables::tTypeInfo::const_iterator it(v.mTypeInfo.begin()), et(v.mTypeInfo.end()); 
       it != et; ++it) {
    os << it->first << " -> {" << NetCDF::typeToStr(it->second.nctype)
       << ", " << it->second.units 
       << ", " << it->second.prefix 
       << ", " << it->second.dimName 
       << "}"
       << std::endl;
  }

  os << std::endl;

  for (Variables::tVars::const_iterator it(v.mVars.begin()), et(v.mVars.end());
       it != et; ++it) {
    os << it->first << " -> " << it->second << std::endl;
  }
  return os;
}


