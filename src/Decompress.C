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
#include <filesystem>
#include <cerrno>

int DecompressTWRBuf::underflow() {
  if (this->gptr() == this->egptr()) { // Decompress data into mBuffer
    if (mqCompressed) {
      char sz[2]; // For length of this frame
      if (this->mIS.read(sz, sizeof(sz))) { // not EOF
        const size_t n(sz[0] << 8 | (sz[1] & 0xff)); // Big endian
        char frame[n];
        if (this->mIS.read(frame, n)) { // not EOF
          const size_t j(LZ4_decompress_safe(frame, this->mBuffer, n, sizeof(this->mBuffer)));
          this->setg(this->mBuffer, this->mBuffer, this->mBuffer + j);
        }
      }
    } else { // Not compressed, so just load from mIS
      if (this->mIS.read(this->mBuffer, sizeof(this->mBuffer)) || this->mIS.gcount()) {
	this->setg(this->mBuffer, this->mBuffer, this->mBuffer + this->mIS.gcount());
      }
    } // if mqCompressed
  }
  return this->gptr() == this->egptr()
	  ? std::char_traits<char>::eof()
	  : std::char_traits<char>::to_int_type(*this->gptr());
}

bool qCompressed(const std::string& fn) {
  const std::string suffix(std::filesystem::path(fn).extension());
  return (suffix.size() == 4) & (std::tolower(suffix[2]) == 'c'); 
}
