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
#include <CLI/CLI.hpp>

int
main(int argc,
     char **argv)
{
  std::string sensorCacheDirectory;
  std::string outputFilename;
  std::vector<std::string> missionsToSkipVec;
  std::vector<std::string> missionsToKeepVec;
  std::vector<std::string> inputFiles;

  CLI::App app{"Extract sensor information from Dinkum Binary Data files", "dbdSensors"};
  app.footer(std::string("\nReport bugs to ") + MAINTAINER);

  app.add_option("-C,--cache", sensorCacheDirectory, "Directory to cache sensor list in");
  app.add_option("-m,--skipMission", missionsToSkipVec, "Mission to skip (can be repeated)");
  app.add_option("-M,--keepMission", missionsToKeepVec, "Mission to keep (can be repeated)");
  app.add_option("-o,--output", outputFilename, "Where to store the data");
  app.add_option("files", inputFiles, "Input DBD files")->required()->check(CLI::ExistingFile);
  app.set_version_flag("-V,--version", VERSION);

  CLI11_PARSE(app, argc, argv);

  // Convert mission vectors to the expected format
  Header::tMissions missionsToSkip;
  Header::tMissions missionsToKeep;
  for (const auto& m : missionsToSkipVec) {
    Header::addMission(m.c_str(), missionsToSkip);
  }
  for (const auto& m : missionsToKeepVec) {
    Header::addMission(m.c_str(), missionsToKeep);
  }

  // Set up output stream
  std::unique_ptr<std::ofstream> outputFile;
  std::ostream *osp(&std::cout);
  if (!outputFilename.empty()) {
    outputFile = std::make_unique<std::ofstream>(outputFilename);
    if (!outputFile || !(*outputFile)) {
      std::cerr << "Error opening '" << outputFilename << "', " << strerror(errno) << std::endl;
      return(1);
    }
    osp = outputFile.get();
  }

  SensorsMap smap(sensorCacheDirectory);

  // Go through and grab all the known sensors

  typedef std::vector<size_t> tFileIndices;
  tFileIndices fileIndices;

  for (size_t i = 0; i < inputFiles.size(); ++i) {
    const char* fn = inputFiles[i].c_str();
    DecompressTWR is(fn, qCompressed(fn));
    if (!is) {
      std::cerr << "Error opening '" << fn << "', " << strerror(errno) << std::endl;
      return(1);
    }
    try {
      Header hdr(is, fn);
      if (hdr.empty()) {
        std::cerr << "Warning '" << fn << "' is empty" << std::endl;
      } else if (hdr.qProcessMission(missionsToSkip, missionsToKeep)) {
        smap.insert(is, hdr, false);
        fileIndices.push_back(i);
      }
    } catch (MyException& e) {
      std::cerr << "Error working on '" << fn << "'" << std::endl;
      std::cerr << e.what() << std::endl;
    }
  }

  if (fileIndices.empty()) {
    std::cerr << "No input files found to process!" << std::endl;
    return(1);
  }

  for (tFileIndices::size_type ii(0), ie(fileIndices.size()); ii < ie; ++ii) {
    const size_t i(fileIndices[ii]);
    const char* fn = inputFiles[i].c_str();
    DecompressTWR is(fn, qCompressed(fn));
    if (!is) {
      std::cerr << "Error opening '" << fn << "', " << strerror(errno) << std::endl;
      return(1);
    }
    try {
      Header hdr(is, fn);
      const Sensors& sensors(smap.find(hdr));
      if (sensors.empty()) {
        std::cerr << "No sensors list found for '" << fn << "'" << std::endl;
        return(1);
      }
    } catch (MyException &e) {
      std::cerr << "Error working on '" << fn << "'" << std::endl;
      std::cerr << e.what() << std::endl;
    }
  }

  smap.setUpForData();

  *osp << smap.allSensors();

  // outputFile automatically cleaned up on function exit

  return(0);
}
