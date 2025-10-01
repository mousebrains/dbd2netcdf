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

#include "Header.H"
#include "SensorsMap.H"
#include "KnownBytes.H"
#include "MyException.H"
#include "Decompress.H"
#include "config.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <getopt.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

namespace {
  const char *options("C:hm:M:o:V");
  const struct option optionsLong[] = {
	  {"cache", required_argument, NULL, 'C'},
          {"skipMission", required_argument, NULL, 'm'},
          {"keepMission", required_argument, NULL, 'M'},
          {"output", required_argument, NULL, 'o'},
          {"version", no_argument, NULL, 'V'},
          {"help", no_argument, NULL, 'h'},
          {NULL, no_argument, NULL, 0}
  };

  int usage(const char *argv0) {
    std::cout << argv0 << " Version " << VERSION << std::endl;
    std::cout << std::endl;
    std::cerr << "Usage: " << argv0 << " -[" << options << "] files" << std::endl;
    std::cerr << std::endl;
    std::cerr << " -C --cache       directory directory to cache sensor list in" << std::endl;
    std::cerr << " -h --help                  display the usage message" << std::endl;
    std::cerr << " -m --skipMission mission   mission to skip, this can be repeated" << std::endl;
    std::cerr << " -M --keepMission mission   mission to keep, this can be repeated" << std::endl;
    std::cerr << " -o --output      filename  where to store the data" << std::endl;
    std::cerr << " -V --version               print out version" << std::endl;
    std::cerr << "\nReport bugs to " << MAINTAINER << std::endl;
    return 1;
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  std::string sensorCacheDirectory;
  std::unique_ptr<std::ofstream> outputFile;  // RAII - automatic cleanup
  std::ostream *osp(&std::cout);

  Header::tMissions missionsToSkip;
  Header::tMissions missionsToKeep;

  while (true) { // Walk through the options
    int optionIndex = 0;
    int c = getopt_long(argc, argv, options, optionsLong, &optionIndex);
    if (c == -1) break; // End of options

    switch (c) {
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
        outputFile = std::make_unique<std::ofstream>(optarg);
        if (!outputFile || !(*outputFile)) {
          std::cerr << "Error opening '" << optarg << "', " << strerror(errno) << std::endl;
          return(1);
        }
        osp = outputFile.get();
        break;
      case 'V': // Print out version string
        std::cerr << VERSION << std::endl;
        return(0);
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
    DecompressTWR is(argv[i], qCompressed(argv[i]));
    if (!is) {
      std::cerr << "Error opening '" << argv[i] << "', " << strerror(errno) << std::endl;
      return(1);
    }
    try {
      Header hdr(is, argv[i]);
      if (hdr.empty()) {
        std::cerr << "Warning '" << argv[i] << "' is empty" << std::endl;
      } else if (hdr.qProcessMission(missionsToSkip, missionsToKeep)) {
        smap.insert(is, hdr, false);
        fileIndices.push_back(i);
      }
    } catch (MyException& e) {
      std::cerr << "Error working on '" << argv[i] << "'" << std::endl;
      std::cerr << e.what() << std::endl;
    }
  }

  if (fileIndices.empty()) {
    std::cerr << "No input files found to process!" << std::endl;
    return usage(argv[0]);
  }

  for (tFileIndices::size_type ii(0), ie(fileIndices.size()); ii < ie; ++ii) {
    const int i(fileIndices[ii]);
    DecompressTWR is(argv[i], qCompressed(argv[i]));
    if (!is) {
      std::cerr << "Error opening '" << argv[i] << "', " << strerror(errno) << std::endl;
      return(1);
    }
    try {
      Header hdr(is, argv[i]);
      const Sensors& sensors(smap.find(hdr));
      if (sensors.empty()) {
        std::cerr << "No sensors list found for '" << argv[i] << "'" << std::endl;
        return(1);
      }
    } catch (MyException &e) {
      std::cerr << "Error working on '" << argv[i] << "'" << std::endl;
      std::cerr << e.what() << std::endl;
    }
  }

  smap.setUpForData();

  *osp << smap.allSensors();

  // outputFile automatically cleaned up on function exit

  return(0);
}
