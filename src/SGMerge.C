#include "SGMerge.H"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <climits>
#include <cmath>

namespace {
  bool ncOp(const int status, const std::string& msg, const std::string& fn, const int ncid) {
    if (status) {
      std::cerr << "Error " << msg << ", '" << fn << "', " << nc_strerror(status) << std::endl;
      if (ncid >= 0) nc_close(ncid);
      return true;
    }
    return false;
  } // ncOp
} // anonymous namespace

SGMerge::SGMerge(const char *fn,
                 const bool qVerbose)
  : mFilename(fn)
  , mqVerbose(qVerbose)
  , mNCID(-1)
{
  mDims.insert(std::make_pair("index", 0));
  mDimNew.insert("index");
}

SGMerge::~SGMerge()
{
  if (mNCID >= 0) {
    ncOp(nc_close(mNCID), "closing", mFilename, -1);
    mNCID = -1;
  }
}

void
SGMerge::loadAttr(const int ncid,
                  const int id,
                  const size_t nAttr,
                  tAttr& attributes,
                  const std::string& fn) const
{
  for (size_t i(0); i < nAttr; ++i) {
    Attribute attr(ncid, id, i, fn);
    if (attr.name.empty()) continue; // _FillValue ??
    tAttr::iterator it(attributes.find(attr.name));
    if (it == attributes.end()) { // This Attribute does not already exist
      attributes.insert(std::make_pair(attr.name, attr));
    } else if (it->second.value != attr.value) {
      Data& data(it->second.value);
      if (attr.value.xtype == data.xtype) { // append
        if (data.xtype == NC_CHAR) { // split with new lines
          data.cVal.push_back('\n');
        }
        data += attr.value;
      } else {
       std::cerr << "Attribute type mismatch" << std::endl;
       std::cerr << " attr " << attr << std::endl;
       std::cerr << "oattr  " << it->second << std::endl;
     }
   }
 }
}

bool
SGMerge::loadFileHeader(const char *fn)
{
  int ncid;
  return ncOp(nc_open(fn, NC_NOWRITE, &ncid), "opening", fn, -1) ? 
         false :
         loadHeader(ncid, fn);
}

bool
SGMerge::loadHeader(const int ncid,
                    const std::string& fn)
{
  { // global attributes
    int nAttr;
    if (ncOp(nc_inq_natts(ncid, &nAttr), "inq_natts", fn, ncid)) return false;
    loadAttr(ncid, NC_GLOBAL, nAttr, mGlobalAttr, fn);
  }

  int ndims, nvars, natts, nunlim;
   
  if (ncOp(nc_inq(ncid, &ndims, &nvars, &natts, &nunlim), "inq", fn, ncid)) return false;

  tDims dims;

  for (size_t id(0); id < ndims; ++id) {
    char name[NC_MAX_NAME+1];
    size_t len;
    if (ncOp(nc_inq_dim(ncid, id, name, &len), "inq_dim", fn, ncid)) return false;
    dims.push_back(Dimension(name, len));
    if (strncmp(name, "string_", 7)) { // Doesn't begin with string_
      tDimMap::iterator it(mDims.find(name));
      if (it == mDims.end()) {
        mDims.insert(std::make_pair(name, len));
        mDimNew.insert(name);
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
    if (ncOp(nc_inq_var(ncid, id, name, &xtype, &nvDim, dimids, &nvAtt), "inq_var", fn, ncid)) 
      return false;

    tVars::iterator it(mVars.find(name));

    if (it == mVars.end()) { // Not known so populate
      struct Variable var(name, xtype);
      loadAttr(ncid, id, nvAtt, var.mAttr, fn);
      size_t totLen(nvDim ? 0 : 1);
      for (size_t j(0); j < nvDim; ++j) { // Walk through this variable's dimensions
        const Dimension& dim(dims[dimids[j]]);
        const std::string& dname(dim.name);
        totLen += dim.len;
        if (dname.substr(0, 7) != "string_") {
          var.mDims.push_back(dname);
        }
      }
      if (!var.check(ncid, id, totLen, fn)) return false; // Check if conversion needed

      if (var.mDims.empty()) { // No dims so fix that up
        if (var.xtype == NC_CHAR) { // No conversion, so 
          char buffer[NC_MAX_NAME + 5];
          snprintf(buffer, sizeof(buffer), "str_%s", name);
          mDims.insert(std::make_pair(buffer, totLen)); 
          var.mDims.push_back(buffer);
          mDimNew.insert(buffer);
        } else { // Some conversion
          var.mDims.push_back("index");
          if (var.nFields > 1) { // Need a second dimension
            char dimName[1024];
            snprintf(dimName, sizeof(dimName), "j_%lu", var.nFields);
            var.mDims.push_back(dimName);
            if (mDims.find(dimName) == mDims.end()) { // Add in this dim
              mDims.insert(std::make_pair(dimName, var.nFields));
              mDimNew.insert(dimName);
            }
          }
        }
      }
      it = mVars.insert(std::make_pair(name, var)).first;
      mVarNew.insert(name);
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

  if (ncOp(nc_close(ncid), "closing", fn, -1)) return false;

  ++mDims["index"];
  return true; 
}

void
SGMerge::updateHeader()
{
  int ncid;
  if (ncOp(nc_create(mFilename.c_str(), NC_CLOBBER | NC_NETCDF4, &ncid), "creating", mFilename, -1))
    return;

  for (tAttr::const_iterator it(mGlobalAttr.begin()), et(mGlobalAttr.end()); it != et; ++it) {
    const Attribute& attr(it->second);
    if (attr.value.putAttr(ncid, NC_GLOBAL, attr.name))
      return;
  }

  for (tDimMap::const_iterator it(mDims.begin()), et(mDims.end()); it != et; ++it) {
    const std::string& name(it->first);
    const bool qNew(mDimNew.find(name) != mDimNew.end());
    const bool qJdim(name.substr(0,2) == "j_");
    int id;
    size_t offset(0);

std::cout << name << " " << qNew << " " << qJdim << std::endl;

    if (qNew) {
      if (ncOp(nc_def_dim(ncid, name.c_str(), it->second, &id), "def_dim", mFilename, ncid)) return;
    } else { // !qNew
      if (ncOp(nc_inq_dimid(ncid, name.c_str(), &id), "inq_dimid for " + name, mFilename, ncid)) 
        return;
      if (!qJdim && ncOp(nc_inq_dimlen(ncid, id, &offset), "inq_dimlen", mFilename, ncid)) 
        return;
    } // qNew

    mDimIDs.insert(std::make_pair(it->first, id));
    mDimCnt.insert(std::make_pair(it->first, offset));

    if (qNew && !qJdim && (name.substr(0,4) != "str_")) {
      Variable var("Dive_" + name, NC_INT);
      var.insertAttr("qDive", true);
      var.mDims.push_back(name);
      mVars.insert(std::make_pair(var.name, var));
      mVarNew.insert(var.name);
      mDiveVars.insert(std::make_pair(var.name, name));
    }
  }

  for (tVars::const_iterator it(mVars.begin()), et(mVars.end()); it != et; ++it) {
    const Variable& var(it->second);
    const bool qNew(mVarNew.find(it->first) != mVarNew.end());
    const size_t n(var.mDims.size());
    int dimids[n];
    for (size_t i(0); i < n; ++i) {
      dimids[i] = mDimIDs[var.mDims[i]];
    }
    int id;
    if (qNew) {
      if (ncOp(nc_def_var(ncid, var.name.c_str(), var.xtype, n, dimids, &id), 
               "def_var", mFilename, ncid) ||
          ncOp(nc_def_var_deflate(ncid, id, NC_SHUFFLE, 1, 3), 
               "def_var_deflate", mFilename, ncid) ||
          Data::setFill(ncid, id, var.xtype, mFilename))
        return;
      for (tAttr::const_iterator it(var.mAttr.begin()), et(var.mAttr.end()); it != et; ++it) {
        const Attribute& attr(it->second);
        if (attr.value.putAttr(ncid, id, attr.name)) return;
      }
    } else { // !qNew
      if (ncOp(nc_inq_varid(ncid, var.name.c_str(), &id), "inq_varid", mFilename, ncid)) return;
    } // qNew

    mVarIDs.insert(std::make_pair(var.name, id));
  }

  if (ncOp(nc_enddef(ncid), "enddef", mFilename, ncid)) return;
  mNCID = ncid;
}

bool
SGMerge::mergeFile(const char *fn)
{
  int ncid;
  if (ncOp(nc_open(fn, NC_NOWRITE, &ncid), "opening", fn, -1)) return false;

  int ndims, nvars, natts, nunlim;
   
  if (ncOp(nc_inq(ncid, &ndims, &nvars, &natts, &nunlim), "inq", fn, ncid)) return false;
  
  tDims dims;
  tDimMap nextCnt(mDimCnt);

  for (size_t id(0); id < ndims; ++id) {
    char name[NC_MAX_NAME+1];
    size_t len;
    if (ncOp(nc_inq_dim(ncid, id, name, &len), "inq_dim", fn, ncid)) return false;
    dims.push_back(Dimension(name, len));
  }

  for (size_t id(0); id < nvars; ++id) {
    char name[NC_MAX_NAME+1];
    nc_type xtype;
    int nvDim;
    int dimids[NC_MAX_VAR_DIMS];
    int nvAtt;
    if (ncOp(nc_inq_var(ncid, id, name, &xtype, &nvDim, dimids, &nvAtt), "inq_var", fn, ncid)) 
      return false;
    size_t len(nvDim ? 0 : 1);
    for (size_t i(0); i < nvDim; ++i) {
      len += dims[dimids[i]].len;
    }
    Data data(xtype, len);
    if (data.getVar(ncid, id, fn)) return false; // Get the actual data

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

    if (data.putVar(mNCID, mVarIDs[oVar.name], start, cnt, fn)) return false;
  }

  if (!mDiveVars.empty()) { // Write out id info 
    int diveNum(0);
    if (ncOp(nc_get_att_int(ncid, NC_GLOBAL, "dive_number", &diveNum), "get_att_int", fn, ncid))
      return false;

    for (tDiveVars::const_iterator it(mDiveVars.begin()), et(mDiveVars.end()); it != et; ++it) {
      const std::string& vName(it->first);
      const std::string& dName(it->second);
      const size_t start(mDimCnt[dName]);
      const size_t cnt(nextCnt[dName] - start);
      Data data(NC_INT);
      data.iVal.resize(cnt, diveNum);
      if (data.putVar(mNCID, mVarIDs[vName], &start, &cnt, fn)) return false;
    }
  }
 
  if (ncOp(nc_close(ncid), "closing", fn, -1)) return false;

  mDimCnt = nextCnt; // Advance offsets
  return true;
}

SGMerge::Attribute::Attribute(const int ncid,
                              const size_t vid,
                              const size_t id,
                              const std::string& fn)
{
  char aname[NC_MAX_NAME+1];
  if (ncOp(nc_inq_attname(ncid, vid, id, aname), "inq_attname", fn, -1)) return;

  size_t len;
  nc_type xtype;

  if (ncOp(nc_inq_att(ncid, vid, aname, &xtype, &len), "inq_att", fn, -1)) return;

  name = aname;

  value = Data(xtype, len);
  value.getAttr(ncid, vid, name);
}

std::ostream&
operator << (std::ostream& os,
             const SGMerge::Attribute& attr)
{
  os << attr.name  << " " << attr.value;
  return os;
}

bool
SGMerge::Variable::check(const int ncid,
                         const size_t id,
                         const size_t len,
                         const std::string& fn)
{
  if (xtype != NC_CHAR) return true; // No conversion needed
  Data data(xtype, len);
  if (data.getVar(ncid, id, name)) return false;

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

void
SGMerge::Variable::insertAttr(const std::string& name,
                              const bool value)
{
  Data datum(NC_BYTE);
  datum.push_back(value);
  mAttr.insert(std::make_pair(name, Attribute(name, datum)));
}

std::ostream&
operator << (std::ostream& os,
             const SGMerge::Variable& v)
{
  os << v.name 
     << ", " << v.xtype
     << ", " << v.qChars
     << ", " << v.qArray
     << ", " << v.nSkip
     << ", " << v.nFields
     << ", " << v.mDims.size()
     ;
  return os;
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
SGMerge::Data::push_back(const bool val)
{
  switch (xtype) {
    case NC_BYTE:
    case NC_CHAR:   cVal.push_back(val); break;
    case NC_INT:    iVal.push_back(val); break;
    case NC_FLOAT:  fVal.push_back(val); break;
    case NC_DOUBLE: dVal.push_back(val); break;
    default: 
      std::cerr << "unsupported data type " << xtype << std::endl;
      exit(1);
  }
  return *this;
}

struct SGMerge::Data&
SGMerge::Data::operator += (const Data& rhs)
{
  if (xtype != rhs.xtype) {
    std::cerr << "Data type mismatch " << xtype << " != " << rhs.xtype << std::endl;
    return *this;
  }
  switch (xtype) {
    case NC_BYTE:
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
    case NC_BYTE:
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
    case NC_BYTE:
    case NC_CHAR: cVal.resize(len); break;
    case NC_INT: iVal.resize(len); break;
    case NC_FLOAT: fVal.resize(len); break;
    case NC_DOUBLE: dVal.resize(len); break;
    default: 
      std::cerr << "unsupported data type " << xtype << std::endl;
      exit(1);
  }
}

bool
SGMerge::Data::getAttr(const int ncid,
                       const int vid,
                       const std::string& name)
{
  int status(NC_EBADTYPE);

  switch (xtype) {
    case NC_BYTE: 
      status = nc_get_att_schar(ncid, vid, name.c_str(), (signed char *) cVal.data());
      break;
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

  return ncOp(status, "get_att", name, -1);
}

bool
SGMerge::Data::putAttr(const int ncid,
                       const int vid,
                       const std::string& name) const
{
  int status(NC_EBADTYPE);

  switch (xtype) {
    case NC_BYTE: 
      status = nc_put_att_schar(ncid, vid, name.c_str(), xtype, cVal.size(), (signed char *) cVal.data());
      break;
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

  return ncOp(status, "put_att", name, ncid);
}

bool
SGMerge::Data::getVar(const int ncid,
                      const int vid,
                      const std::string& fn)
{
  int status(NC_EBADTYPE);

  switch (xtype) {
    case NC_BYTE: 
      status = nc_get_var_schar(ncid, vid, (signed char *) cVal.data());
      break;
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
  
  return ncOp(status, "get_var", fn, ncid);
}

bool
SGMerge::Data::putVar(const int ncid,
                      const int vid,
                      const size_t start[],
                      const size_t cnt[],
                      const std::string& fn) const
{
  int status(NC_EBADTYPE);

  switch (xtype) {
    case NC_BYTE: 
      status =  nc_put_vara_schar(ncid, vid, start, cnt, (signed char *) cVal.data());
      break;
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
  
  return ncOp(status, "put_var", fn, ncid);
}

bool
SGMerge::Data::setFill(const int ncid,
                       const int id,
                       const nc_type xtype,
                       const std::string& fn)
{
  int status(NC_EBADTYPE);

  switch (xtype) {
    case NC_BYTE: 
      {
        char fv(SCHAR_MIN); 
        status =  nc_def_var_fill(ncid, id, 0, (void *) &fv);
      }
      break;
    case NC_CHAR: 
      {
        char fv(0); 
        status =  nc_def_var_fill(ncid, id, 0, (void *) &fv);
      }
      break;
    case NC_INT: 
      {
        int fv(INT_MIN); 
        status =  nc_def_var_fill(ncid, id, 0, (void *) &fv);
      }
      break;
    case NC_FLOAT: 
      {
        float fv(nanf(""));
        status =  nc_def_var_fill(ncid, id, 0, (void *) &fv);
      }
      break;
    case NC_DOUBLE: 
      {
        double fv(nan(""));
        status =  nc_def_var_fill(ncid, id, 0, (void *) &fv);
      }
      break;
  }
  
  return ncOp(status, "def_var_fill", fn, ncid);
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

std::ostream&
operator << (std::ostream& os,
             const SGMerge::Data& d)
{
  os << d.xtype << " ";
  switch (d.xtype) {
    case NC_CHAR: 
      os << d.cVal.size() << " '" 
         << std::string(d.cVal.data(), d.cVal.size())
         << "'"; 
      return os;
    case NC_BYTE: 
      os << d.cVal.size() << " {";
      for (size_t i(0), e(d.cVal.size()); i < e; ++i) 
        os << ((i == 0) ? "" : ",") << (signed char) d.cVal[i];
      os << "}";
      return os;
    case NC_INT: 
      os << d.iVal.size() << " {";
      for (size_t i(0), e(d.iVal.size()); i < e; ++i) 
        os << ((i == 0) ? "" : ",") << d.iVal[i];
      os << "}";
      return os;
    case NC_FLOAT: 
      os << d.fVal.size() << " {";
      for (size_t i(0), e(d.fVal.size()); i < e; ++i) 
        os << ((i == 0) ? "" : ",") << d.fVal[i];
      os << "}";
      return os;
    case NC_DOUBLE: 
      os << d.dVal.size() << " {";
      for (size_t i(0), e(d.dVal.size()); i < e; ++i) 
        os << ((i == 0) ? "" : ",") << d.dVal[i];
      os << "}";
      return os;
  }
  os << " Unsupported data type";
  return os;
}
