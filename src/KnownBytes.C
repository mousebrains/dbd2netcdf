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

#include "KnownBytes.H"
#include "MyException.H"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <exception>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

KnownBytes::KnownBytes(std::istream& is)
  : mFlip(false)
{
  const int8_t tag(read8(is));
  const int8_t int8(read8(is));
  int16_t int16(read16(is));
  float fnum(0);
  double dnum(0);

  static_assert(sizeof(int8_t) == 1, "sizeof(int8_t) != 1");
  static_assert(sizeof(int16_t) == 2, "sizeof(int16_t) != 2");
  static_assert(sizeof(float) == 4, "sizeof(float) != 4");
  static_assert(sizeof(double) == 8, "sizeof(double) != 8");

  if (tag != 's') {
    throw(MyException("Error known bytes cycle tag(%c) != 's'"));
  }


  if (int8 != 'a') {
    std::ostringstream oss;
    oss << "Error known bytes first byte(0x" << std::hex << static_cast<int>(int8)
        << ", '" << int8 << "') != 'a'";
    throw MyException(oss.str());
  }

  if (int16 != 0x1234) {
    mFlip = true;
    int16 = ntohs(int16);
    if (int16 != 0x1234) {
      std::ostringstream oss;
      oss << "Error known bytes int16(0x" << std::hex << int16 << ") ~= 0x1234";
      throw MyException(oss.str());
    }
  }

  fnum = read32(is);

  if (fabs(fnum - 123.456) > 0.00001) {
    std::ostringstream oss;
    oss << std::setprecision(17) << "Error known bytes float(" << fnum << ") != 123.456";
    throw MyException(oss.str());
  }

  dnum = read64(is);

  if (fabs(dnum - 123456789.12345) > 0.000000001) {
    std::ostringstream oss;
    oss << std::setprecision(17) << "Error known bytes double(" << dnum << ") != 123456789.12345";
    throw MyException(oss.str());
  }
}

int8_t
KnownBytes::read8(std::istream& is) const
{
  int8_t val;

  if (!is.read(reinterpret_cast<char*>(&val), 1)) {
    std::ostringstream oss;
    oss << "Error reading a byte, " << strerror(errno);
    throw MyException(oss.str());
  }

  return val;
}

int16_t
KnownBytes::read16(std::istream& is) const
{
  int16_t val;

  if (!is.read(reinterpret_cast<char*>(&val), 2)) {
    std::ostringstream oss;
    oss << "Error reading two bytes, " << strerror(errno);
    throw MyException(oss.str());
  }

  return mFlip ? ntohs(val) : val;
}

float
KnownBytes::read32(std::istream& is) const
{
  union uVal {
    float fnum;
    int32_t inum;
  } val;

  if (!is.read(reinterpret_cast<char*>(&val), 4)) {
    std::ostringstream oss;
    oss << "Error reading four bytes, " << strerror(errno);
    throw MyException(oss.str());
  }

  if (mFlip)
    val.inum = ntohl(val.inum);

  return val.fnum;
}

double
KnownBytes::read64(std::istream& is) const
{
  union uVal {
    double fnum;
    int32_t i32[2];
  } val;

  if (!is.read(reinterpret_cast<char*>(&val), 8)) {
    std::ostringstream oss;
    oss << "Error reading eight bytes, " << strerror(errno);
    throw MyException(oss.str());
  }

  if (mFlip) {
    const int32_t itmp(ntohl(val.i32[0]));
    val.i32[0] = ntohl(val.i32[1]);
    val.i32[1] = itmp;
  }

  return val.fnum;
}
