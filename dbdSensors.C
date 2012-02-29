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

#include <Header.H>
#include <SensorsMap.H>
#include <KnownBytes.H>
#include <MyException.H>
#include <Version.H>
#include <iostream>
#include <fstream>
#include <cerrno>

namespace {
  void usage(const char *argv0, const char *options) {
    std::cout << argv0 << " Version " << versionString << std::endl;
    std::cout << std::endl;
    std::cerr << "Usage: " << argv0 << " -[" << options << "] files" << std::endl;
    std::cerr << std::endl;
    std::cerr << " -C directory directory to cache sensor list in" << std::endl;
    std::cerr << " -h           display this usage message" << std::endl;
    std::cerr << " -m mission   mission to skip, this can be repeated" << std::endl;
    std::cerr << " -M mission   mission to keep, this can be repeated" << std::endl;
    std::cerr << " -o filename  where to store the data" << std::endl;
    std::cerr << " -V           Print out version" << std::endl;
    std::cerr << "\nReport bugs to " << maintainer << std::endl;
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  const char *options("C:hm:M:o:V");

  std::string sensorCacheDirectory;
  std::ostream *osp(&std::cout);
  bool qUsingStdOut(true);

  Header::tMissions missionsToSkip;
  Header::tMissions missionsToKeep;

  for (int ch; (ch = getopt(argc, argv, options)) != -1;) { // Process options
    switch (ch) {
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
          std::cerr << "Error creating a new ofstream for '" << optarg << "', " << strerror(errno) << std::endl;
          return(1);
        }
        if (!(*osp)) {
          std::cerr << "Error opening '" << optarg << "', " << strerror(errno) << std::endl;
          return(1);
        }
        qUsingStdOut = false;
        break;
      case 'V': // Print out version string
        std::cout << versionString << std::endl;
        return(0);
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
      Header hdr(is);
      if (hdr.qProcessMission(missionsToSkip, missionsToKeep)) {
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
    usage(argv[0], options);
    exit(1);
  }

  for (tFileIndices::size_type ii(0), ie(fileIndices.size()); ii < ie; ++ii) {
    const int i(fileIndices[ii]);
    std::ifstream is(argv[i]);
    if (!is) {
      std::cerr << "Error opening '" << argv[i] << "', " << strerror(errno) << std::endl;
      return(1);
    }
    try {
      Header hdr(is);
      const Sensors& sensors(smap.find(hdr));
      if (sensors.empty()) {
        std::cerr << "No sensors list found for '" << argv[i] << "'" << std::endl;
        exit(1);
      }
    } catch (MyException &e) {
      std::cerr << "Error working on '" << argv[i] << "'" << std::endl;
      std::cerr << e.what() << std::endl;
    }
  }

  smap.setUpForData();
  const Sensors &sensors(smap.allSensors());

  *osp << smap.allSensors();

  if (!qUsingStdOut && osp) {
    delete(osp);
    osp = 0;
  }

  return (0);
}
