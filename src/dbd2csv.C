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
// output them into a CSV file

#include "Header.H"
#include "SensorsMap.H"
#include "KnownBytes.H"
#include "Data.H"
#include "MyException.H"
#include "config.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

namespace {
  void usage(const char *argv0, const char *options) {
    std::cerr << argv0 << " Version " << VERSION << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage: " << argv0 << " -[" << options << "] files" << std::endl;
    std::cerr << std::endl;
    std::cerr << " -c filename  file containing sensors to select on" << std::endl;
    std::cerr << " -C directory directory to cache sensor list in" << std::endl;
    std::cerr << " -h           display this usage message" << std::endl;
    std::cerr << " -k filename  file containing sensors to output" << std::endl;
    std::cerr << " -m mission   mission to skip, this can be repeated" << std::endl;
    std::cerr << " -M mission   mission to keep, this can be repeated" << std::endl;
    std::cerr << " -o filename  where to store the data" << std::endl;
    std::cerr << " -s           Skip the first data record in each file, except the first" << std::endl;
    std::cerr << " -V           Print out version" << std::endl;
    std::cerr << " -v           Enable some diagnostic output" << std::endl;
    std::cerr << "\nReport bugs to " << MAINTAINER << std::endl;
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  const char *options("c:C:hk:m:M:o:sVv"); 

  std::string sensorCacheDirectory;
  Sensors::tNames toKeep, criteria;
  std::ostream *osp(&std::cout);
  bool qUsingStdOut(true);

  bool qSkipFirstRecord(false);
  bool qVerbose(false);

  Header::tMissions missionsToSkip;
  Header::tMissions missionsToKeep;

  for (int ch; (ch = getopt(argc, argv, options)) != -1;) { // Process options
    switch (ch) {
      case 'c': // Sensors to select on
        Sensors::loadNames(optarg, criteria);
        break;
      case 'C': // directory to cache sensor lists in
        sensorCacheDirectory = optarg;
        break;
      case 'k': // Sensors to keep
        Sensors::loadNames(optarg, toKeep);
        break;
      case 'm': // Missions to skip
        Header::addMission(optarg, missionsToSkip);
        break;
      case 'M': // Missions to keep
        Header::addMission(optarg, missionsToKeep);
        break;
      case 'o': // Output filename
        osp = new std::ofstream(optarg);
        if (!osp) {
          std::cerr << "Error creating a new ofstream for '" << optarg << "', " << strerror(errno) << std::endl;
          return(1);
        }
        if (!(*osp)) {
          std::cerr << "Error opening '" << optarg << "', " << strerror(errno) << std::endl;
          return(1);
        }
        qUsingStdOut = false;
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
        usage(argv[0], options);
        exit(1);
    }
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
    try {
      const Header hdr(is);
      if (!hdr.empty() && hdr.qProcessMission(missionsToSkip, missionsToKeep)) {
        smap.insert(is, hdr, false);
        fileIndices.push_back(i);
      }
    } catch (MyException& e) {
      std::cerr << "Error processing '" << argv[i] << "', " << e.what() << std::endl;
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

  const Sensors& all(smap.allSensors());
  std::string delim;

  typedef std::vector<Data::tRow::size_type> tRowsToOutput;
  tRowsToOutput rowsToOutput;

  for (Sensors::size_type i(0), e(all.size()); i < e; ++i) {
    const Sensor& sensor(all[i]);
    if (sensor.qKeep()) {
      rowsToOutput.push_back(i);
      *osp << delim << sensor.name();
    delim = ",";
    }
  }
  *osp << std::endl;

  // Go through and grab all the data

  const size_t k0(qSkipFirstRecord ? 1 : 0);

  for (tFileIndices::size_type ii(0), iie(fileIndices.size()); ii < iie; ++ii) {
    const int i(fileIndices[ii]);
    std::ifstream is(argv[i]);
    if (!is) {
      std::cerr << "Error opening '" << argv[i] << "', " << strerror(errno) << std::endl;
      return(1);
    }
    const Header hdr(is);             // Load up header
    smap.insert(is, hdr, true); // Since will move to the right position in the file
    const KnownBytes kb(is);          // Get little/big endian
    Data data(is, kb, smap.find(hdr));
    data.delim(",");

    const size_t n(data.size());
    const size_t kStart(ii == 0 ? 0 : k0);

    if (n > kStart) { // some data to output
      for (size_t k(kStart); k < n; ++k) {
        const Data::tRow& row(data[k]);
        for (tRowsToOutput::size_type j(0), je(rowsToOutput.size()); j < je; ++j) {
          const size_t index(rowsToOutput[j]);
          if (j != 0) {
            *osp << ',';
          }
          const double dval(row[index]);
          if (!std::isnan(dval)) {
            *osp << all[index].toStr(dval);
          }
        }
        *osp << std::endl;
      }
    }

    if (qVerbose) {
      std::cerr << argv[i] << ' ' << data.size() << std::endl;
    }
  }

  if (!qUsingStdOut && osp) {
    delete(osp);
    osp = 0;
  }

  return (0);
}
