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

#include <KnownBytes.H>
#include <MyException.H>
#include <iostream>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <exception>
#include <arpa/inet.h>

KnownBytes::KnownBytes(std::istream& is)
  : mFlip(false)
{
  const int8_t tag(read8(is));
  const int8_t int8(read8(is));
  int16_t int16(read16(is));
  float fnum(0);
  double dnum(0);

  if (sizeof(int8) != 1) {
    throw(MyException("sizeof(int8) != 1"));
  }

  if (sizeof(int16) != 2) {
    throw(MyException("sizeof(int16) !=2"));
  }

  if (sizeof(fnum) != 4) {
    throw(MyException("sizeof(fnum) != 4"));
  }

  if (sizeof(dnum) != 8) {
    throw(MyException("sizeof(dnum) != 8"));
  }

  if (tag != 's') {
    throw(MyException("Error known bytes cycle tag(%c) != 's'"));
  }

  
  if (int8 != 'a') {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Error known bytes first byte(0x%x, '%c') != 'a'", 
             int8, int8);
    throw(MyException(buffer));
  }

  if (int16 != 0x1234) {
    mFlip = true;
    int16 = ntohs(int16);
    if (int16 != 0x1234) {
      char buffer[256];
      snprintf(buffer, sizeof(buffer), "Error known bytes int16(0x%x) ~= 0x1234", int16);
      throw(MyException(buffer));
    }
  }

  fnum = read32(is);

  if (fabs(fnum - 123.456) > 0.00001) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Error known bytes float(%.17g) != 123.456", fnum);
    throw(MyException(buffer));
  }

  dnum = read64(is);

  if (fabs(dnum - 123456789.12345) > 0.000000001) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Error known bytes double(%.17g) != 123456789.12345", dnum);
    throw(MyException(buffer));
  }
}

int8_t
KnownBytes::read8(std::istream& is) const
{
  int8_t val;

  if (!is.read((char *) &val, 1)) {
    char buffer[2048];
    snprintf(buffer, sizeof(buffer), "Error reading a byte, %s", strerror(errno));
    throw(MyException(buffer));
  }

  return val;
}

int16_t
KnownBytes::read16(std::istream& is) const
{
  int16_t val;

  if (!is.read((char *) &val, 2)) {
    char buffer[2048];
    snprintf(buffer, sizeof(buffer), "Error reading two bytes, %s", strerror(errno));
    throw(MyException(buffer));
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

  if (!is.read((char *) &val, 4)) {
    char buffer[2048];
    snprintf(buffer, sizeof(buffer), "Error reading four bytes, %s", strerror(errno));
    throw(MyException(buffer));
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

  if (!is.read((char *) &val, 8)) {
    char buffer[2048];
    snprintf(buffer, sizeof(buffer), "Error reading eight bytes, %s", strerror(errno));
    throw(MyException(buffer));
  }

  if (mFlip) {
    const int32_t itmp(ntohl(val.i32[0]));
    val.i32[0] = ntohl(val.i32[1]);
    val.i32[1] = itmp;
  }

  return val.fnum;
}
