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
#include <getopt.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

namespace {
  const char *options("afho:uVv"); 
  const struct option optionsLong[] = {
	  {"append", no_argument, NULL, 'c'},
          {"output", required_argument, NULL, 'o'},
	  {"unlimited", no_argument, NULL, 'c'},
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
    std::cerr << " -a --append              append to output file if it exists" << std::endl;
    std::cerr << " -h --help                display the usage message" << std::endl;
    std::cerr << " -o --output    filename  where to store the data" << std::endl;
    std::cerr << " -u --unlimited           do not use unlimited dimensions," << std::endl;
    std::cerr << "                          so output can not be appended to." << std::endl;
    std::cerr << "                          This will be faster and smaller output." << std::endl;
    std::cerr << " -V --version             Print out version" << std::endl;
    std::cerr << " -v --verbose             Enable some diagnostic output" << std::endl;
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
  bool qAppend(false);
  bool qUnlimited(true);

    while (true) { // Walk through the options
    int thisOptionOptind = optind ? optind : 1;
    int optionIndex = 0;
    int c = getopt_long(argc, argv, options, optionsLong, &optionIndex);
    if (c == -1) break; // End of options

    switch (c) {
      case 'a': // Append
        qAppend = !qAppend;
        break;
      case 'o': // Output filename
        ofn = optarg;
        break;
      case 'u': // Unlimited
        qUnlimited = !qUnlimited;
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
    usage(argv[0]);
    return 1;
  }

  SGMerge sg(ofn, qVerbose, qAppend, qUnlimited);

  for (size_t i = optind; i < argc; ++i) { // Loop over input files
    if (!sg.loadFileHeader(argv[i])) return 1;
  }
 
  sg.updateHeader(); 

  for (size_t i = optind; i < argc; ++i) { // Loop over input files
    if (!sg.mergeFile(argv[i])) return 1;
  }

  return(0);
}
