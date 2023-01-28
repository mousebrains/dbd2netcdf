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

// Read in a set of Dinkum Binary Data files, and
// output them into a netCDF file

#include "MyNetCDF.H"
#include "FileInfo.H"
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>

void
NetCDF::basicOp(int retval,
                const std::string& label) const
{
  if (!retval) {
    return;
  }
  std::cerr << "Error " << label << " '" << mFilename << "', " << nc_strerror(retval) << std::endl;
  exit(2);
}

NetCDF::NetCDF(const std::string& fn, const bool qAppend)
  : mFilename(fn)
  , mId(-1)
  , mqOpen(false)
  , mChunkSize(5000)
  , mChunkPriority(true)
  , mCompressionLevel(9)
  , mCountOne(0)
  , mCountLength(0)
{
  if (qAppend && fs::exists(fn)) {
    basicOp(nc_open(fn.c_str(), NC_WRITE, &mId), "opening");
  } else {
    basicOp(nc_create(fn.c_str(), NC_NETCDF4 | NC_CLOBBER, &mId), "creating");
  }
  mqOpen = true;
}

NetCDF::~NetCDF()
{
  close();
  delete mCountOne;
}

int
NetCDF::maybeCreateDim(const std::string& name)
{
  int dimId;
  const int retval(nc_inq_dimid(mId, name.c_str(), &dimId));
  if (retval) return createDim(name);
  // Put checks in here
  return dimId;
} // maybeCreateDim

int
NetCDF::createDim(const std::string& name)
{
  return createDim(name, NC_UNLIMITED, 0);
}

int
NetCDF::createDim(const std::string& name,
                  const size_t len)
{
  return createDim(name, len, (len == NC_UNLIMITED) ? 0 : len);
}

int
NetCDF::createDim(const std::string& name,
                  const size_t len,
                  const size_t maxLength)
{
  int dimId;
  const int retval(nc_def_dim(mId, name.c_str(), len, &dimId));
  if (retval) {
    std::cerr << "Error creating dimension '" << name << "' in '" << mFilename << "', "
              << nc_strerror(retval) << std::endl;
    exit(2);
  }

  mDimensionLimits.insert(std::make_pair(dimId, (len != NC_UNLIMITED) ? len : maxLength));

  return dimId;
}

size_t
NetCDF::lengthDim(const int dimId) {
  char name[NC_MAX_NAME + 1];
  size_t length;
  const int retval(nc_inq_dim(mId, dimId, name, &length));
  if (retval) {
    std::cerr << "Error getting dimension '" << name << "' length in '" << mFilename << "', "
              << nc_strerror(retval) << std::endl;
    exit(2);
  }
  return length;
} // lengthDim

int 
NetCDF::maybeCreateVar(const std::string& name,
		const nc_type idType,
                const int idDim,
                const std::string& units)
{
  int varId;
  const int retval(nc_inq_varid(mId, name.c_str(), &varId));
  if (retval) return createVar(name, idType, idDim, units);
  // Put checks in here
  return varId;
}

int
NetCDF::createVar(const std::string& name,
                  const nc_type idType,
                  const int idDim,
                  const std::string& units)
{
  return createVar(name, idType, &idDim, 1, units);
}

int
NetCDF::createVar(const std::string& name,
                  const nc_type idType,
                  const int dims[],
                  const size_t nDims,
                  const std::string& units)
{
  int varId;
  int retval;

  if ((retval = nc_def_var(mId, name.c_str(), idType, nDims, dims, &varId))) {
    std::cerr << "Error creating variable '" << name << "' in '" << mFilename << "', "
              << nc_strerror(retval) << std::endl;
    exit(2);
  }

  { // Deal with chunking for multiple dimensions
    typedef std::vector<size_t> tLengths;
    tLengths lengths(nDims);
    for (size_t i(0); i < nDims; ++i) {
      basicOp(nc_inq_dimlen(mId, dims[i], &lengths[i]), "Get dimension length");
    }

    size_t *chunkSizes(new size_t[nDims]);
    size_t availSize(mChunkSize);

    for (size_t i(nDims - 1); i < nDims; --i) { // Use wrap of unsigned ops
      if (lengths[i] != 0) { // A defined length
        chunkSizes[i] = lengths[i];
        availSize /= lengths[i];
      } else { // Unlimited length
        chunkSizes[i] = 1;
      }
    }

    if (availSize) { // space available for unlimited dimensions
      const size_t iStart(mChunkPriority ? 0 : nDims - 1);
      const int iOp(mChunkPriority ? 1 : -1);
      for (size_t i(iStart); i < nDims; i += iOp) {
          if (lengths[i] == 0) { // An unlimited dimension
            tDimensionLimits::const_iterator it(mDimensionLimits.find(dims[i]));
            if ((it != mDimensionLimits.end()) &&
                (it->second > 0) &&
                (it->second < availSize)) {
                chunkSizes[i] = it->second;
                availSize /= it->second;
                if (availSize == 0)
                  break;
            } else {
              chunkSizes[i] = availSize;
              break;
            }
          }
        }
      }

      if ((retval = nc_def_var_chunking(mId, varId, NC_CHUNKED, chunkSizes))) {
        std::cerr << "Error setting chunk size to {";
        for (tLengths::size_type i(0), e(lengths.size()); i < e; ++i) {
          if (i != 0) {
            std::cerr << ", ";
          }
          std::cerr << lengths[i];
        }
        std::cerr << "} for '" << name << "' in '" << mFilename << "', "
                  << nc_strerror(retval) << std::endl;
        exit(2);
      }

      delete[] chunkSizes;
    } // Chunking

  const bool qShuffle((idType == NC_SHORT) || (idType == NC_INT) || (idType == NC_INT64));
  const bool qCompress(idType != NC_STRING);

  if ((qShuffle || qCompress) &&
		  (retval = nc_def_var_deflate(mId, varId, qShuffle, qCompress, mCompressionLevel))
		  ) {
    std::cerr
	    << "Error enabling compression and shuffle("
	    << qShuffle
	    << ","
	    << mCompressionLevel
	    << ") for '"
	    << name
	    << "' in '"
	    << mFilename
	    << "', "
	    << nc_strerror(retval)
	    << std::endl;
    exit(2);
  }

  if (idType == NC_FLOAT) {
    const float badValue(nan(""));
    if ((retval = nc_def_var_fill(mId, varId, NC_FILL, &badValue))) {
      std::cerr << "Error setting fill value, " << badValue
                << " for '" << name << "' in '" << mFilename
                << "', " << nc_strerror(retval) << std::endl;
      exit(2);
    }
  } else if (idType == NC_DOUBLE) {
    const double badValue(nan(""));
    if ((retval = nc_def_var_fill(mId, varId, NC_FILL, &badValue))) {
      std::cerr << "Error setting fill value, " << badValue
                << " for '" << name << "' in '" << mFilename
                << "', " << nc_strerror(retval) << std::endl;
      exit(2);
    }
  }

  if (!units.empty()) {
    if ((retval = nc_put_att_text(mId, varId, "units", units.size(), units.c_str()))) {
      std::cerr << "Error setting units attribute, '" << units << "' for '"
                << name << "' in '" << mFilename << "', " << nc_strerror(retval)
                << std::endl;
      exit(2);
    }
  }

  return varId;
}

void
NetCDF::enddef()
{
  basicOp(nc_enddef(mId), "ending definitions for");
}

void
NetCDF::close()
{
  if (mqOpen) {
    basicOp(nc_close(mId), "closing");
  }
  mqOpen = false;
}

void
NetCDF::putVars(const int varId,
                const size_t start,
                const size_t count,
                const double data[])
{
  putVars(varId, &start, &count, data);
}

void
NetCDF::putVars(const int varId,
                const size_t start,
                const size_t count,
                const int32_t data[])
{
  putVars(varId, &start, &count, data);
}

void
NetCDF::putVars(const int varId,
                const size_t start,
                const size_t count,
                const uint32_t data[])
{
  putVars(varId, &start, &count, data);
}

void
NetCDF::putVars(const int varId,
                const size_t start[],
                const size_t count[],
                const double data[])
{
  putVarError(nc_put_vara_double(mId, varId, start, count, data), varId);
}

void
NetCDF::putVars(const int varId,
                const size_t start[],
                const size_t count[],
                const int32_t data[])
{
  putVarError(nc_put_vara_int(mId, varId, start, count, data), varId);
}

void
NetCDF::putVars(const int varId,
                const size_t start[],
                const size_t count[],
                const uint32_t data[])
{
  putVarError(nc_put_vara_uint(mId, varId, start, count, data), varId);
}

void
NetCDF::putVar(const int varId,
               const size_t start[],
               const size_t len,
               const double value)
{
  putVarError(nc_put_vara_double(mId, varId, start, mkCountOne(len), &value), varId);
}

void
NetCDF::putVar(const int varId,
               const size_t start[],
               const size_t len,
               const uint32_t value)
{
  putVarError(nc_put_vara_uint(mId, varId, start, mkCountOne(len), &value), varId);
}

void
NetCDF::putVar(const int varId,
               const size_t start[],
               const size_t len,
               const int32_t value)
{
  putVarError(nc_put_vara_int(mId, varId, start, mkCountOne(len), &value), varId);
}

void
NetCDF::putVar(const int varId,
               const size_t start,
               const double value)
{
  const size_t count(1);
  putVarError(nc_put_vara_double(mId, varId, &start, &count, &value), varId);
}

void
NetCDF::putVar(const int varId,
               const size_t start,
               const uint32_t value)
{
  const size_t count(1);
  putVarError(nc_put_vara_uint(mId, varId, &start, &count, &value), varId);
}

void
NetCDF::putVar(const int varId,
               const size_t start,
               const int32_t value)
{
  const size_t count(1);
  putVarError(nc_put_vara_int(mId, varId, &start, &count, &value), varId);
}

void
NetCDF::putVar(int varId,
               size_t start,
               const std::string& str)
{
  const char *cstr(str.c_str());
  putVarError(nc_put_var1_string(mId, varId, &start, &cstr), varId);
}

void
NetCDF::putVarError(const int retval,
                    const int varId)
{
  if (!retval)
    return;

  char varName[NC_MAX_NAME + 1];
  basicOp(nc_inq_varname(mId, varId, varName), "getting variable name");
  std::cerr << "Error writing data to '" << varName << "' in '" << mFilename
            << "', " << nc_strerror(retval) << std::endl;
  exit(2);
}

const size_t *
NetCDF::mkCountOne(const size_t len)
{
  if (!mCountOne || (mCountLength < len)) {
    delete mCountOne;
    mCountLength = len;
    mCountOne = new size_t[len];
    for (size_t i(0); i < len; ++i) {
      mCountOne[i] = 1;
  }
  }
  return mCountOne;

}

std::string
NetCDF::typeToStr(const nc_type t)
{
  std::string str;

  switch (t) {
    case NC_BYTE:   str = "int8"; break;
    case NC_CHAR:   str = "char"; break;
    case NC_SHORT:  str = "int16"; break;
    case NC_INT:    str = "int32"; break;
    case NC_FLOAT:  str = "float"; break;
    case NC_DOUBLE: str = "double"; break;
    case NC_UBYTE:  str = "uint8"; break;
    case NC_USHORT: str = "uint16"; break;
    case NC_UINT:   str = "uint32"; break;
    case NC_INT64:  str = "int64"; break;
    case NC_UINT64: str = "uint64"; break;
    case NC_STRING: str = "string"; break;
    default:        str = "gotMe"; break;
  }

  return str;
}

