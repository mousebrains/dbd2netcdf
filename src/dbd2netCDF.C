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

// Read in a set of Dinkum Binary Data files, and
// output them into a netCDF file

#include <Header.H>
#include <SensorsMap.H>
#include <KnownBytes.H>
#include <Data.H>
#include <MyException.H>
#include <config.h>
#include <netcdf.h>
#include <set>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <cmath>

namespace {
  void ncBasicOp(int retval, const std::string& label, const std::string& fn) {
    if (retval) {
      std::cerr << "Error " << label << " '" << fn << "', " << nc_strerror(retval)
                << std::endl;
      exit(2);
    }
  }

  int ncOpenFile(const std::string& fn) {
    int ncid;
    ncBasicOp(nc_create(fn.c_str(), NC_NETCDF4 | NC_CLOBBER, &ncid),
              "opening", fn);
    return ncid;
  }

  int ncCreateDim(int ncid, const std::string& name, const std::string& fn) {
    int dimId;
    const int retval(nc_def_dim(ncid, name.c_str(), NC_UNLIMITED, &dimId));
    if (retval) {
      std::cerr << "Error creating dimension '" << name << "' in '" << fn << "', "
                << nc_strerror(retval) << std::endl;
      exit(2);
    }
    return dimId;
  }

  int ncCreateVar(int ncid, const std::string& name, 
                nc_type idType, int idDim, 
                const std::string& units, const std::string& fn) {
    int varId;
    int retval(nc_def_var(ncid, name.c_str(), idType, 1, &idDim, &varId));
    if (retval) {
      std::cerr << "Error creating variable '" << name << "' in '" << fn << "', "
                << nc_strerror(retval) << std::endl;
      exit(2);
    }

    const size_t chunkSize(100000);

    if ((retval = nc_def_var_chunking(ncid, varId, NC_CHUNKED, &chunkSize))) {
      std::cerr << "Error setting chunk size to " << chunkSize
                << " for '" << name << "' in '" << fn << "', "
                << nc_strerror(retval) << std::endl;
      exit(2);
    }

    const int qShuffle((idType != NC_FLOAT) && (idType != NC_DOUBLE));
    const int compressionLevel(9);
    if ((retval = nc_def_var_deflate(ncid, varId, qShuffle, 1, compressionLevel))) {
      std::cerr << "Error enabling compression and shuffle(" << qShuffle
                << ") for '" << name << "' in '" << fn << "', "
                << nc_strerror(retval) << std::endl;
      exit(2);
    }

    if (idType == NC_FLOAT) {
      const float badValue(nan(""));
      if ((retval = nc_def_var_fill(ncid, varId, NC_FILL, &badValue))) {
        std::cerr << "Error setting fill value, " << badValue 
                  << " for '" << name << "' in '" << fn 
                  << "', " << nc_strerror(retval) << std::endl;
        exit(2);
      }
    } else if (idType == NC_DOUBLE) {
      const double badValue(nan(""));
      if ((retval = nc_def_var_fill(ncid, varId, NC_FILL, &badValue))) {
        std::cerr << "Error setting fill value, " << badValue 
                  << " for '" << name << "' in '" << fn 
                  << "', " << nc_strerror(retval) << std::endl;
        exit(2);
      }
    }

    if (!units.empty()) {
      if ((retval = nc_put_att_text(ncid, varId, "units", 
                                    units.size(), units.c_str()))) {
        std::cerr << "Error setting units attribute, '" << units << "' for '"
                  << name << "' in '" << fn << "', " << nc_strerror(retval)
                  << std::endl;
        exit(2);
      }
    }

    return varId;
  }

  void ncPutVars(int ncid, int varId, size_t start, size_t count, 
                 const double data[], const std::string& fn) {
    const int retval(nc_put_vara_double(ncid, varId, &start, &count, data));
    if (retval) {
      char varName[NC_MAX_NAME + 1];
      ncBasicOp(nc_inq_varname(ncid, varId, varName), "getting variable name", fn);
      std::cerr << "Error writing data to '" << varName << "' in '" << fn
                << "', " << nc_strerror(retval) << std::endl;
      exit(2);
    }
  }

  void ncPutVar(int ncid, int varId, size_t start, 
                unsigned int value, const std::string& fn) {
    const size_t count(1);
    const int retval(nc_put_vara_uint(ncid, varId, &start, &count, &value));
    if (retval) {
      char varName[NC_MAX_NAME + 1];
      ncBasicOp(nc_inq_varname(ncid, varId, varName), "getting variable name", fn);
      std::cerr << "Error writing data to '" << varName << "' in '" << fn
                << "', " << nc_strerror(retval) << std::endl;
      exit(2);
    }
  }

  void ncPutVar(int ncid, int varId, size_t start, const std::string& str,
                const std::string& fn) {
    const char *cstr(str.c_str());
    const int retval(nc_put_var1_string(ncid, varId, &start, &cstr));
    if (retval) {
      char varName[NC_MAX_NAME + 1];
      ncBasicOp(nc_inq_varname(ncid, varId, varName), "getting variable name", fn);
      std::cerr << "Error writing a string, '" << str 
                << "', to '" << varName << "' in '" << fn
                << "', " << nc_strerror(retval) << std::endl;
      exit(2);
    }
  }

  int usage(const char *argv0, const char *options) {
    std::cerr << argv0 << " Version " << VERSION << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage: " << argv0 << " -[" << options << "] files" << std::endl;
    std::cerr << std::endl;
    std::cerr << " -c filename  file containing sensors to select on" << std::endl;
    std::cerr << " -C directory directory to cache sensor list in" << std::endl;
    std::cerr << " -h           display the usage message" << std::endl;
    std::cerr << " -k filename  file containing sensors to output" << std::endl;
    std::cerr << " -m mission   mission to skip, this can be repeated" << std::endl;
    std::cerr << " -M mission   mission to keep, this can be repeated" << std::endl;
    std::cerr << " -o filename  where to store the data" << std::endl;
    std::cerr << " -s           Skip the first data record in each file, except the first" << std::endl;
    std::cerr << " -V           Print out version" << std::endl;
    std::cerr << " -v           Enable some diagnostic output" << std::endl;
    std::cerr << "\nReport bugs to " << MAINTAINER << std::endl;
    return 1;
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  const char *options("c:C:hk:m:M:o:sVv"); 

  std::string sensorCacheDirectory;
  Sensors::tNames toKeep, criteria;
  const char *ofn(0);
  bool qSkipFirstRecord(false);
  bool qVerbose(false);

  Header::tMissions missionsToSkip;
  Header::tMissions missionsToKeep;
 
  for (int ch; (ch = getopt(argc, argv, options)) != -1;) { // Process options
    switch (ch) {
      case 'k': // Sensors to keep
        Sensors::loadNames(optarg, toKeep);
        break;
      case 'c': // Sensors to select on
        Sensors::loadNames(optarg, criteria);
        break;
      case 'C': // directory to cache sensor lists in
        sensorCacheDirectory = optarg;
        break;
      case 'm': // Missions to skip
        Header::addMission(optarg, missionsToSkip);
        break; 
      case 'M': // Missions to keep
        Header::addMission(optarg, missionsToKeep);
        break; 
      case 'o': // Output filename
        ofn = optarg;
        break;
      case 's': // Skip first data record in the DBD file
        qSkipFirstRecord = true;
        break;
      case 'V': // Print out version string
        std::cerr << VERSION << std::endl;
        return(0);
      case 'v': // Verbose
        qVerbose = !qVerbose;
        break;
      default:
        std::cerr << "Unrecognized option '" << ((char) ch) << "'" << std::endl;
      case 'h': // Sensors to select on
        return usage(argv[0], options);
    }
  }

  if (!ofn) {
    std::cerr << "ERROR: No output filename specified" << std::endl;
    return usage(argv[0], options);
  }

  if (optind >= argc) {
    std::cerr << "No input files specified!" << std::endl;
    usage(argv[0], options);
    exit(1);
  }

  SensorsMap smap(sensorCacheDirectory);

  // Go through and grab all the known sensors

  typedef std::vector<int> tFileIndices;
  tFileIndices fileIndices;

  for (int i = optind; i < argc; ++i) {
    std::ifstream is(argv[i]);
    if (!is) {
      std::cerr << "Error opening '" << argv[i] << "', " << strerror(errno) << std::endl;
      return(1);
    }
    const Header hdr(is);
    if (!hdr.empty() && hdr.qProcessMission(missionsToSkip, missionsToKeep)) {
      smap.insert(is, hdr, false);
      fileIndices.push_back(i);
    }
  }

  if (fileIndices.empty()) {
    std::cerr << "No input files found to process!" << std::endl;
    usage(argv[0], options);
    exit(1);
  }

  smap.qKeep(toKeep);
  smap.qCriteria(criteria);
  smap.setUpForData(); // Get a common list of sensors

  typedef std::vector<int> tVars;
  tVars vars(smap.allSensors().nToStore());

  const int ncid(ncOpenFile(ofn));
  const int iDim(ncCreateDim(ncid, "i", ofn));
  const int jDim(ncCreateDim(ncid, "j", ofn));

  { // Setup variables
    const Sensors& all(smap.allSensors());

    for (Sensors::const_iterator it(all.begin()), et(all.end()); it != et; ++it) {
      const Sensor& sensor(*it);
      if (sensor.qKeep()) {
        const size_t index(sensor.index());
        const std::string& name(sensor.name());
        const std::string& units(sensor.units());
        switch (sensor.size()) {
        case 1: 
          vars[index] = ncCreateVar(ncid, name, NC_BYTE, iDim, units, ofn);
          break;
        case 2: 
          vars[index] = ncCreateVar(ncid, name, NC_SHORT, iDim, units, ofn);
          break;
        case 4: 
          vars[index] = ncCreateVar(ncid, name, NC_FLOAT, iDim, units, ofn);
          break;
        case 8: 
          vars[index] = ncCreateVar(ncid, name, NC_DOUBLE, iDim, units, ofn);
          break;
        default:
          std::cerr << "Unsupported sensor size " << sensor << std::endl;
          exit(1);
        }
      }
    }
  }

  typedef std::vector<std::string> tHdrNames;
  tHdrNames hdrNames;
  hdrNames.push_back("full_filename");
  hdrNames.push_back("encoding_ver");
  hdrNames.push_back("the8x3_filename");
  hdrNames.push_back("filename_extension");
  hdrNames.push_back("mission_name");
  hdrNames.push_back("fileopen_time");
  hdrNames.push_back("sensor_list_crc");

  tVars hdrVars(hdrNames.size());

  for (tVars::size_type i(0), e(hdrVars.size()); i < e; ++i) {
    const std::string& name("hdr_" + hdrNames[i]);
    hdrVars[i] = ncCreateVar(ncid, name, NC_STRING, jDim, std::string(), ofn);
  }

  const int hdrStartIndex(ncCreateVar(ncid, "hdr_start_index", NC_UINT, jDim,
                                      std::string(), ofn));
  const int hdrStopIndex(ncCreateVar(ncid, "hdr_stop_index", NC_UINT, jDim,
                                      std::string(), ofn));
  const int hdrLength(ncCreateVar(ncid, "hdr_nRecords", NC_UINT, jDim, 
                                  std::string(), ofn));

  ncBasicOp(nc_enddef(ncid), "ending definitions for", ofn);

  // Go through and grab all the data

  unsigned int indexOffset(0);
  const size_t k0(qSkipFirstRecord ? 1 : 0);

  for (tFileIndices::size_type ii(0), iie(fileIndices.size()); ii < iie; ++ii) {
    const int i(fileIndices[ii]);
    std::ifstream is(argv[i]);
    if (!is) {
      std::cerr << "Error opening '" << argv[i] << "', " 
                << strerror(errno) << std::endl;
      return(1);
    }
    const Header hdr(is);             // Load up header
    try {
      smap.insert(is, hdr, true);       // will move to the right position in the file
      const Sensors& sensors(smap.find(hdr));
      const KnownBytes kb(is);          // Get little/big endian
      Data data;

      try {
        data.load(is, kb, sensors);
      } catch (MyException& e) {
        std::cerr << "Error processing '" << argv[i] << "', " << e.what() 
                  << ", retaining " << data.size() << " records" << std::endl;
      }

      if (data.empty()) {
        continue;
      }

      const size_t n(data.size());
      const size_t kStart(ii == 0 ? 0 : k0);

      double *values(new double[n * sizeof(double)]);

      { // Update file info
        for (tVars::size_type i(0), e(hdrVars.size()); i < e; ++i) {
          const std::string str(hdr.find(hdrNames[i]));
          ncPutVar(ncid, hdrVars[i], (size_t) ii, str, ofn);
        }
        if (n > kStart) {
          const unsigned int stopIndex(indexOffset + n - kStart - 1);

          ncPutVar(ncid, hdrStartIndex, ii, indexOffset, ofn);
          ncPutVar(ncid, hdrStopIndex, ii, stopIndex, ofn);
        }
        ncPutVar(ncid, hdrLength, ii, (unsigned int) n, ofn);
      }
      
      if (n <= kStart) { // No data to be written
        continue;
      }

      for (tVars::size_type j(0), je(vars.size()); j < je; ++j) {
        const int var(vars[j]);
        size_t iFirst(0);
        bool qLooking(true);
        for (size_t k(kStart); k < n; ++k) {
          const double value(data[k][j]);
          const bool qSkip(std::isnan(value) || std::isinf(value));
          if (!qSkip) {
            values[k] = value;
            if (qLooking) {
              qLooking = false;
              iFirst = k;
            }
          } else { // isnan
            if (!qLooking) { // We have some data to write out
              qLooking = true;
              const size_t start(indexOffset + iFirst);
              const size_t count(k - iFirst);
              ncPutVars(ncid, var, start, count, &values[iFirst], ofn);
            }
          }
        }

        if (!qLooking) {
          const size_t start(indexOffset + iFirst);
          const size_t count(n - iFirst);
          ncPutVars(ncid, var, start, count, &values[iFirst], ofn);
        }
      }
      
      delete values;

      indexOffset += data.size() - kStart;

      if (qVerbose) {
        std::cerr << argv[i] << " " << data.size() - kStart << std::endl;
      }
    } catch (MyException& e) { // Catch my exceptions, where I toss the whole file
      std::cerr << "Error processing '" << argv[i] << "', " << e.what() << std::endl;
    }
  }
 
  ncBasicOp(nc_close(ncid), "closing", ofn);

  return(0);
}
