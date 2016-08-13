#include "SGMerge.H"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>

//  NetCDF ncid(ofn);
//  ncid.enddef();
//  ncid.close();

SGMerge::SGMerge(const char *fn)
  : mFilename(fn)
  , oncid(-1)
{
  mDims.insert(std::make_pair("index", 0));
}

SGMerge::~SGMerge()
{
  if (oncid >= 0) {
    const int status(nc_close(oncid));
    if (status)
      std::cerr << "Error closing " << mFilename << ", " << nc_strerror(status) << std::endl;
    oncid = -1;
  }
}

bool
SGMerge::loadFileHeader(const char *fn)
{
  int ncid;
  int status(nc_open(fn, NC_NOWRITE, &ncid));
  if (status) {
    std::cerr << "Error opening '" << fn << "' for reading, " << nc_strerror(status) << std::endl;
    return false;
  }

  { // global attributes
    int natts;
    if ((status = nc_inq_natts(ncid, &natts))) {
      std::cerr << "Error inq_natts '" << fn << "', " << nc_strerror(status) << std::endl;
      nc_close(ncid);
      return false;
    }
    for (size_t i(0); i < natts; ++i) {
      Attribute attr(ncid, NC_GLOBAL, i, fn);
      tAttr::iterator it(mGlobalAttr.find(attr.name));
      if (it == mGlobalAttr.end()) {
        mGlobalAttr.insert(std::make_pair(attr.name, attr));
      } else if (it->second.value != attr.value) {
        Data& data(it->second.value);
        if (attr.value.xtype == data.xtype) { // append
          if (data.xtype == NC_CHAR) { // split with new lines
            data.cVal.push_back('\n');
          }
          data += attr.value;
        } else {
          std::cout << "Global attribute type mismatch" << std::endl;
          std::cout << " attr " << attr.toString() << std::endl;
          std::cout << "oattr  " << it->second.toString() << std::endl;
        }
      }
    }
  }

  int ndims, nvars, natts, nunlim;
   
  if ((status = nc_inq(ncid, &ndims, &nvars, &natts, &nunlim))) {
    std::cerr << "Error inq '" << fn << "', " << nc_strerror(status) << std::endl;
    nc_close(ncid);
    return false;
  }

  tDims dims;

  for (size_t id(0); id < ndims; ++id) {
    char name[NC_MAX_NAME+1];
    size_t len;
    if ((status = nc_inq_dim(ncid, id, name, &len))) {
      std::cerr << "Error inq_dim '" << fn << "', " << id << ", " 
                << nc_strerror(status) << std::endl;
      nc_close(ncid);
      return false;
    }
    dims.push_back(Dimension(name, len));
    if (strncmp(name, "string_", 7)) { // Doesn't begin with string_
      tDimMap::iterator it(mDims.find(name));
      if (it == mDims.end()) {
        mDims.insert(std::make_pair(name, len));
      } else {
        it->second += len;
      }
    }
  }

  for (size_t id(0); id < nvars; ++id) {
    char name[NC_MAX_NAME+1];
    nc_type xtype;
    int nvDim;
    int dimids[NC_MAX_VAR_DIMS];
    int nvAtt;
    if ((status = nc_inq_var(ncid, id, name, &xtype, &nvDim, dimids, &nvAtt))) {
      std::cerr << "Error inq_var '" << fn << "', " << id << ", " 
                << nc_strerror(status) << std::endl;
      nc_close(ncid);
      return false;
    }

    tVars::iterator it(mVars.find(name));

    if (it == mVars.end()) { // Not known so populate
      struct Variable var(name, xtype);
      for (size_t j(0); j < nvAtt; ++j) {
        struct Attribute attr(ncid, id, j, fn);
        if (attr.name.empty()) continue;
        var.mAttr.insert(std::make_pair(attr.name, attr));
      }
      size_t totLen(0);
      for (size_t j(0); j < nvDim; ++j) { // Walk through this variable's dimensions
        const Dimension& dim(dims[dimids[j]]);
        const std::string& dname(dim.name);
        totLen += dim.len;
        if (dname.substr(0, 7) != "string_") {
          var.mDims.push_back(dname);
        }
      }
      if (!var.check(ncid, id, totLen, fn)) { // Check if conversion needed
        nc_close(ncid);
        return false;
      }
      if (var.mDims.empty()) { // No dims so fix that up
        if (var.xtype == NC_CHAR) { // No conversion, so 
          char buffer[NC_MAX_NAME + 5];
          snprintf(buffer, sizeof(buffer), "str_%s", name);
          mDims.insert(std::make_pair(buffer, totLen)); 
          var.mDims.push_back(buffer);
        } else { // Some conversion
          var.mDims.push_back("index");
          if (var.nFields > 1) { // Need a second dimension
            char dimName[1024];
            snprintf(dimName, sizeof(dimName), "j_%lu", var.nFields);
            var.mDims.push_back(dimName);
            if (mDims.find(dimName) == mDims.end()) { // Add in this dim
              mDims.insert(std::make_pair(dimName, var.nFields));
            }
          }
        }
      }
      it = mVars.insert(std::make_pair(name, var)).first;
    } else if (it->second.xtype == NC_CHAR) { // No conversion
      char buffer[NC_MAX_NAME + 5];
      snprintf(buffer, sizeof(buffer), "str_%s", name);
      size_t totLen(0);
      for (size_t j(0); j < nvDim; ++j) { // Walk through this variable's dimensions
        totLen += dims[dimids[j]].len;
      }
      tDimMap::iterator jt(mDims.find(buffer));
      jt->second += 1 + totLen; // New line plus this string
    }
  }

  if ((status = nc_close(ncid))) {
    std::cerr << "Error closing '" << fn << "', " << nc_strerror(status) << std::endl;
    return false;
  }

  ++mDims["index"];
  return true; 
}

void
SGMerge::updateHeader()
{
  int ncid;
  int status(nc_create(mFilename.c_str(), NC_CLOBBER | NC_NETCDF4, &ncid));
  if (status) {
    std::cerr << "Error creating '" << mFilename << "', " << nc_strerror(status) << std::endl;
    return;
  }

  for (tAttr::const_iterator it(mGlobalAttr.begin()), et(mGlobalAttr.end()); it != et; ++it) {
    const Attribute& attr(it->second);
    const int status(attr.value.putAttr(ncid, NC_GLOBAL, attr.name));
    if (status) {
      nc_close(ncid);
      return;
    }
  }

  for (tDimMap::const_iterator it(mDims.begin()), et(mDims.end()); it != et; ++it) {
    int id;
    if ((status = nc_def_dim(ncid, it->first.c_str(), it->second, &id))) {
      std::cerr << "Error def_dim '" << mFilename << "', " << nc_strerror(status) << std::endl;
      nc_close(ncid);
      return;
    }
    mDimIDs.insert(std::make_pair(it->first, id));
    mDimCnt.insert(std::make_pair(it->first, 0));
  }

  for (tVars::const_iterator it(mVars.begin()), et(mVars.end()); it != et; ++it) {
    const Variable& var(it->second);
    const size_t n(var.mDims.size());
    int dimids[n];
    for (size_t i(0); i < n; ++i) {
      dimids[i] = mDimIDs[var.mDims[i]];
    }
    int id;
    if ((status = nc_def_var(ncid, var.name.c_str(), var.xtype, n, dimids, &id))) {
      std::cerr << "Error def_var '" << mFilename << "', " << nc_strerror(status) << std::endl;
      nc_close(ncid);
      return;
    }
    mVarIDs.insert(std::make_pair(var.name, id));
    if ((status = nc_def_var_deflate(ncid, id, NC_SHUFFLE, 1, 3))) {
      std::cerr << "Error def_var_deflate '" << mFilename << "', " 
                << nc_strerror(status) << std::endl;
      nc_close(ncid);
      return;
    }
    for (tAttr::const_iterator it(var.mAttr.begin()), et(var.mAttr.end()); it != et; ++it) {
      const Attribute& attr(it->second);
      if ((status = attr.value.putAttr(ncid, id, attr.name))) {
        nc_close(ncid);
        return;
      }
    }
  }

  if ((status = nc_enddef(ncid))) {
    std::cerr << "Error enddef '" << mFilename << "', " << nc_strerror(status) << std::endl;
    nc_close(ncid);
    return;
  }
  oncid = ncid;
}

bool
SGMerge::mergeFile(const char *fn)
{
  int ncid;
  int status(nc_open(fn, NC_NOWRITE, &ncid));
  if (status) {
    std::cerr << "Error opening '" << fn << "' for reading, " << nc_strerror(status) << std::endl;
    return false;
  }
 
  int ndims, nvars, natts, nunlim;
   
  if ((status = nc_inq(ncid, &ndims, &nvars, &natts, &nunlim))) {
    std::cerr << "Error inq '" << fn << "', " << nc_strerror(status) << std::endl;
    nc_close(ncid);
    return false;
  }
  
  tDims dims;
  tDimMap nextCnt(mDimCnt);

  for (size_t id(0); id < ndims; ++id) {
    char name[NC_MAX_NAME+1];
    size_t len;
    if ((status = nc_inq_dim(ncid, id, name, &len))) {
      std::cerr << "Error inq_dim '" << fn << "', " << id << ", " 
                << nc_strerror(status) << std::endl;
      nc_close(ncid);
      return false;
    }
    dims.push_back(Dimension(name, len));
  }

  for (size_t id(0); id < nvars; ++id) {
    char name[NC_MAX_NAME+1];
    nc_type xtype;
    int nvDim;
    int dimids[NC_MAX_VAR_DIMS];
    int nvAtt;
    if ((status = nc_inq_var(ncid, id, name, &xtype, &nvDim, dimids, &nvAtt))) {
      std::cerr << "Error inq_var '" << fn << "', " << id << ", " 
                << nc_strerror(status) << std::endl;
      nc_close(ncid);
      return false;
    }
    size_t len(nvDim ? 0 : 1);
    for (size_t i(0); i < nvDim; ++i) {
      len += dims[dimids[i]].len;
    }
    Data data(xtype, len);
    data.getVar(ncid, id); // Get the actual data

    const Variable &oVar(mVars.find(name)->second); // We know it is there
    if (oVar.qChars) { // Convert a series of digits to an vector of ints
      data.toDigits();
    } else if (oVar.qArray) { // Convert a series of number to a vector
      data.toArray(oVar.nSkip);
      len = data.size();
    }
   
    // Now build target start/cnt

    const size_t nODims(oVar.mDims.size()); // Number of output dims
    size_t start[nODims]; // Start/offset to write data at
    size_t cnt[nODims]; // Length of the data in each dimension
    std::string sName; // str_ name

    for (size_t i(0); i < nODims; ++i) { // Loop over output dims
      const std::string& dName(oVar.mDims[i]);
      if (dName.substr(0, 4) == "str_") sName = dName;

      if (i == 0) {
        start[i] = mDimCnt[dName];
        cnt[i] = len;
      } else { // Not first
        start[i] = 0;
        cnt[i] = mDims[dName];
        cnt[0] /= cnt[i];
      }
      nextCnt[dName] = start[0] + cnt[0];
    }

    if (!sName.empty() && (start[0] != 0)) { // Insert a new line if not first one
      data.cVal.insert(data.cVal.begin(), '\n');
      ++cnt[0];
      nextCnt[sName] = start[0] + cnt[0];
    }

    data.putVar(oncid, mVarIDs[oVar.name], start, cnt);
  }
 
  if ((status = nc_close(ncid))) {
    std::cerr << "Error closing '" << fn << "', " << nc_strerror(status) << std::endl;
  }

  mDimCnt = nextCnt; // Advance offsets
  return true;
}

SGMerge::Attribute::Attribute(const int ncid,
                              const size_t vid,
                              const size_t id,
                              const char *fn)
{
  char aname[NC_MAX_NAME+1];
  int status(nc_inq_attname(ncid, vid, id, aname));
  if (status) {
    std::cerr << "Error inq_attname '" << fn << "', " << vid << ", " << id << ", " 
              << nc_strerror(status) << std::endl;
    return;
  }

  size_t len;
  nc_type xtype;

  if ((status = nc_inq_att(ncid, vid, aname, &xtype, &len))) {
    std::cerr << "Error inq_att'" << fn << "', " << vid << ", " << aname << ", " 
              << nc_strerror(status) << std::endl;
    return;
  }

  name = aname;

  value = Data(xtype, len);
  value.getAttr(ncid, vid, name);
}

std::string
SGMerge::Attribute::toString() const
{
  std::ostringstream os;
  os << name << " " << value.xtype << " " << value.size() << " " << value.toString();
    
  return os.str();
}

bool
SGMerge::Variable::check(const int ncid,
                         const size_t id,
                         const size_t len,
                         const char *fn)
{
  if (xtype != NC_CHAR) return true; // No conversion needed
  Data data(xtype, len);
  const int status(data.getVar(ncid, id));
  if (status) {
    std::cerr << "Error get_var_text '" << fn << "', " << id << ", " << len << ", " 
              << nc_strerror(status) << std::endl; 
    return false;
  }

  if (mDims.empty()) {
    checkArray(data);
  } else {
    qChars = data.qDigits();
    xtype = qChars ? NC_BYTE : xtype;
  }
  return true;
}

void
SGMerge::Variable::checkArray(const Data& data)
{
  const char *delim(",");
  const size_t n(data.cVal.size());
  char buffer[n + 1];
  bool qInt(true);

  nSkip = 0;
  nFields = 0;
  qArray = true;
  strncpy(buffer, data.cVal.data(), n);
  buffer[n] = '\0';

  for (char *ptr(strtok(buffer, delim)); qArray && ptr; ptr = strtok(NULL, delim)) {
    if ((nFields == 0) && (*ptr == '$')) {
      nSkip = 1;
      continue;
    }
    char *eptr;
    const double value(strtod(ptr, &eptr)); 
    qArray &= (ptr != eptr);
    qInt &= index(ptr, '.') == 0;
    ++nFields;
  }
  xtype = qArray ? (qInt ? NC_INT : NC_DOUBLE) : NC_CHAR;
}

bool
SGMerge::Data::operator == (const Data& rhs) const
{
  return (xtype == rhs.xtype)
      && (cVal == rhs.cVal)
      && (iVal == rhs.iVal)
      && (fVal == rhs.fVal)
      && (dVal == rhs.dVal)
      ;
}

struct SGMerge::Data&
SGMerge::Data::operator += (const Data& rhs)
{
  if (xtype != rhs.xtype) {
    std::cerr << "Data type mismatch " << xtype << " != " << rhs.xtype << std::endl;
    return *this;
  }
  switch (xtype) {
    case NC_CHAR:   cVal.insert(cVal.end(), rhs.cVal.begin(), rhs.cVal.end()); break;
    case NC_INT:    iVal.insert(iVal.end(), rhs.iVal.begin(), rhs.iVal.end()); break;
    case NC_FLOAT:  fVal.insert(fVal.end(), rhs.fVal.begin(), rhs.fVal.end()); break;
    case NC_DOUBLE: dVal.insert(dVal.end(), rhs.dVal.begin(), rhs.dVal.end()); break;
    default: 
      std::cerr << "unsupported data type " << xtype << std::endl;
      exit(1);
  }
  return *this;
}

size_t
SGMerge::Data::size() const
{
  switch (xtype) {
    case NC_CHAR: return cVal.size();
    case NC_INT: return iVal.size();
    case NC_FLOAT: return fVal.size();
    case NC_DOUBLE: return dVal.size();
    default: 
      std::cerr << "unsupported data type " << xtype << std::endl;
      exit(1);
  }
  return 0;
}

void
SGMerge::Data::resize(const size_t len)
{
  switch (xtype) {
    case NC_CHAR: cVal.resize(len); break;
    case NC_INT: iVal.resize(len); break;
    case NC_FLOAT: fVal.resize(len); break;
    case NC_DOUBLE: dVal.resize(len); break;
    default: 
      std::cerr << "unsupported data type " << xtype << std::endl;
      exit(1);
  }
}

int
SGMerge::Data::getAttr(const int ncid,
                       const int vid,
                       const std::string& name)
{
  int status(NC_EBADTYPE);

  switch (xtype) {
    case NC_CHAR: 
      status = nc_get_att_text(ncid, vid, name.c_str(), cVal.data());
      break;
    case NC_INT: 
      status = nc_get_att_int(ncid, vid, name.c_str(), iVal.data());
      break;
    case NC_FLOAT: 
      status = nc_get_att_float(ncid, vid, name.c_str(), fVal.data());
      break;
    case NC_DOUBLE: 
      status = nc_get_att_double(ncid, vid, name.c_str(), dVal.data());
      break;
  }

  if (status) 
    std::cerr << "Error put_att " << name << ", " << nc_strerror(status);
  
  return status;
}

int
SGMerge::Data::putAttr(const int ncid,
                       const int vid,
                       const std::string& name) const
{
  int status(NC_EBADTYPE);

  switch (xtype) {
    case NC_CHAR: 
      status = nc_put_att_text(ncid, vid, name.c_str(), cVal.size(), cVal.data());
      break;
    case NC_INT: 
      status = nc_put_att_int(ncid, vid, name.c_str(), xtype, iVal.size(), iVal.data());
      break;
    case NC_FLOAT: 
      status = nc_put_att_float(ncid, vid, name.c_str(), xtype, fVal.size(), fVal.data());
      break;
    case NC_DOUBLE: 
      status = nc_put_att_double(ncid, vid, name.c_str(), xtype, dVal.size(), dVal.data());
      break;
  }

  if (status) 
    std::cerr << "Error put_att " << name << ", " << nc_strerror(status);
  
  return status;
}

int
SGMerge::Data::getVar(const int ncid,
                      const int vid)
{
  int status(NC_EBADTYPE);

  switch (xtype) {
    case NC_CHAR: 
      status = nc_get_var_text(ncid, vid, cVal.data());
      break;
    case NC_INT: 
      status = nc_get_var_int(ncid, vid, iVal.data());
      break;
    case NC_FLOAT: 
      status = nc_get_var_float(ncid, vid, fVal.data());
      break;
    case NC_DOUBLE: 
      status = nc_get_var_double(ncid, vid, dVal.data());
      break;
  }
  
  if (status) 
    std::cerr << "Error get_var, " << nc_strerror(status);
  
  return status;
}

int
SGMerge::Data::putVar(const int ncid,
                      const int vid,
                      const size_t start[],
                      const size_t cnt[]) const
{
  int status(NC_EBADTYPE);

  switch (xtype) {
    case NC_CHAR: 
      status =  nc_put_vara_text(ncid, vid, start, cnt, cVal.data());
      break;
    case NC_INT: 
      status =  nc_put_vara_int(ncid, vid, start, cnt, iVal.data());
      break;
    case NC_FLOAT: 
      status =  nc_put_vara_float(ncid, vid, start, cnt, fVal.data());
      break;
    case NC_DOUBLE: 
      status =  nc_put_vara_double(ncid, vid, start, cnt, dVal.data());
      break;
  }
  
  if (status) 
    std::cerr << "Error put_var, " << nc_strerror(status);
  
  return status;
}

bool
SGMerge::Data::qDigits() const
{
  bool q(xtype == NC_CHAR);
  for (size_t i(0), e(cVal.size()); q && (i < e); ++i) {
    const char c(cVal[i]);
    q &= (c >= '0') && (c <= '9');
  }
  return q;
}

void 
SGMerge::Data::toDigits()
{
  if (xtype != NC_CHAR) return;
  iVal.resize(cVal.size());

  for (size_t i(0), e(cVal.size()); i < e; ++i) {
    iVal[i] = cVal[i] - '0';
  }
  xtype = NC_INT;
  cVal.clear();
}

void
SGMerge::Data::toArray(const size_t nSkip)
{
  if (xtype != NC_CHAR) return;
  const char *delim(",");
  const size_t n(cVal.size());
  size_t cnt(0);
  char buffer[n + 1];
  strncpy(buffer, cVal.data(), n);
  buffer[n] = '\0';

  for (char *ptr(strtok(buffer, delim)); ptr; ptr = strtok(NULL, delim)) {
    if (nSkip >= ++cnt) continue; // Skip this field
    dVal.push_back(strtod(ptr, NULL));
  }
  xtype = NC_DOUBLE;
  cVal.clear();
}

std::string
SGMerge::Data::toString() const
{
  std::ostringstream os;

  switch (xtype) {
    case NC_CHAR: os << std::string(cVal.data(), cVal.size()); return os.str();
    case NC_INT: 
      for (size_t i(0), e(iVal.size()); i < e; ++i) 
        os << ((i == 0) ? "" : ",") << iVal[i];
      return os.str();
    case NC_FLOAT: 
      for (size_t i(0), e(fVal.size()); i < e; ++i) 
        os << ((i == 0) ? "" : ",") << fVal[i];
      return os.str();
    case NC_DOUBLE: 
      for (size_t i(0), e(dVal.size()); i < e; ++i) 
        os << ((i == 0) ? "" : ",") << dVal[i];
      return os.str();
  }
  return "unsupported data type";
}
