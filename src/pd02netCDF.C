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

#include <NetCDF.H>
#include <PD0.H>
#include <MyException.H>
#include <config.h>
#include <iostream>

namespace {
  int usage(const char *argv0, const char *options) {
    std::cerr << argv0 << " Version " << VERSION << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage: " << argv0 << " -[" << options << "] files" << std::endl;
    std::cerr << std::endl;
    std::cerr << " -h           display the usage message" << std::endl;
    std::cerr << " -o filename  where to store the data" << std::endl;
    std::cerr << " -V           Print out version" << std::endl;
    std::cerr << " -v           Enable some diagnostic output" << std::endl;
    std::cerr << "\nReport bugs to " << MAINTAINER << std::endl;
    return 1;
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  const char *options("ho:Vv"); 

  const char *ofn(0);
  bool qVerbose(false);

  for (int ch; (ch = getopt(argc, argv, options)) != -1;) { // Process options
    switch (ch) {
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
        std::cerr << "Unrecognized option '" << ((char) ch) << "'" << std::endl;
      case 'h': // Sensors to select on
        return usage(argv[0], options);
    }
  }

  if (!ofn) {
    std::cerr << "ERROR: No output filename specified" << std::endl;
    return usage(argv[0], options);
  }

  if (optind >= argc) {
    std::cerr << "No input files specified!" << std::endl;
    usage(argv[0], options);
    exit(1);
  }

  uint8_t nCells(0);

  for (int i(optind), hIndex(0); i < argc; ++i, ++hIndex) {
    const uint8_t mCells(PD0::maxNumberOfCells(argv[i]));
    nCells = (nCells >= mCells) ? nCells : mCells;
  }

  if (qVerbose)
    std::cout << "Maximum number of cells " << nCells << std::endl;

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
