#ifndef INC_PD0_H_
#define INC_PD0_H_

// Apr-2012, Pat Welch, pat@mousebrains.com

#include <iosfwd>
#include <string>
#include <vector>
#include <stdint.h>

class NetCDF;

class PD0 {
private:
  class Common {
  protected:
    bool mqSet;
   
    enum DataTypes {
      dtUInt8,
      dtInt8,
      dtUInt16,
      dtInt16,
      dtUInt32,
      dtInt32
    }; // DataTypes

    union tValues {
      uint8_t  ui8;
       int8_t  i8;
      uint16_t ui16;
       int16_t i16;
      uint32_t ui32;
       int32_t i32;
      tValues() : i32(0) {}
    };

    typedef std::vector<tValues> tArray;

    struct Item {
      std::string mName;
      std::string mUnits;
      DataTypes mType;
      int mMeld;
      tArray mArray;
      int mVarId;

      Item(const std::string& name, const DataTypes t, 
           const size_t length = 1, const int meld = -1, 
           const std::string& units = std::string())
        : mName(name), mUnits(units), mType(t), mMeld(meld), mArray(length), mVarId(-1) {}

      Item(const std::string& name, const DataTypes t, const std::string& units)
        : mName(name), mUnits(units), mType(t), mMeld(-1), mArray(1), mVarId(-1) {}

      int ncType() const;
    }; // Item

    typedef std::vector<Item> tItems;
    tItems mItems;
  public:
    Common() : mqSet(false) {}
    operator bool() const {return mqSet;}
    void clear() {mqSet = false;}

    bool load(std::istream& is, PD0& pd0, const bool qDump = false);
    void dump(std::ostream& os);

    void ncSetup(NetCDF& nc, const int iDim);
    void ncSetup(NetCDF& nc, const int iDim, const int kDim);

    void ncDump(NetCDF& nc, const size_t index);
  }; // Common

  class Fixed : public Common {
  public:
    Fixed();

    size_t nCells() const;
  }; // Fixed

  class Variable : public Common {
  public:
    Variable();
  }; // Variable

  class Velocity : public Common {
  private:
    int mVarId;
  public:
    Velocity() : mVarId(-1) {}

    bool load(std::istream& is, PD0& pd0);

    void ncSetup(NetCDF& nc, const int iDim, const int jDim, const int kDim);

    void ncDump(NetCDF& nc, const size_t index);
  }; // Velocity

  class Correlation : public Common {
  private:
    std::string mName;
    int mVarId;
  public:
    explicit Correlation(const std::string& name) : mName(name), mVarId(-1) {}

    bool load(std::istream& is, PD0& pd0);

    void ncSetup(NetCDF& nc, const int iDim, const int jDim, const int kDim);
    void ncDump(NetCDF& nc, const size_t index);
  }; // Correlation

  class BottomTrack : public Common {
  public:
    BottomTrack();
    bool load(std::istream& is, PD0& pd0);
  }; // BottomTrack

  class VMDAS : public Common {
  public:
    VMDAS();
    bool load(std::istream& is, PD0& pd0);
  }; // VMDAS

  Fixed mFixed;
  Variable mVariable;
  Velocity mVelocity;
  Correlation mCorrelation;
  Correlation mEcho;
  Correlation mPercentGood;
  BottomTrack mBottomTrack;
  VMDAS mVMDAS;

  std::string mFilename;
  uint8_t mMaxNumberOfCells;

  int miDim;
  int mjDim;
  int mk4Dim;
  int mk8Dim;

  size_t load(std::istream& is, NetCDF& nc, size_t index);
  bool loadBlock(std::istream& is);
  size_t dumpBlock(NetCDF& nc, size_t index);

  bool readAndCheckByte(std::istream& is, const int expectedValue, const bool qThrow = true);
  uint8_t readByte(std::istream& is, const bool qThrow = true);
  uint16_t readUInt16(std::istream& is, const bool qThrow = true);
  int16_t readInt16(std::istream& is, const bool qThrow = true) {return (int16_t) readUInt16(is, qThrow);}
  uint32_t readUInt32(std::istream& is, const bool qThrow = true);
  int32_t readInt32(std::istream& is, const bool qThrow = true) {return (int32_t) readUInt32(is, qThrow);}
public:
  PD0() : mCorrelation("correlation"), mEcho("echo"), mPercentGood("percent_good"), mMaxNumberOfCells(0) {}

  size_t load(const std::string& fn, NetCDF& nc, size_t index);

  void setupNetCDFVars(NetCDF& nc);

  void maxNumberOfCells(uint8_t nCells) {mMaxNumberOfCells = nCells;}
  static uint8_t maxNumberOfCells(const std::string& fn);
}; // PD0

#endif // INC_PD0_H_

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
