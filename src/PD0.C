#include <PD0.H>
#include <NetCDF.H>
#include <MyException.H>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <vector>

size_t
PD0::load(const std::string& fn,
          NetCDF& nc,
          size_t index)
{
  mFilename = fn;
  mFixed.clear();
  mVariable.clear();
  mVelocity.clear();
  mCorrelation.clear();
  mEcho.clear();
  mPercentGood.clear();
  mBottomTrack.clear();

  std::ifstream is(fn.c_str());

  if (!is) {
    const std::string msg("Error opening '" + fn + "', " + strerror(errno));
    std::cerr << msg << std::endl;
    throw(MyException(msg));
  }

  index = load(is, nc, index);

  return index;
}

size_t
PD0::load(std::istream& is,
          NetCDF& nc,
          size_t index)
{
  while (loadBlock(is)) {
    index = dumpBlock(nc, index);
  }
  return index;
}

bool
PD0::loadBlock(std::istream& is)
{
  const std::streampos iPos(is.tellg());

  if (!readAndCheckByte(is, 0x7f, false)) // Check first header byte
    return false;

  readAndCheckByte(is, 0x7f, true);

  const size_t nBytes(readUInt16(is)); // Number of bytes in ensemble

  char *buffer(new char[nBytes]);

  buffer[0] = 0x7f;
  buffer[1] = 0x7f;
  buffer[2] = nBytes & 0xff;
  buffer[3] = (nBytes >> 8) & 0xff;

  is.read(&buffer[4], nBytes - 4);

  if (!is) {
    std::ostringstream oss;
    oss << "Error reading " << nBytes << " from '" << mFilename << "', " << strerror(errno);
    std::cerr << oss.str() << std::endl;
    throw(MyException(oss.str()));
  }


  const unsigned int chkSum(readUInt16(is));

  unsigned int sum(0);
  for (size_t i(0); i < nBytes; ++i) {
    sum = (sum + (unsigned char) buffer[i]) & 0xffff;
  }

  if (sum != chkSum) {
    std::ostringstream oss;
    oss << "Checksum mismatch for '" << mFilename << "' starting at " << iPos
        << " ending at " << is.tellg() 
        << " calculated check sum 0x" << std::hex << sum
        << " file's checksum 0x" << chkSum
        << std::dec;
    std::cerr << oss.str() << std::endl;
    throw(MyException(oss.str()));
  }

  const std::string data(buffer, nBytes);
  delete buffer;

  std::istringstream iss(data, std::ios_base::in | std::ios_base::binary);
  iss.seekg(5, std::ios::beg);

  const size_t nDataTypes(readByte(iss)); // Number of data types

  typedef std::vector<size_t> tOffsets;
  tOffsets offsets(nDataTypes, 0);

  for (size_t i(0); i < nDataTypes; ++i) {
    offsets[i] = readUInt16(iss);
  }

  for (size_t i(0); i < nDataTypes; ++i) { // Load in data blocks
    const std::streampos opos(iss.tellg());
    iss.seekg(offsets[i], std::ios::beg);
    // const std::streampos npos(iss.tellg());
    const size_t hdr(readUInt16(iss));
    // std::cout << std::dec << "TPW i " << i << " off " << offsets[i] 
              // << " len " << ((((i + 1) < nDataTypes) ? offsets[i+1] : (nBytes + 1)) - offsets[i])
              // << " opos " << opos << " npos " << npos 
              // << " hdr " << std::hex << hdr << std::dec << std::endl;
    switch (hdr) {
      case 0x0000: 
        if (!mFixed.load(iss, *this))
          return false;
        break;
      case 0x0080: 
        if (!mVariable.load(iss, *this))
          return false;
        break;
      case 0x0100:
        if (!mVelocity.load(iss, *this))
          return false;
        break;
      case 0x0200:
        if (!mCorrelation.load(iss, *this))
          return false;
        break;
      case 0x0300:
        if (!mEcho.load(iss, *this))
          return false;
        break;
      case 0x0400:
        if (!mPercentGood.load(iss, *this))
          return false;
        break;
      case 0x0600:
        if (!mBottomTrack.load(iss, *this))
          return false;
        break;
      default:
        {
          std::ostringstream msg;
          msg << "Unrecognized header type 0x" << std::hex << hdr << " in " << mFilename;
          std::cerr << msg.str() << std::endl;
          throw(MyException(msg.str()));
          break;
        }
    }
  }

  // std::cout << "TPW final " << iss.tellg() << std::endl;
  return true;
}

size_t
PD0::dumpBlock(NetCDF& nc,
               size_t index)
{
  bool qDumped(false);

  if (mFixed) { // Something to dump out
    qDumped = true;
    mFixed.ncDump(nc, index);
  }

  if (mVariable) { // Something to dump out
    qDumped = true;
    mVariable.ncDump(nc, index);
  }

  if (mVelocity) { // Something to dump out
    qDumped = true;
    mVelocity.ncDump(nc, index);
  }

  if (mCorrelation) { // Something to dump out
    qDumped = true;
    mCorrelation.ncDump(nc, index);
  }

  if (mEcho) { // Something to dump out
    qDumped = true;
    mEcho.ncDump(nc, index);
  }

  if (mPercentGood) { // Something to dump out
    qDumped = true;
    mPercentGood.ncDump(nc, index);
  }

  if (mBottomTrack) { // Something to dump out
    qDumped = true;
    mBottomTrack.ncDump(nc, index);
  }

  return qDumped ? (index + 1) : index;
}

uint8_t
PD0::readByte(std::istream& is,
              const bool qThrow)
{
  char c;
  is.read(&c, 1);

  if (!is) { // EOF
    if (qThrow) {
      std::ostringstream msg;
      msg << "Error reading a byte from " << mFilename << ", " << strerror(errno);
      std::cerr << msg.str() << std::endl;
      throw(MyException(msg.str()));
    }
    return 0;
  }

  return ((unsigned char) c) & 0xff;
}

uint16_t
PD0::readUInt16(std::istream& is,
                const bool qThrow)
{
  char c[2];
  is.read(c, sizeof(c));

  if (!is) { // EOF
    if (qThrow) {
      std::ostringstream msg;
      msg << "Error reading a byte from " << mFilename << ", " << strerror(errno);
      std::cerr << msg.str() << std::endl;
      throw(MyException(msg.str()));
    }
    return 0;
  }

  return (c[0] & 0xff) | ((c[1] << 8) & 0xff00);
}

uint32_t
PD0::readUInt32(std::istream& is,
                const bool qThrow)
{
  char c[4];
  is.read(c, sizeof(c));

  if (!is) { // EOF
    if (qThrow) {
      std::ostringstream msg;
      msg << "Error reading a byte from " << mFilename << ", " << strerror(errno);
      std::cerr << msg.str() << std::endl;
      throw(MyException(msg.str()));
    }
    return 0;
  }

  return (c[0]         &       0xff) | 
         ((c[1] <<  8) &     0xff00) |
         ((c[2] << 16) &   0xff0000) |
         ((c[3] << 24) & 0xff000000);
}

bool
PD0::readAndCheckByte(std::istream& is,
                      const int expectedValue,
                      const bool qThrow)
{
  const char c(readByte(is, qThrow));

  if (!is)
    return false;

  if (c == expectedValue) 
    return true;

  std::ostringstream msg;
  msg << "Incorrect header byte(0x" << std::hex << c << ") should have been 0x" << expectedValue << ")";
  std::cerr << msg.str() << std::endl;

  if (qThrow) {
    throw(MyException(msg.str()));
  }

  return false;
}

bool
PD0::Common::load(std::istream& is,
                  PD0& pd0,
                  bool qDump)
{
  mqSet = false;

  for (tItems::size_type i(0), e(mItems.size()); i < e; ++i) {
    for (tArray::size_type j(0), je(mItems[i].mArray.size()); j < je; ++j) {
      switch (mItems[i].mType) {
        case dtUInt8:  mItems[i].mArray[j].ui8  = pd0.readByte(is); break;
        case dtInt8:   mItems[i].mArray[j].i8   = pd0.readByte(is); break;
        case dtInt16:  mItems[i].mArray[j].i16  = pd0.readInt16(is); break;
        case dtUInt16: mItems[i].mArray[j].ui16 = pd0.readUInt16(is); break;
        case dtInt32:  mItems[i].mArray[j].i32  = pd0.readInt32(is); break;
        case dtUInt32: mItems[i].mArray[j].ui32 = pd0.readUInt32(is); break;
      }
      if (mItems[i].mMeld >= 0) {
        const int meld(mItems[i].mMeld);
        switch (mItems[i].mType) {
          case dtUInt8: 
            mItems[meld].mArray[j].ui32 = (mItems[meld].mArray[j].ui16 & 0xffff) | ((mItems[i].mArray[j].ui8 << 16) & 0xff0000);
          break;
          case dtUInt16: 
            mItems[meld].mArray[j].ui32 = (mItems[meld].mArray[j].ui16 & 0xffff) | ((mItems[i].mArray[j].ui16 << 16) & 0xff0000);
            break;
          case dtInt8: 
            mItems[meld].mArray[j].i32 = (mItems[meld].mArray[j].ui16 & 0xffff) | ((mItems[i].mArray[j].i8 << 16) & 0xffff0000);
            break;
          case dtInt16: 
            mItems[meld].mArray[j].i32 = (mItems[meld].mArray[j].ui16 & 0xffff) | ((mItems[i].mArray[j].i16 << 16) & 0xffff0000);
            break;
          case dtInt32:
          case dtUInt32:
            std::cerr << "Unable to meld 32 bits" << std::endl;
            exit(1);
        }
      }
    }
  }

  mqSet = true;

  if (qDump) dump(std::cout);

  return mqSet;
}

void
PD0::Common::dump(std::ostream& os)
{
  for (tItems::size_type i(0), e(mItems.size()); i < e; ++i) {
    if (mItems[i].mMeld >= 0) 
      continue;

    os << mItems[i].mName;
    switch (mItems[i].mType) {
      case dtUInt8:  os << " uint8 "; break;
      case dtInt8:   os << "  int8 "; break;
      case dtUInt16: os << " uint16"; break;
      case dtInt16:  os << "  int16"; break;
      case dtUInt32: os << " uint32"; break;
      case dtInt32:  os << "  int32"; break;
    }
    for (tArray::size_type j(0), je(mItems[i].mArray.size()); j < je; ++j) {
      switch (mItems[i].mType) {
        case dtUInt8:  os << " " << (uint16_t)  mItems[i].mArray[j].ui32; break;
        case dtUInt16: os << " " << (uint16_t) mItems[i].mArray[j].ui32; break;
        case dtUInt32: os << " " << (uint32_t) mItems[i].mArray[j].ui32; break;
        case dtInt8:   os << " " << (int16_t)   mItems[i].mArray[j].ui32; break;
        case dtInt16:  os << " " << (int16_t)  mItems[i].mArray[j].ui32; break;
        case dtInt32:  os << " " << (int32_t)  mItems[i].mArray[j].ui32; break;
      }
    }
    os << std::endl;
  }
}

void
PD0::setupNetCDFVars(NetCDF& nc)
{
  const int iDim(nc.createDim("i"));
  const int jDim(nc.createDim("j", mMaxNumberOfCells > 0 ? mMaxNumberOfCells : NC_UNLIMITED, 128));
  const int k4Dim(nc.createDim("k4", 4));
  const int k8Dim(nc.createDim("k8", 8));

  mFixed.ncSetup(nc, iDim, k8Dim);
  mVariable.ncSetup(nc, iDim);
  mVelocity.ncSetup(nc, iDim, jDim, k4Dim);
  mCorrelation.ncSetup(nc, iDim, jDim, k4Dim);
  mEcho.ncSetup(nc, iDim, jDim, k4Dim);
  mPercentGood.ncSetup(nc, iDim, jDim, k4Dim);
  mBottomTrack.ncSetup(nc, iDim, k4Dim);
}

int
PD0::Common::Item::ncType() const
{
  switch(mType) {
    case dtUInt8:   return NC_UBYTE;
    case dtInt8:   return NC_BYTE;
    case dtUInt16: return NC_USHORT;
    case dtInt16:  return NC_SHORT;
    case dtUInt32: return NC_UINT;
    case dtInt32:  return NC_INT;
  }
  return NC_UBYTE; // Should not be reached
}

void
PD0::Common::ncSetup(NetCDF& nc,
                     const int iDim)
{
  ncSetup(nc, iDim, iDim);
}

void
PD0::Common::ncSetup(NetCDF& nc,
                     const int iDim,
                     const int kDim)
{
  for (tItems::size_type i(0), e(mItems.size()); i < e; ++i) {
    if (mItems[i].mMeld < 0) {
      if (mItems[i].mArray.size() == 1) {
        mItems[i].mVarId = nc.createVar(mItems[i].mName, mItems[i].ncType(), &iDim, 1, mItems[i].mUnits);
      } else {
        const int dims[2] = {iDim, kDim};
        mItems[i].mVarId = nc.createVar(mItems[i].mName, mItems[i].ncType(), dims, 2, mItems[i].mUnits);
      }
    }
  }
}

void
PD0::Common::ncDump(NetCDF& nc,
                    const size_t index)
{
  for (tItems::size_type i(0), e(mItems.size()); i < e; ++i) {
    if (mItems[i].mMeld >= 0) 
      continue;

    if (mItems[i].mArray.size() == 1) {
      switch (mItems[i].mType) {
        case dtUInt8:  nc.putVar(mItems[i].mVarId, index, mItems[i].mArray[0].ui8); break;
        case dtInt8:   nc.putVar(mItems[i].mVarId, index, mItems[i].mArray[0].i8); break;
        case dtUInt16: nc.putVar(mItems[i].mVarId, index, mItems[i].mArray[0].ui16); break;
        case dtInt16:  nc.putVar(mItems[i].mVarId, index, mItems[i].mArray[0].i16); break;
        case dtUInt32: nc.putVar(mItems[i].mVarId, index, mItems[i].mArray[0].ui32); break;
        case dtInt32:  nc.putVar(mItems[i].mVarId, index, mItems[i].mArray[0].i32); break;
      }
    } else {
      for (tArray::size_type j(0), je(mItems[i].mArray.size()); j < je; ++j) {
        const size_t dims[2] = {index, j};
        switch (mItems[i].mType) {
          case dtUInt8:  nc.putVar(mItems[i].mVarId, dims, 2, mItems[i].mArray[j].ui8); break;
          case dtInt8:   nc.putVar(mItems[i].mVarId, dims, 2, mItems[i].mArray[j].i8); break;
          case dtUInt16: nc.putVar(mItems[i].mVarId, dims, 2, mItems[i].mArray[j].ui16); break;
          case dtInt16:  nc.putVar(mItems[i].mVarId, dims, 2, mItems[i].mArray[j].i16); break;
          case dtUInt32: nc.putVar(mItems[i].mVarId, dims, 2, mItems[i].mArray[j].ui32); break;
          case dtInt32:  nc.putVar(mItems[i].mVarId, dims, 2, mItems[i].mArray[j].i32); break;
        }
      }
    }
  }
}

PD0::Fixed::Fixed()
{
  mItems.push_back(Item("firmware_ver", dtUInt8));
  mItems.push_back(Item("firmware_rev", dtUInt8));
  mItems.push_back(Item("system_config", dtUInt16));
  mItems.push_back(Item("sim_flag", dtUInt8));
  mItems.push_back(Item("lag_length", dtUInt8));
  mItems.push_back(Item("n_beams", dtUInt8));
  mItems.push_back(Item("n_cells", dtUInt8));
  mItems.push_back(Item("pings_per_ensemble", dtUInt16));
  mItems.push_back(Item("depth_cell_length", dtUInt16));
  mItems.push_back(Item("blank_after_transmit", dtUInt16));
  mItems.push_back(Item("profiling_mode", dtUInt8));
  mItems.push_back(Item("low_corr_threshold", dtUInt8));
  mItems.push_back(Item("n_code_reps", dtUInt8));
  mItems.push_back(Item("percent_good_minimum", dtUInt8));
  mItems.push_back(Item("error_velocity_maximum", dtUInt16));
  mItems.push_back(Item("TPP_minutes", dtUInt8));
  mItems.push_back(Item("TPP_seconds", dtUInt8));
  mItems.push_back(Item("TPP_hundredths", dtUInt8));
  mItems.push_back(Item("coordinate_transform", dtUInt8));
  mItems.push_back(Item("heading_alignment", dtInt16));
  mItems.push_back(Item("heading_bias", dtInt16));
  mItems.push_back(Item("sensor_source", dtUInt8));
  mItems.push_back(Item("sensor_available", dtUInt8));
  mItems.push_back(Item("bin_1_distance", dtUInt16, "cm"));
  mItems.push_back(Item("xmit_pulse_length", dtUInt16, "cm"));
  mItems.push_back(Item("wp_ref_layer_average", dtUInt16));
  mItems.push_back(Item("false_target_threshold", dtUInt8));
  mItems.push_back(Item("fixed_spare0", dtUInt8));
  mItems.push_back(Item("transmit_lag_distance", dtUInt16, "cm"));
  mItems.push_back(Item("cpu_board_serial_number", dtUInt8, 8));
  mItems.push_back(Item("system_bandwidth", dtUInt16));
  mItems.push_back(Item("fixed_spare1", dtUInt8));
  mItems.push_back(Item("base_frequency_index", dtUInt8));
}

size_t
PD0::Fixed::nCells() const
{
  return mItems[6].mArray[0].ui8;
}

PD0::Variable::Variable()
{
  mItems.push_back(Item("ensemble_number", dtUInt16));
  mItems.push_back(Item("rtc_year", dtUInt8));
  mItems.push_back(Item("rtc_month", dtUInt8));
  mItems.push_back(Item("rtc_day", dtUInt8));
  mItems.push_back(Item("rtc_hour", dtUInt8));
  mItems.push_back(Item("rtc_minute", dtUInt8));
  mItems.push_back(Item("rtc_second", dtUInt8));
  mItems.push_back(Item("rtc_hundredths", dtUInt8));
  mItems.push_back(Item("ensemble_number_msb", dtUInt8, 1, 0));
  mItems.push_back(Item("bit_result", dtUInt16));
  mItems.push_back(Item("speed_of_sound", dtUInt16, "m/sec"));
  mItems.push_back(Item("depth_of_transducer", dtUInt16, "decimeter"));
  mItems.push_back(Item("heading", dtUInt16, "centidegree"));
  mItems.push_back(Item("pitch", dtInt16, "centidegree"));
  mItems.push_back(Item("roll", dtInt16, "centidegree"));
  mItems.push_back(Item("salinity", dtUInt16, "ppt"));
  mItems.push_back(Item("temperature", dtInt16, "centidegree"));
  mItems.push_back(Item("mpt_minutes", dtUInt8));
  mItems.push_back(Item("mpt_seconds", dtUInt8));
  mItems.push_back(Item("mpt_hundredths", dtUInt8));
  mItems.push_back(Item("heading_stddev", dtUInt8, "degrees"));
  mItems.push_back(Item("pitch_stddev", dtUInt8, "decidegrees"));
  mItems.push_back(Item("roll_stddev", dtUInt8, "decidegrees"));
  mItems.push_back(Item("adc0", dtUInt8));
  mItems.push_back(Item("adc1", dtUInt8));
  mItems.push_back(Item("adc2", dtUInt8));
  mItems.push_back(Item("adc3", dtUInt8));
  mItems.push_back(Item("adc4", dtUInt8));
  mItems.push_back(Item("adc5", dtUInt8));
  mItems.push_back(Item("adc6", dtUInt8));
  mItems.push_back(Item("adc7", dtUInt8));
  mItems.push_back(Item("error_status", dtUInt32));
  mItems.push_back(Item("var_spare0", dtUInt16));
  mItems.push_back(Item("pressure", dtUInt32, "decapascals"));
  mItems.push_back(Item("pressure_variance", dtUInt32, "decapascals"));
  mItems.push_back(Item("var_spare1", dtUInt8));
  mItems.push_back(Item("rtc_2k_century", dtUInt8));
  mItems.push_back(Item("rtc_2k_year", dtUInt8));
  mItems.push_back(Item("rtc_2k_month", dtUInt8));
}

bool
PD0::Velocity::load(std::istream& is,
                    PD0& pd0)
{
  const size_t nCells(pd0.mFixed.nCells()); // Number of depth cells from fixed header
  const size_t nSize(nCells * 4); // Number of entries, 4 beams * number of depth cells

  if (mItems.empty() || (mItems[0].mArray.size() != nSize)) {
    mItems.clear();
    mItems.push_back(Item("velocity", dtInt16, nSize, -1, "mm/s"));
  }

  return ((Common *) this)->load(is, pd0);
}

void
PD0::Velocity::ncSetup(NetCDF& nc,
                       const int iDim,
                       const int jDim,
                       const int kDim
                       )
{
  const int dims[3] = {iDim, jDim, kDim};
  mVarId = nc.createVar("velocity", NC_SHORT, dims, 3, "mm/s");
}

void
PD0::Velocity::ncDump(NetCDF& nc,
                      size_t index)
{
  size_t n(mItems[0].mArray.size());
  int32_t *ptr(new int32_t[n]);
  const size_t dims[3] = {index, 0, 0};
  const size_t cnt[3] = {1, n / 4, 4};

  for (tArray::size_type j(0); j < n; ++j) {
    ptr[j] = mItems[0].mArray[j].i16;
  }

  nc.putVars(mVarId, dims, cnt, ptr);

  delete ptr;
}

bool
PD0::Correlation::load(std::istream& is,
                       PD0& pd0)
{
  const size_t nCells(pd0.mFixed.nCells()); // Number of depth cells from fixed header
  const size_t nSize(nCells * 4); // Number of entries, 4 beams * number of depth cells

  if (mItems.empty() || (mItems[0].mArray.size() != nSize)) {
    mItems.clear();
    mItems.push_back(Item(mName, dtUInt8, nSize));
  }

  return ((Common *) this)->load(is, pd0);
}

void
PD0::Correlation::ncSetup(NetCDF& nc,
                       const int iDim,
                       const int jDim,
                       const int kDim)
{
  const int dims[3] = {iDim, jDim, kDim};
  mVarId = nc.createVar(mName, NC_UBYTE, dims, 3, std::string());
}

void
PD0::Correlation::ncDump(NetCDF& nc,
                         size_t index)
{
  size_t n(mItems[0].mArray.size());
  uint32_t *ptr(new uint32_t[n]);
  const size_t dims[3] = {index, 0, 0};
  const size_t cnt[3] = {1, n / 4, 4};

  for (tArray::size_type j(0); j < n; ++j) {
    ptr[j] = mItems[0].mArray[j].ui8;
  }

  nc.putVars(mVarId, dims, cnt, ptr);

  delete ptr;
}

PD0::BottomTrack::BottomTrack()
{
  mItems.push_back(Item("bt_pings_per_ensemble", dtUInt16, "pings"));
  mItems.push_back(Item("bt_delay_before_reacquire", dtUInt16));
  mItems.push_back(Item("bt_corr_mag_min", dtUInt8));
  mItems.push_back(Item("bt_eval_amp_min", dtUInt8));
  mItems.push_back(Item("bt_percent_good_min", dtUInt8));
  mItems.push_back(Item("bt_mode", dtUInt8));
  mItems.push_back(Item("bt_error_velocity_maximum", dtUInt16, "mm/s"));
  mItems.push_back(Item("bt_reserved_0", dtUInt32));
  mItems.push_back(Item("bt_range", dtUInt16, 4, -1, "cm"));
  mItems.push_back(Item("bt_velocity", dtInt16, 4, -1, "mm/s"));
  mItems.push_back(Item("bt_correlation", dtUInt8, 4));
  mItems.push_back(Item("bt_eval_amp", dtUInt8, 4));
  mItems.push_back(Item("bt_percent_good", dtUInt8, 4));
  mItems.push_back(Item("bt_ref_layer_min", dtUInt16, "decimeter"));
  mItems.push_back(Item("bt_ref_layer_near", dtUInt16, "decimeter"));
  mItems.push_back(Item("bt_ref_layer_far", dtUInt16, "decimeter"));
  mItems.push_back(Item("bt_ref_layer_velocity", dtInt16, 4, -1, "mm/s"));
  mItems.push_back(Item("bt_ref_correlation", dtUInt8, 4));
  mItems.push_back(Item("bt_ref_int", dtUInt8, 4));
  mItems.push_back(Item("bt_ref_percent_good", dtUInt8, 4));
  mItems.push_back(Item("bt_max_depth", dtUInt16, "decimeter"));
  mItems.push_back(Item("bt_rssi_amp", dtUInt8, 4));
  mItems.push_back(Item("bt_gain", dtUInt8));
  mItems.push_back(Item("bt_range_msb", dtUInt8, 4, 9));
}

bool
PD0::BottomTrack::load(std::istream& is,
                       PD0& pd0)
{
  return ((Common *) this)->load(is, pd0);
}

uint8_t
PD0::maxNumberOfCells(const std::string& fn)
{
  std::ifstream is(fn.c_str());

  if (!is) {
    const std::string msg("Error opening '" + fn + "', " + strerror(errno));
    std::cerr << msg << std::endl;
    throw(MyException(msg));
  }

  uint8_t maxnCells(0);
  const std::streamoff chkSum(2);

  for(std::streampos spos(is.tellg()); (is.get() == 0x7f) && (is.get() == 0x7f); spos = is.tellg()) {
    const std::streamoff nBytes((is.get() & 0xff) | ((is.get() << 8) & 0xff00));
    is.get(); // spare
    const size_t nTypes(is.get());
    std::vector<std::streamoff> offsets(nTypes, 0);
    for (size_t i(0); i < nTypes; ++i) {
      offsets[i] = (is.get() & 0xff) | ((is.get() << 8) & 0xff00);
    }

    for (size_t i(0); i < nTypes; ++i) {
      is.seekg(spos + offsets[i], std::ios::beg);
      const size_t hdr((is.get() & 0xff) | ((is.get() << 8) & 0xff00));
      if (hdr == 0x0000) { // Fixed
        is.seekg(7, std::ios::cur); // Move to number of cells
        const uint8_t nCells(is.get());
        maxnCells = (maxnCells > nCells) ? maxnCells : nCells;
        break; // Go to next block
      }
    }

    is.seekg(spos + nBytes + chkSum, std::ios::beg);
  }

  return maxnCells;
}
