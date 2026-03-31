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
#include "Logger.H"
#include "Decompress.H"
#include "FileInfo.H"
#include "config.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <algorithm>
#include <numeric>
#include <CLI/CLI.hpp>

int
main(int argc,
     char **argv)
{
  std::string sensorCacheDirectory;
  std::string outputFilename;
  std::string sensorsFile;
  std::string sensorOutputFile;
  std::vector<std::string> missionsToSkipVec;
  std::vector<std::string> missionsToKeepVec;
  std::vector<std::string> inputFiles;
  std::string logLevel = "warn";
  bool qSkipFirstRecord(false);
  bool qSkipAllFirst(false);
  bool qKeepFirst(false);
  bool qRepair(false);
  bool qStrict(false);
  bool qVerbose(false);
  std::string sortOrder = "none";

  CLI::App app{"Convert Dinkum Binary Data files to CSV", "dbd2csv"};
  app.footer(std::string("\nReport bugs to ") + MAINTAINER);

  app.add_option("-c,--sensors", sensorsFile, "File containing sensors to select on");
  app.add_option("-C,--cache", sensorCacheDirectory, "Directory to cache sensor list in");
  app.add_option("-k,--sensorOutput", sensorOutputFile, "File containing sensors to output");
  app.add_option("-m,--skipMission", missionsToSkipVec, "Mission to skip (can be repeated)")->type_size(1)->allow_extra_args(false);
  app.add_option("-M,--keepMission", missionsToKeepVec, "Mission to keep (can be repeated)")->type_size(1)->allow_extra_args(false);
  app.add_option("-o,--output", outputFilename, "Where to store the data");
  auto* skipGroup = app.add_option_group("first-record", "First record handling");
  skipGroup->add_flag("-s,--skipFirst", qSkipFirstRecord, "Skip first record in each file, but the first");
  skipGroup->add_flag("-A,--skipAll", qSkipAllFirst, "Skip first record in ALL files including the first");
  skipGroup->add_flag("--keepFirst", qKeepFirst, "Keep first record of all files (default)");
  skipGroup->require_option(0, 1);
  app.add_flag("-r,--repair", qRepair, "Attempt to repair bad data records");
  app.add_flag("-S,--strict", qStrict, "Fail immediately on any file error (no partial results)");
  app.add_flag("-v,--verbose", qVerbose, "Enable some diagnostic output");
  app.add_option("--sort", sortOrder, "File sort order (none, header_time, lexicographic)")
     ->default_val("none")
     ->check(CLI::IsMember({"none", "header_time", "lexicographic"}));
  app.add_option("-l,--log-level", logLevel, "Log level (trace,debug,info,warn,error,critical,off)")
     ->default_val("warn");
  app.add_option("files", inputFiles, "Input DBD files")->required()->check(CLI::ExistingFile);
  app.set_version_flag("-V,--version", VERSION);

  CLI11_PARSE(app, argc, argv);

  // Initialize logger
  dbd::logger().init("dbd2csv", dbd::logLevelFromString(logLevel));
  if (qVerbose && logLevel == "warn") {
    dbd::logger().setLevel(dbd::LogLevel::Info);
  }

  // Load sensor names from files if specified
  Sensors::tNames toKeep, criteria;
  if (!sensorsFile.empty()) {
    Sensors::loadNames(sensorsFile.c_str(), criteria);
  }
  if (!sensorOutputFile.empty()) {
    Sensors::loadNames(sensorOutputFile.c_str(), toKeep);
  }

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
      LOG_ERROR("Error opening '{}': {}", outputFilename, strerror(errno));
      return(1);
    }
    osp = outputFile.get();
  }

  SensorsMap smap(sensorCacheDirectory);

  // Go through and grab all the known sensors

  // First pass: discover sensors across all files (files re-opened in second pass for data)
  typedef std::vector<size_t> tFileIndices;
  tFileIndices fileIndices;
  std::vector<time_t> fileOpenTimes;
  std::vector<size_t> fileSizes;

  for (size_t i = 0; i < inputFiles.size(); ++i) {
    const char* fn = inputFiles[i].c_str();
    DecompressTWR is(fn, qCompressed(fn));
    if (!is) {
      LOG_ERROR("Error opening '{}': {}", fn, strerror(errno));
      return(1);
    }
    try {
      const Header hdr(is, fn);
      if (!hdr.empty() && hdr.qProcessMission(missionsToSkip, missionsToKeep)) {
        smap.insert(is, hdr, false);
        fileIndices.push_back(i);
        fileOpenTimes.push_back(Header::parseFileOpenTime(hdr.find("fileopen_time")));
        fileSizes.push_back(fs::file_size(fn));
      }
    } catch (MyException& e) {
      if (qStrict) {
        LOG_ERROR("Error processing '{}': {}", fn, e.what());
        return(1);
      }
      LOG_WARN("Error processing '{}': {} (skipping file)", fn, e.what());
    }
  }

  if (fileIndices.empty()) {
    LOG_ERROR("No input files found to process!");
    return(1);
  }

  if (sortOrder == "header_time") {
    std::vector<size_t> perm(fileIndices.size());
    std::iota(perm.begin(), perm.end(), 0);
    std::sort(perm.begin(), perm.end(), [&](size_t a, size_t b) {
      return fileOpenTimes[a] < fileOpenTimes[b];
    });
    tFileIndices sortedIdx(fileIndices.size());
    std::vector<size_t> sortedSizes(fileSizes.size());
    for (size_t i = 0; i < perm.size(); ++i) {
      sortedIdx[i] = fileIndices[perm[i]];
      sortedSizes[i] = fileSizes[perm[i]];
    }
    fileIndices = std::move(sortedIdx);
    fileSizes = std::move(sortedSizes);
  } else if (sortOrder == "lexicographic") {
    std::vector<size_t> perm(fileIndices.size());
    std::iota(perm.begin(), perm.end(), 0);
    std::sort(perm.begin(), perm.end(), [&](size_t a, size_t b) {
      return inputFiles[fileIndices[a]] < inputFiles[fileIndices[b]];
    });
    tFileIndices sortedIdx(fileIndices.size());
    std::vector<size_t> sortedSizes(fileSizes.size());
    for (size_t i = 0; i < perm.size(); ++i) {
      sortedIdx[i] = fileIndices[perm[i]];
      sortedSizes[i] = fileSizes[perm[i]];
    }
    fileIndices = std::move(sortedIdx);
    fileSizes = std::move(sortedSizes);
  }

  smap.qKeep(toKeep);
  smap.qCriteria(criteria);
  smap.setUpForData(); // Get a common list of sensors

  const Sensors& all(smap.allSensors());
  std::string delim;

  typedef std::vector<size_t> tRowsToOutput;
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
    const size_t i(fileIndices[ii]);
    const char* fn = inputFiles[i].c_str();
    DecompressTWR is(fn, qCompressed(fn));
    if (!is) {
      LOG_ERROR("Error opening '{}': {}", fn, strerror(errno));
      return(1);
    }
    const Header hdr(is, fn);             // Load up header
    smap.insert(is, hdr, true); // Since will move to the right position in the file
    const Sensors& sensors(smap.find(hdr));
    const KnownBytes kb(is);          // Get little/big endian
    Data data;
    const size_t nBytes(fileSizes[ii]);

    try {
      data.load(is, kb, sensors, qRepair, nBytes);
    } catch (MyException& e) {
      if (qStrict) {
        LOG_ERROR("Error processing '{}': {}", fn, e.what());
        return(1);
      }
      LOG_WARN("Error processing '{}': {}, retaining {} records", fn, e.what(), data.size());
    }

    if (data.empty()) continue;

    data.delim(",");

    const size_t n(data.size());
    const size_t kStart(qSkipAllFirst ? 1 : (ii == 0 ? 0 : k0));

    if (n > kStart) { // some data to output
      for (size_t k(kStart); k < n; ++k) {
        for (tRowsToOutput::size_type j(0), je(rowsToOutput.size()); j < je; ++j) {
          const size_t index(rowsToOutput[j]);
          if (j != 0) {
            *osp << ',';
          }
          const double dval(data(k, index));
          if (!std::isnan(dval)) {
            *osp << all[index].toStr(dval);
          }
        }
        *osp << std::endl;
      }
    }

    LOG_INFO("{}: {} records", fn, data.size() - kStart);
  }

  // outputFile automatically cleaned up on function exit

  return(0);
}
