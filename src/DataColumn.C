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

#include "DataColumn.H"
#include "KnownBytes.H"
#include "StackDump.H"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cxxabi.h>

DataColumn::DataColumn(const size_t id,
                       const size_t sz)
  : mId(id)
  , mqSet(sz, 0)
{
  switch (id) {
    case 1: mInt8.resize(sz, 0); break;
    case 2: mInt16.resize(sz, 0); break;
    case 4: mFloat.resize(sz, NAN); break;
    case 8: mDouble.resize(sz, NAN); break;
    default:
      std::cerr << "Invalid id in DataColumn, " << id << std::endl;
      stackDumpExit(1);
  }
}

double
DataColumn::getDouble(const size_t index) const
{
  if (mId == 8) {
    if (index < mqSet.size()) {
      return mDouble[index];
    }
    std::cerr << "Index " << index << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }
  std::cerr << "Called getDouble with an invalid id " << mId << std::endl;
  stackDumpExit(1);

  return 0; // Not reached
}

float
DataColumn::getFloat(const size_t index) const
{
  if (mId == 4) {
    if (index < mqSet.size()) {
      return mFloat[index];
    }
    std::cerr << "Index " << index << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }
  std::cerr << "Called getFloat with an invalid id " << mId << std::endl;
  stackDumpExit(1);

  return 0; // Not reached
}

int16_t
DataColumn::getInt16(const size_t index) const
{
  if (mId == 2) {
    if (index < mqSet.size()) {
      return mInt16[index];
    }
    std::cerr << "Index " << index << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }
  std::cerr << "Called getInt16 with an invalid id " << mId << std::endl;
  stackDumpExit(1);

  return 0; // Not reached
}

int8_t
DataColumn::getInt8(const size_t index) const
{
  if (mId == 1) {
    if (index < mqSet.size()) {
      return mInt8[index];
    }
    std::cerr << "Index " << index << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }
  std::cerr << "Called getInt8 with an invalid id " << mId << std::endl;
  stackDumpExit(1);

  return 0; // Not reached
}

void
DataColumn::setDouble(const size_t index,
                      const double value)
{
  if (mId == 8) {
    if (index < mqSet.size()) {
      mqSet[index] = 1;
      mDouble[index] = value;
      return;
    }
    std::cerr << "Index " << index << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }
  std::cerr << "Called setDouble with an invalid id " << mId << std::endl;
  stackDumpExit(1);
}

void
DataColumn::setFloat(const size_t index,
                     const float value)
{
  if (mId == 4) {
    if (index < mqSet.size()) {
      mqSet[index] = 1;
      mFloat[index] = value;
      return;
    }
    std::cerr << "Index " << index << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }
  std::cerr << "Called setFloat with an invalid id " << mId << std::endl;
  stackDumpExit(1);
}

void
DataColumn::setInt16(const size_t index,
                     const int16_t value)
{
  if (mId == 2) {
    if (index < mqSet.size()) {
      mqSet[index] = 1;
      mInt16[index] = value;
      return;
    }
    std::cerr << "Index " << index << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }
  std::cerr << "Called setInt16 with an invalid id " << mId << std::endl;
  stackDumpExit(1);
}

void
DataColumn::setInt8(const size_t index,
                    const int8_t value)
{
  if (mId == 1) {
    if (index < mqSet.size()) {
      mqSet[index] = 1;
      mInt8[index] = value;
      return;
    }
    std::cerr << "Index " << index << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }
  std::cerr << "Called setInt8 with an invalid id " << mId << std::endl;
  stackDumpExit(1);
}

void
DataColumn::set(const size_t lhsIndex,
                const size_t rhsIndex,
                const DataColumn& rhs)
{
  if (mId != rhs.mId) {
    std::cerr << "lhs.mId(" << mId << ") != rhs.mId(" << rhs.mId << ")" << std::endl;
    stackDumpExit(1);
  }

  if (lhsIndex >= mqSet.size()) {
    std::cerr << "lhsIndex " << lhsIndex << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }

  if (rhsIndex >= rhs.mqSet.size()) {
    std::cerr << "rhsIndex " << lhsIndex << " is too large, max is " 
              << rhs.mqSet.size() << std::endl;
    stackDumpExit(1);
  }

  mqSet[lhsIndex] = rhs.mqSet[rhsIndex];
  
  switch (mId) {
    case 1: mInt8[lhsIndex] = rhs.mInt8[rhsIndex]; break;
    case 2: mInt16[lhsIndex] = rhs.mInt16[rhsIndex]; break;
    case 4: mFloat[lhsIndex] = rhs.mFloat[rhsIndex]; break;
    case 8: mDouble[lhsIndex] = rhs.mDouble[rhsIndex]; break;
  }
}

void 
DataColumn::read(const size_t index, 
                 std::istream& is, 
                 const KnownBytes& kb)
{
  if (index >= mqSet.size()) {
    std::cerr << "index " << index << " is too large, max is " << mqSet.size() << std::endl;
    stackDumpExit(1);
  }

  mqSet[index] = 1;

  switch (mId) {
    case 1: mInt8[index] = kb.read8(is); break;
    case 2: mInt16[index] = kb.read16(is); break;
    case 4: mFloat[index] = kb.read32(is); break;
    case 8: mDouble[index] = kb.read64(is); break;
  }
}

void
DataColumn::resize(const size_t sz)
{
  if (sz > mqSet.size()) {
    std::cerr << "Attempt to resize from " << mqSet.size() << " to " << sz << std::endl;
    stackDumpExit(1);
  }

  mqSet.resize(sz);
  switch (mId) {
    case 1: mInt8.resize(sz); break;
    case 2: mInt16.resize(sz); break;
    case 4: mFloat.resize(sz); break;
    case 8: mDouble.resize(sz); break;
  }
}

std::string
DataColumn::toStr(const size_t index) const
{
  if (!qSet(index)) {
    return std::string("nan");
  }
  std::ostringstream oss;
  switch(mId) {
    case 1: oss << static_cast<int>(mInt8[index]); break;
    case 2: oss << mInt16[index]; break;
    case 4: oss << std::setprecision(7) << mFloat[index]; break;
    case 8: oss << std::setprecision(15) << mDouble[index]; break;
  }
  return oss.str();
}

std::ostream&
operator << (std::ostream& os,
             const DataColumn& d)
{
  for (DataColumn::size_type i(0), e(d.size()); i < e; ++i) {
    os << d.toStr(i) << std::endl;
  }

  return os;
}

