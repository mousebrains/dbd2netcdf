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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

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
  const char *options("fho:Vv"); 

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
    return 1;
  }

  SGMerge sg(ofn);

  for (size_t i = optind; i < argc; ++i) { // Loop over input files
    if (!sg.loadFileHeader(argv[i])) return 1;
  }
 
  sg.updateHeader(); 

  for (size_t i = optind; i < argc; ++i) { // Loop over input files
    if (!sg.mergeFile(argv[i])) return 1;
  }

  return(0);
}
