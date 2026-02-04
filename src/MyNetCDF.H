#ifndef INC_NetCDF_H_
#define INC_NetCDF_H_

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

#include <netcdf.h>
#include <string>
#include <map>
#include <vector>
#include <stdint.h>

class NetCDF {
private:
  std::string mFilename;
  int mId;
  bool mqOpen;
  size_t mChunkSize;
  bool mChunkPriority;
  int mCompressionLevel;

  std::vector<size_t> mCountOne;  // RAII memory management

  const size_t *mkCountOne(const size_t len);

  typedef std::map<int, size_t> tDimensionLimits;
  tDimensionLimits mDimensionLimits;

  void putVarError(int retval, const int varid);
  void basicOp(int retval, const std::string& label) const;
public:
  explicit NetCDF(const std::string& fn, const bool qAppend=false);
  ~NetCDF();

  void chunkSize(const size_t cs) {mChunkSize = cs;}
  void chunkPriority(const bool qFirst) {mChunkPriority = qFirst;}

  void compressionLevel(const int cl) {mCompressionLevel = cl;}

  int createDim(const std::string& name);
  int createDim(const std::string& name,
		  const size_t nLength);
  int createDim(const std::string& name,
		  const size_t nLength,
		  const size_t maxLength);
  int maybeCreateDim(const std::string& name);
  size_t lengthDim(const int dimId);

  int createVar(const std::string& name,
		  const nc_type idType,
		  const int idDim,
		  const std::string& units);

  int createVar(const std::string& name,
		  const nc_type idType,
		  const int dims[],
		  const size_t nDims,
		  const std::string& units);

  int maybeCreateVar(const std::string& name,
		  const nc_type idType,
		  const int idDim,
		  const std::string& units);

  void enddef();
  void close();

  void putVars(const int varId, const size_t start, const size_t count, const double data[]);
  void putVars(const int varId, const size_t start, const size_t count, const int32_t data[]);
  void putVars(const int varId, const size_t start, const size_t count, const uint32_t data[]);

  void putVars(const int varId, const size_t indices[], const size_t count[], const double data[]);
  void putVars(const int varId, const size_t indices[], const size_t count[], const int32_t data[]);
  void putVars(const int varId, const size_t indices[], const size_t count[], const uint32_t data[]);

  void putVar(const int varId, const size_t index, const double value);
  void putVar(const int varId, const size_t index, const int32_t value);
  void putVar(const int varId, const size_t index, const uint32_t value);

  void putVar(const int varId, const size_t index, const std::string& str);

  void putVar(const int varId, const size_t indices[], const size_t len, const double value);
  void putVar(const int varId, const size_t indices[], const size_t len, const uint32_t value);
  void putVar(const int varId, const size_t indices[], const size_t len, const int32_t value);

  static std::string typeToStr(const nc_type t);
}; // NetCDF

#endif // INC_NetCDF_H_
