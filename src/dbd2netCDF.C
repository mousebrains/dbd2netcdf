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

#include "MyNetCDF.H"
#include "Header.H"
#include "SensorsMap.H"
#include "KnownBytes.H"
#include "Data.H"
#include "MyException.H"
#include "Logger.H"
#include "config.h"
#include "Decompress.H"
#include "FileInfo.H"
#include <set>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
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
  bool qAppend(false);
  bool qSkipFirstRecord(false);
  bool qSkipAllFirst(false);
  bool qKeepFirst(false);
  bool qRepair(false);
  bool qStrict(false);
  bool qVerbose(false);
  int compressionLevel(5);
  size_t batchSize(100);
  std::string sortOrder = "none";

  CLI::App app{"Convert Dinkum Binary Data files to NetCDF", "dbd2netCDF"};
  app.footer(std::string("\nReport bugs to ") + MAINTAINER);

  app.add_flag("-a,--append", qAppend, "Append to the NetCDF file");
  app.add_option("-c,--sensors", sensorsFile, "File containing sensors to select on");
  app.add_option("-C,--cache", sensorCacheDirectory, "Directory to cache sensor list in");
  app.add_option("-k,--sensorOutput", sensorOutputFile, "File containing sensors to output");
  app.add_option("-m,--skipMission", missionsToSkipVec, "Mission to skip (can be repeated)")->type_size(1)->allow_extra_args(false);
  app.add_option("-M,--keepMission", missionsToKeepVec, "Mission to keep (can be repeated)")->type_size(1)->allow_extra_args(false);
  app.add_option("-o,--output", outputFilename, "Where to store the data")->required();
  auto* skipGroup = app.add_option_group("first-record", "First record handling");
  skipGroup->add_flag("-s,--skipFirst", qSkipFirstRecord, "Skip first record in each file, but the first");
  skipGroup->add_flag("-A,--skipAll", qSkipAllFirst, "Skip first record in ALL files including the first");
  skipGroup->add_flag("--keepFirst", qKeepFirst, "Keep first record of all files (default)");
  skipGroup->require_option(0, 1);
  app.add_flag("-r,--repair", qRepair, "Attempt to repair bad data records");
  app.add_flag("-S,--strict", qStrict, "Fail immediately on any file error (no partial results)");
  app.add_flag("-v,--verbose", qVerbose, "Enable some diagnostic output");
  app.add_option("-z,--compression", compressionLevel, "Zlib compression level (0=none, 9=max)")
     ->default_val("5")
     ->check(CLI::Range(0, 9));
  app.add_option("-b,--batch-size", batchSize, "Files per batch (0=all at once, reduces memory)")
     ->default_val("100");
  app.add_option("--sort", sortOrder, "File sort order (none, header_time, lexicographic)")
     ->default_val("none")
     ->check(CLI::IsMember({"none", "header_time", "lexicographic"}));
  app.add_option("-l,--log-level", logLevel, "Log level (trace,debug,info,warn,error,critical,off)")
     ->default_val("warn");
  app.add_option("files", inputFiles, "Input DBD files")->required()->check(CLI::ExistingFile);
  app.set_version_flag("-V,--version", VERSION);

  CLI11_PARSE(app, argc, argv);

  // Initialize logger
  dbd::logger().init("dbd2netCDF", dbd::logLevelFromString(logLevel));
  if (qVerbose && logLevel == "warn") {
    dbd::logger().setLevel(dbd::LogLevel::Info);
  }

  const char *ofn = outputFilename.c_str();

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

  SensorsMap smap(sensorCacheDirectory);

  // Go through and grab all the known sensors

  // First pass: discover sensors across all files (files re-opened in second pass for data)
  typedef std::vector<size_t> tFileIndices;
  tFileIndices fileIndices;
  std::vector<time_t> fileOpenTimes;
  std::vector<size_t> fileSizes; // Cache file sizes to avoid repeated fs::file_size calls

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
    // Build permutation to keep fileSizes in sync
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

  typedef std::vector<int> tVars;
  tVars vars(smap.allSensors().nToStore());
  std::vector<size_t> varSizes(smap.allSensors().nToStore());

  try {

  // NetCDF dimension names: "i" for data records, "j" for files
  constexpr char DATA_DIMENSION[] = "i";
  constexpr char FILE_DIMENSION[] = "j";

  const Sensors& all(smap.allSensors());

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

  // Go through and grab all the data

  const size_t k0(qSkipFirstRecord ? 1 : 0);

  size_t indexOffset(0);
  size_t jOffset(0);

  const size_t nFiles(fileIndices.size());
  const size_t filesPerBatch(batchSize > 0 ? batchSize : nFiles);

  for (size_t batchStart(0); batchStart < nFiles; batchStart += filesPerBatch) {
    const size_t batchEnd(std::min(batchStart + filesPerBatch, nFiles));

    NetCDF ncid(ofn, qAppend || batchStart > 0);
    ncid.compressionLevel(compressionLevel);

    if (batchStart == 0 && !qAppend) {
      ncid.putGlobalAtt("Conventions", "CF-1.10");
      ncid.putGlobalAtt("history", std::string("Created by dbd2netCDF ") + VERSION);
      ncid.putGlobalAtt("source", "Slocum Glider Dinkum Binary Data files");
    }

    const int iDim(ncid.maybeCreateDim(DATA_DIMENSION));
    const int jDim(ncid.maybeCreateDim(FILE_DIMENSION));

    // Setup variables (maybeCreateVar looks up existing vars on reopen)
    for (Sensors::const_iterator it(all.begin()), et(all.end()); it != et; ++it) {
      const Sensor& sensor(*it);
      if (sensor.qKeep()) {
        const size_t index(sensor.index());
        const std::string& name(sensor.name());
        const std::string& units(sensor.units());
        int idType(-1);
        switch (sensor.size()) {
          case 1: idType = NC_BYTE; break;
          case 2: idType = NC_SHORT; break;
          case 4: idType = NC_FLOAT; break;
          case 8: idType = NC_DOUBLE; break;
          default:
            LOG_ERROR("Unsupported sensor size for {}", sensor.name());
            return(1);
        } // switch

        vars[index] = ncid.maybeCreateVar(name, idType, iDim, units);
        varSizes[index] = sensor.size();
      } // if sensor.qKeep
    } // for all

    for (tVars::size_type i(0), e(hdrVars.size()); i < e; ++i) {
      const std::string& name("hdr_" + hdrNames[i]);
      hdrVars[i] = ncid.maybeCreateVar(name, NC_STRING, jDim, std::string());
    }

    const int hdrStartIndex(ncid.maybeCreateVar("hdr_start_index", NC_UINT, jDim, std::string()));
    const int hdrStopIndex(ncid.maybeCreateVar("hdr_stop_index", NC_UINT, jDim, std::string()));
    const int hdrLength(ncid.maybeCreateVar("hdr_nRecords", NC_UINT, jDim, std::string()));

    if (batchStart == 0) {
      indexOffset = qAppend ? ncid.lengthDim(iDim) : 0;
      jOffset = qAppend ? ncid.lengthDim(jDim) : 0;
    }

    for (tFileIndices::size_type ii(batchStart); ii < batchEnd; ++ii) {
      const size_t i(fileIndices[ii]);
      const char* fn = inputFiles[i].c_str();
      DecompressTWR is(fn, qCompressed(fn));
      if (!is) {
        LOG_ERROR("Error opening '{}': {}", fn, strerror(errno));
        return(1);
      }
      const Header hdr(is, fn);             // Load up header
      try {
        smap.insert(is, hdr, true);       // will move to the right position in the file
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

        const size_t n(data.size());
        const size_t kStart(qSkipAllFirst ? 1 : (ii == 0 ? 0 : k0));

        { // Update file info
          for (tVars::size_type j(0), je(hdrVars.size()); j < je; ++j) {
            const std::string str(hdr.find(hdrNames[j]));
            ncid.putVar(hdrVars[j], static_cast<size_t>(ii) + jOffset, str);
          }
          if (n > kStart) {
            const unsigned int stopIndex(static_cast<unsigned int>(indexOffset + n - kStart - 1));

            ncid.putVar(hdrStartIndex, static_cast<size_t>(ii) + jOffset, static_cast<unsigned int>(indexOffset));
            ncid.putVar(hdrStopIndex, static_cast<size_t>(ii) + jOffset, stopIndex);
          }
          ncid.putVar(hdrLength, static_cast<size_t>(ii) + jOffset, static_cast<unsigned int>(n - kStart));
        }

        if (n <= kStart) { // No data to be written
          continue;
        }

        const size_t writeCount(n - kStart);
        std::vector<double> values(writeCount);

        for (tVars::size_type j(0), je(vars.size()); j < je; ++j) {
          const int var(vars[j]);
          const Data::tColumn& col(data.column(j));
          if (varSizes[j] == 8) {
            // NC_DOUBLE: NaN and inf are both representable
            ncid.putVars(var, indexOffset, writeCount, &col[kStart]);
          } else {
            // NC_FLOAT/SHORT/BYTE: replace NaN/inf with fill value
            double fillValue = NAN;
            if (varSizes[j] == 1) fillValue = -127.0;
            else if (varSizes[j] == 2) fillValue = -32768.0;
            for (size_t k(0); k < writeCount; ++k) {
              const double v(col[kStart + k]);
              values[k] = (std::isnan(v) || std::isinf(v)) ? fillValue : v;
            }
            ncid.putVars(var, indexOffset, writeCount, values.data());
          }
        }

        indexOffset += data.size() - kStart;

        LOG_INFO("{}: {} records written", fn, data.size() - kStart);
      } catch (MyException& e) { // Catch my exceptions, where I toss the whole file
        if (qStrict) {
          LOG_ERROR("Error processing '{}': {}", fn, e.what());
          return(1);
        }
        LOG_WARN("Error processing '{}': {} (skipping file)", fn, e.what());
      }
    }

    ncid.close();
  } // for batchStart

  } catch (MyException& e) {
    LOG_CRITICAL("Fatal error: {}", e.what());
    return(2);
  } catch (std::exception& e) {
    LOG_CRITICAL("Unexpected error: {}", e.what());
    return(3);
  }

  return(0);
}
