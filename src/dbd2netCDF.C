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
#include <netcdf>
#include <set>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <cmath>

using namespace netCDF;
using namespace netCDF::exceptions;

namespace {
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

  const double badDouble (nan (""));
  const float badFloat(badDouble);
  int rc(0);

  try {
    typedef std::vector<NcVar> tVars;
    typedef std::vector<size_t> tSize;

    NcFile df(ofn, NcFile::replace, NcFile::nc4);
    NcDim dim(df.addDim("i")); // Unlimited netCDF dimension for data
    tVars vars(smap.allSensors().nToStore());

    { // Setup variables
      tSize chunkSize(1, 100000);
      const int compressionLevel(9);

      const Sensors& all(smap.allSensors());

      for (Sensors::const_iterator it(all.begin()), et(all.end()); it != et; ++it) {
        const Sensor& sensor(*it);
        if (sensor.qKeep()) {
          const size_t index(sensor.index());
          const std::string& name(sensor.name());
          const std::string& units(sensor.units());
          switch (sensor.size()) {
            case 1: 
              vars[index] = NcVar(df.addVar(name, ncByte, dim)); 
              break;
            case 2: 
              vars[index] = NcVar(df.addVar(name, ncShort, dim)); 
              break;
            case 4: 
              vars[index] = NcVar(df.addVar(name, ncFloat, dim)); 
              vars[index].setFill(true, &badFloat);
              break;
            case 8: 
              vars[index] = NcVar(df.addVar(name, ncDouble, dim)); 
              vars[index].setFill(true, &badDouble);
              break;
            default:
              std::cerr << "Unsupported sensor size " << sensor << std::endl;
              exit(1);
          }
          NcVar& var(vars[index]);
          var.setChunking(NcVar::nc_CHUNKED, chunkSize);
          var.setCompression(true, true, compressionLevel);
          var.putAtt("units", units);
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
    NcDim dimj(df.addDim("j")); // Unlimited netCDF dimension for file header
    for (tVars::size_type i(0), e(hdrVars.size()); i < e; ++i) {
      const std::string& name("hdr_" + hdrNames[i]);
      hdrVars[i] = NcVar(df.addVar(name, ncString, dimj));
    }

    NcVar hdrStartIndex(df.addVar("hdr_start_index", ncUint, dimj));
    NcVar hdrStopIndex(df.addVar("hdr_stop_index", ncUint, dimj));
    NcVar hdrLength(df.addVar("hdr_nRecords", ncUint, dimj));

    // Go through and grab all the data

    size_t indexOffset(0);
    const size_t k0(qSkipFirstRecord ? 1 : 0);

    for (tFileIndices::size_type ii(0), iie(fileIndices.size()); ii < iie; ++ii) {
      const int i(fileIndices[ii]);
      std::ifstream is(argv[i]);
      if (!is) {
        std::cerr << "Error opening '" << argv[i] << "', " << strerror(errno) << std::endl;
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
          const tSize start(1, ii);
          const tSize count(1, 1);
          for (tVars::size_type i(0), e(hdrVars.size()); i < e; ++i) {
             const std::string str(hdr.find(hdrNames[i]));
             hdrVars[i].putVar(start, count, &str);
          }
          if (n > kStart) {
            const size_t stopIndex(indexOffset + n - kStart - 1);

            hdrStartIndex.putVar(start, count, &indexOffset);
            hdrStopIndex.putVar(start, count, &stopIndex);
          }
          hdrLength.putVar(start, count, &n);
        }
      
        if (n <= kStart) { // No data to be written
          continue;
        }

        for (tVars::size_type j(0), je(vars.size()); j < je; ++j) {
          NcVar& var(vars[j]);
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
                const tSize start(1, indexOffset + iFirst);
                const tSize count(1, k - iFirst);
                var.putVar(start, count, &values[iFirst]);
              }
            }
          }

          if (!qLooking) {
            const tSize start(1, indexOffset + iFirst);
            const tSize count(1, n - iFirst);
            var.putVar(start, count, &values[iFirst]);
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
  } catch (NcException& e) { // Catch netCDF exceptions
    e.what();
    rc = 2;
  }
  
  return(rc);
}
