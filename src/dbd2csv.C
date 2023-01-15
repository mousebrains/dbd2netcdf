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
#include <getopt.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

namespace {
  const char *options("c:C:hk:m:M:o:sVv"); 
    const struct option optionsLong[] = {
          {"sensors", required_argument, NULL, 'c'},
          {"cache", required_argument, NULL, 'C'},
          {"sensorOutput", required_argument, NULL, 'k'},
          {"skipMission", required_argument, NULL, 'm'},
          {"keepMission", required_argument, NULL, 'M'},
          {"output", required_argument, NULL, 'o'},
          {"skipFirst", no_argument, NULL, 's'},
          {"repair", no_argument, NULL, 'r'},
          {"verbose", no_argument, NULL, 'v'},
          {"version", no_argument, NULL, 'V'},
          {"help", no_argument, NULL, 'h'},
          {NULL, no_argument, NULL, 0}
  };

  int usage(const char *argv0) {
    std::cerr << argv0 << " Version " << VERSION << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage: " << argv0 << " -[" << options << "] files" << std::endl;
    std::cerr << std::endl;
    std::cerr << " -c --sensor       filename  file containing sensors to select on" << std::endl;
    std::cerr << " -C --cache        directory directory to cache sensor list in" << std::endl;
    std::cerr << " -h --help                   display the usage message" << std::endl;
    std::cerr << " -k --sensorOutput filename  file containing sensors to output" << std::endl;
    std::cerr << " -m --skipMission  mission   mission to skip, this can be repeated" << std::endl;
    std::cerr << " -M --keepMission  mission   mission to keep, this can be repeated" << std::endl;
    std::cerr << " -o --output       filename  where to store the data" << std::endl;
    std::cerr << " -s --skipFirst              "
            << "Skip first record in each file, but the first file" << std::endl;
    std::cerr << " -r --repair                 attempt to repair bad data records" << std::endl;
    std::cerr << " -V --version                print out version" << std::endl;
    std::cerr << " -v --verbose                enable some diagnostic output" << std::endl;
    std::cerr << "\nReport bugs to " << MAINTAINER << std::endl;
    return 1;
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  std::string sensorCacheDirectory;
  Sensors::tNames toKeep, criteria;
  std::ostream *osp(&std::cout);
  bool qUsingStdOut(true);

  bool qSkipFirstRecord(false);
  bool qRepair(false);
  bool qVerbose(false);

  Header::tMissions missionsToSkip;
  Header::tMissions missionsToKeep;

    while (true) { // Walk through the options
    int thisOptionOptind = optind ? optind : 1;
    int optionIndex = 0;
    int c = getopt_long(argc, argv, options, optionsLong, &optionIndex);
    if (c == -1) break; // End of options

    switch (c) {
      case 'c': // Sensors to select on
        Sensors::loadNames(optarg, criteria);
        break;
      case 'k': // Sensors to keep
        Sensors::loadNames(optarg, toKeep);
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
        osp = new std::ofstream(optarg);
        if (!osp) {
          std::cerr << "Error creating a new ofstream for '" << optarg << "', "
		  << strerror(errno) << std::endl;
	  return(1);
	}

        if (!(*osp)) {
          std::cerr << "Error opening '" << optarg << "', " << strerror(errno) << std::endl;
          return(1);
        }
        qUsingStdOut = false;
        break;
      case 'r': // Attempt to repair DBD files for missing data cycles
        qRepair = true;
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
        std::cerr << "Unsupported option 0x" << std::hex << c << std::dec;
        if ((c >= 0x20) && (c <= 0x7e)) std::cerr << " '" << ((char) (c & 0xff)) << "'";
        std::cerr << std::endl;
      case '?': // Unsupported option
      case 'h': // Help
        return usage(argv[0]);
    }
  }

  if (optind >= argc) {
    std::cerr << "No input files specified!" << std::endl;
    return usage(argv[0]);
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
    return usage(argv[0]);
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
    const Sensors& sensors(smap.find(hdr));
    const KnownBytes kb(is);          // Get little/big endian
    Data data;

    try {
      data.load(is, kb, sensors, qRepair);
    } catch (MyException& e) {
      std::cerr << "Error processing '" << argv[i] << "', " << e.what()
	      << ", retaining " << data.size() << " records" << std::endl;
    }

    if (data.empty()) continue;

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

  return(0);
}
