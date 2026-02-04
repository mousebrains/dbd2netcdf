#ifndef INC_Decompress_H_
#define INC_Decompress_H_

// Jan-2023, Pat Welch, pat@mousebrains.com

#include <iostream>
#include <fstream>

class DecompressTWRBuf: public std::streambuf {
  std::ifstream mIS;
  const bool mqCompressed;
  char mBuffer[65536];
  const std::string mFilename;
public:
  DecompressTWRBuf(const std::string& fn, const bool qCompressed)
    : mIS(fn.c_str(), std::ios::binary)
    , mqCompressed(qCompressed)
    , mFilename(fn)

  {}

  ~DecompressTWRBuf() {mIS.close();}

  void close() {mIS.close();}

  int underflow();
};

class DecompressTWR: public std::istream {
private:
  DecompressTWRBuf mBuf;
public:
  DecompressTWR(const std::string& fn, const bool qCompressed)
    : std::ios(0)
    , std::istream(&mBuf)
    , mBuf(fn, qCompressed)
  {}
};

bool qCompressed(const std::string& fn); // Check if filename like *.?[Cc]?

#endif // INC_Decompress_H_

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
