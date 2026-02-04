// Aug-2016, Pat Welch, pat@mousebrains.com

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

// Read in the netCDF file from a set of Seaglider dives and merge
// them together in the output

#include "SGMerge.H"
#include "config.h"
#include <netcdf.h>
#include <iostream>
#include <string>
#include <vector>
#include <CLI/CLI.hpp>

int
main(int argc,
     char **argv)
{
  std::string outputFilename;
  std::vector<std::string> inputFiles;
  bool qVerbose(false);
  bool qAppend(false);
  bool qUnlimited(true);

  CLI::App app{"Merge Seaglider NetCDF files", "sgMergeNetCDF"};
  app.footer(std::string("\nReport bugs to ") + MAINTAINER);

  app.add_flag("-a,--append", qAppend, "Append to output file if it exists");
  app.add_option("-o,--output", outputFilename, "Where to store the data")->required();
  app.add_flag("-u,--unlimited", [&qUnlimited](int64_t) { qUnlimited = !qUnlimited; },
               "Do not use unlimited dimensions (faster, smaller output, but cannot append)");
  app.add_flag("-v,--verbose", qVerbose, "Enable some diagnostic output");
  app.add_option("files", inputFiles, "Input NetCDF files")->required()->check(CLI::ExistingFile);
  app.set_version_flag("-V,--version", VERSION);

  CLI11_PARSE(app, argc, argv);

  const char *ofn = outputFilename.c_str();

  SGMerge sg(ofn, qVerbose, qAppend, qUnlimited);

  for (const auto& file : inputFiles) { // Loop over input files
    if (!sg.loadFileHeader(file.c_str())) return 1;
  }

  sg.updateHeader();

  for (const auto& file : inputFiles) { // Loop over input files
    if (!sg.mergeFile(file.c_str())) return 1;
  }

  return(0);
}
