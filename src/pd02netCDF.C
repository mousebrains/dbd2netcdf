// Apr-2012, Pat Welch, pat@mousebrains.com

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

// Read in a set of PD0 files, and
// output them into a netCDF file

#include "MyNetCDF.H"
#include "PD0.H"
#include "MyException.H"
#include "Logger.H"
#include "config.h"
#include <iostream>
#include <cstdlib>
#include <CLI/CLI.hpp>

int
main(int argc,
     char **argv)
{
  std::string outputFilename;
  std::vector<std::string> inputFiles;
  std::string logLevel = "warn";
  bool qVerbose(false);

  CLI::App app{"Convert PD0 files to NetCDF", "pd02netCDF"};
  app.footer(std::string("\nReport bugs to ") + MAINTAINER);

  app.add_option("-o,--output", outputFilename, "Where to store the data")->required();
  app.add_flag("-v,--verbose", qVerbose, "Enable some diagnostic output");
  app.add_option("-l,--log-level", logLevel, "Log level (trace,debug,info,warn,error,critical,off)")
     ->default_val("warn");
  app.add_option("files", inputFiles, "Input PD0 files")->required()->check(CLI::ExistingFile);
  app.set_version_flag("-V,--version", VERSION);

  CLI11_PARSE(app, argc, argv);

  dbd::logger().init("pd02netCDF", dbd::logLevelFromString(logLevel));
  if (qVerbose && logLevel == "warn") {
    dbd::logger().setLevel(dbd::LogLevel::Info);
  }

  try {
    const char *ofn = outputFilename.c_str();

    uint8_t nCells(0);

    for (size_t i = 0; i < inputFiles.size(); ++i) {
      const uint8_t mCells(PD0::maxNumberOfCells(inputFiles[i].c_str()));
      nCells = (nCells >= mCells) ? nCells : mCells;
    }

    LOG_INFO("Maximum number of cells {}", static_cast<unsigned int>(nCells));

    NetCDF nc(ofn);
    const int hDim(nc.createDim("h"));

    const int hdrFilename(nc.createVar("hdr_filename", NC_STRING, hDim, std::string()));
    const int hdrStartIndex(nc.createVar("hdr_start_index", NC_UINT, hDim, std::string()));
    const int hdrStopIndex(nc.createVar("hdr_stop_index", NC_UINT, hDim, std::string()));

    PD0 pd0;
    pd0.maxNumberOfCells(nCells);
    pd0.setupNetCDFVars(nc);

    nc.enddef();

    size_t index(0);

    for (size_t i = 0, hIndex = 0; i < inputFiles.size(); ++i, ++hIndex) {
      const char* fn = inputFiles[i].c_str();
      const size_t sIndex(index);
      index = pd0.load(fn, nc, index);

      if (index != sIndex) {
        nc.putVar(hdrFilename, hIndex, fn);
        nc.putVar(hdrStartIndex, hIndex, static_cast<unsigned int>(sIndex));
        nc.putVar(hdrStopIndex, hIndex, static_cast<unsigned int>(index - 1));

        LOG_INFO("Found {} records in {}", index - sIndex, fn);
      } else {
        LOG_WARN("No records found in {}", fn);
      }
    }

    nc.close();
  } catch (MyException& e) {
    LOG_CRITICAL("Fatal error: {}", e.what());
    return(2);
  } catch (std::exception& e) {
    LOG_CRITICAL("Unexpected error: {}", e.what());
    return(3);
  }

  return(0);
}
