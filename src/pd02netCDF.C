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
#include "config.h"
#include <iostream>
#include <cstdlib>
#include <getopt.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

namespace {
  const char *options("ho:Vv"); 
  const struct option optionsLong[] = {
          {"output", required_argument, NULL, 'o'},
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
    std::cerr << " -h --help             display the usage message" << std::endl;
    std::cerr << " -o --output  filename where to store the data" << std::endl;
    std::cerr << " -V --version          print out version" << std::endl;
    std::cerr << " -v --verbose          some diagnostic output" << std::endl;
    std::cerr << "\nReport bugs to " << MAINTAINER << std::endl;
    return 1;
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  const char *ofn(0);
  bool qVerbose(false);

  while (true) { // Walk through the options
    int thisOptionOptind = optind ? optind : 1;
    int optionIndex = 0;
    int c = getopt_long(argc, argv, options, optionsLong, &optionIndex);
    if (c == -1) break; // End of options

    switch (c) {
      case 'o': // Output filename
        ofn = optarg;
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

  if (!ofn) {
    std::cerr << "ERROR: No output filename specified" << std::endl;
    return usage(argv[0]);
  }

  if (optind >= argc) {
    std::cerr << "No input files specified!" << std::endl;
    return usage(argv[0]);
  }

  uint8_t nCells(0);

  for (int i(optind), hIndex(0); i < argc; ++i, ++hIndex) {
    const uint8_t mCells(PD0::maxNumberOfCells(argv[i]));
    nCells = (nCells >= mCells) ? nCells : mCells;
  }

  if (qVerbose)
    std::cout << "Maximum number of cells " << ((unsigned int) nCells) << std::endl;

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

  for (int i(optind), hIndex(0); i < argc; ++i, ++hIndex) {
    const size_t sIndex(index);
    index = pd0.load(argv[i], nc, index);

    if (index != sIndex) {
      nc.putVar(hdrFilename, hIndex, argv[i]);
      nc.putVar(hdrStartIndex, hIndex, (unsigned int) sIndex);
      nc.putVar(hdrStopIndex, hIndex, (unsigned int) index - 1);
    
      if (qVerbose)
        std::cout << "Found " << (index - sIndex) << " records in " << argv[i] << std::endl;
    } else if (qVerbose) {
        std::cout << "No records found in " << argv[i] << std::endl;
    }
  }

  nc.close();

  return(0);
}
