#ifndef INC_SGMerge_H_
#define INC_SGMerge_H_

// Aug-2016, Pat Welch, pat@mousebrains.com

#include <netcdf.h>
#include <vector>
#include <map>
#include <map>
#include <string>

class SGMerge {
private:
  struct Data {
    nc_type xtype;
    std::vector<char> cVal;
    std::vector<int> iVal;
    std::vector<float> fVal;
    std::vector<double> dVal;
    Data() : xtype(NC_NAT) {}
    Data(const nc_type t) : xtype(t) {}
    Data(const nc_type t, const size_t l) : xtype(t) {resize(l);}
    bool operator == (const Data& rhs) const;
    bool operator != (const Data& rhs) const {return !(*this == rhs);}
    struct Data& operator += (const Data& rhs);
    size_t size() const;
    void resize(const size_t len);
    int getAttr(const int ncid, const int vid, const std::string& name);
    int putAttr(const int ncid, const int vid, const std::string& name) const;
    int getVar(const int ncid, const int vid);
    int putVar(const int ncid, const int vid, const size_t start[], const size_t cnt[]) const;
    bool qDigits() const;
    void toDigits();
    void toArray(const size_t nSkip);
    static int setFill(const int ncid, const int vid, const nc_type xtype);
    std::string toString() const;
  };

  struct Dimension {
    std::string name;
    size_t len;
    Dimension(const std::string& n, const size_t l) : name(n), len(l) {}
  };
  
  typedef std::vector<Dimension> tDims;
  typedef std::map<std::string, size_t> tDimMap;
  tDimMap mDims;
  tDimMap mDimCnt;
  tDimMap mDimIDs;
 
  struct Attribute {
    std::string name;
    struct Data value;

    Attribute(const int ncid, const size_t vid, const size_t aid, const char *fn);
    std::string toString() const;
  };

  typedef std::map<std::string, Attribute> tAttr;
  tAttr mGlobalAttr;
 
  struct Variable {
    const std::string name; // Name of variable
    nc_type xtype; // Data type
    size_t nSkip; // Number of leading fields to skip
    size_t nFields; // Number of numeric fields
    bool qArray; // Comma deliminated array of numbers
    bool qChars; // Single character to number
    std::vector<std::string> mDims; // Names of dimensions

    tAttr mAttr;
    Variable(const std::string& n, const nc_type xt) 
      : name(n), xtype(xt), nSkip(0), nFields(0), qArray(false), qChars(false) {} 
    bool check(const int ncid, const size_t id, const size_t len, const char *fn);
    void checkArray(const Data& data);
  };

  typedef std::map<std::string, Variable> tVars;
  tVars mVars;
  tDimMap mVarIDs;

  typedef std::map<std::string, std::string> tDiveVars;
  tDiveVars mDiveVars;

  const std::string mFilename;
  int oncid;
public:
  explicit SGMerge(const char *fn);
  ~SGMerge();
  bool loadFileHeader(const char *fn);
  void updateHeader();
  bool mergeFile(const char *fn);
}; // SGMerge

#endif // INC_SGMerge_H_

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
