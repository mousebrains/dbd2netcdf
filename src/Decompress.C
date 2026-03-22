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

#include "Decompress.H"
#include "lz4.h"
#include "Logger.H"
#include "FileInfo.H"
#include <cerrno>
#include <cstdio>
#include <vector>

int DecompressTWRBuf::underflow() {
  // We are only called if the buffer has been consumed

  if (mqCompressed) { // Working with compressed files, so load an lz4 block
    unsigned char sz[2]; // For length of this frame
    if (!this->mIS.read(reinterpret_cast<char*>(sz), sizeof(sz)) || (this->mIS.gcount() != 2)) { // EOF
      return std::char_traits<char>::eof();
    }
    const size_t n((sz[0] << 8) | sz[1]); // unsigned Big endian
    std::vector<char> frame(n);  // RAII heap allocation instead of VLA
    if (!this->mIS.read(frame.data(), n)) { // EOF
      return std::char_traits<char>::eof();
    }
    const size_t j(LZ4_decompress_safe(frame.data(), this->mBuffer, static_cast<int>(n), sizeof(this->mBuffer)));
    if (j > sizeof(this->mBuffer)) { // Probably a corrupted file
      LOG_ERROR("Attempt to decompress lz4 block with too much data: {} > {} in {} (block size {})",
                j, sizeof(this->mBuffer), this->mFilename, n);
      return std::char_traits<char>::eof();
    }
    this->setg(this->mBuffer, this->mBuffer, this->mBuffer + j);
    this->mPos += j;
  } else { // Not compressed
    if (this->mIS.read(this->mBuffer, sizeof(this->mBuffer)) || this->mIS.gcount()) {
      const auto n = this->mIS.gcount();
      this->setg(this->mBuffer, this->mBuffer, this->mBuffer + n);
      this->mPos += static_cast<size_t>(n);
    } else {
      return std::char_traits<char>::eof();
    }
  } // mqCompressed

  return std::char_traits<char>::to_int_type(*this->gptr());
}

DecompressTWRBuf::pos_type
DecompressTWRBuf::seekoff(off_type off, std::ios_base::seekdir dir,
                          std::ios_base::openmode /*which*/) {
  // Only support tellg(): seekoff(0, cur)
  if (dir == std::ios_base::cur && off == 0) {
    // mPos is total bytes loaded; subtract unread bytes remaining in buffer
    const auto remaining = this->egptr() - this->gptr();
    return static_cast<pos_type>(this->mPos - static_cast<size_t>(remaining));
  }
  return pos_type(off_type(-1)); // Seeking not supported
}

bool qCompressed(const std::string& fn) {
  const std::string suffix(fs::path(fn).extension().string());
  const bool q((suffix.size() == 4) && (std::tolower(static_cast<unsigned char>(suffix[2])) == 'c'));
  return q;
}
