// Jan-2023, Pat Welch, pat@mousebrains.com

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

// Decompress the TWR Slocum lz4 compressed files.

#include "config.h"
#include "Decompress.H"
#include "FileInfo.H"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <getopt.h>

namespace {
  const char *options("o:Vv"); 
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
    std::cerr << " -h --help              display the usage message" << std::endl;
    std::cerr << " -o --output  directory where to store the data" << std::endl;
    std::cerr << " -V --version           print out version" << std::endl;
    std::cerr << " -v --verbose           enable some diagnostic output" << std::endl;
    std::cerr << "\nReport bugs to " << MAINTAINER << std::endl;
    return 1;
  }

  std::string mkOutputFilename(const char *dir, const std::string& ifn) {
    const std::string fn(fs::filename(ifn));
    std::string ofn(dir == NULL ? fn : (std::string(dir) + "/" + fn));
    std::string ext(fs::extension(ofn));
    if ((ext.size() == 4) && (tolower(ext[2]) == 'c')) {
      switch (ext[3]) {
	case 'g': ext[2] = 'l'; break;
	case 'G': ext[2] = 'L'; break;
	case 'd': ext[2] = 'b'; break;
	case 'D': ext[2] = 'B'; break;
      }
    }
    return fs::replace_extension(ofn, ext);
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  const char *directory(NULL);
  bool qVerbose(false);

  while (true) { // Walk through the options
    int thisOptionOptind = optind ? optind : 1;
    int optionIndex = 0;
    int c = getopt_long(argc, argv, options, optionsLong, &optionIndex);
    if (c == -1) break; // End of options
    
    switch (c) {
      case 'o': // Output filename
        directory = optarg;
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

  if (optind >= argc) {
    std::cerr << "No input files specified!" << std::endl;
    return usage(argv[0]);
  }

  for (int i = optind; i < argc; ++i) {
    std::string ifn(argv[i]);
    DecompressTWR is(ifn, true);
    if (!is) {
      std::cerr << "Error opening '" << ifn << "', " << strerror(errno) << std::endl;
      return(1);
    }
    const std::string ofn(mkOutputFilename(directory, ifn));
    std::ostringstream oss; // to_string not defined in CentOS 7
    oss << getpid();
    const std::string tfn(ofn + "." + oss.str());
    try {
      std::ofstream os(tfn.c_str());
      if (!os) {
        std::cerr << "Error opening '" << tfn << "', " << strerror(errno) << std::endl;
	return 1;
      }
      while (is) { // Loop until EOF
          char buffer[1024*1024];
          if (is.read(buffer, sizeof(buffer)) || is.gcount()) {
	    os.write(buffer, is.gcount());
	  }
      }
      os.close();
      std::rename(tfn.c_str(), ofn.c_str());
    } catch (int e) {
      std::cerr << "Error creating '" << ofn << "', " << strerror(e) << std::endl;
      // remove(tfn); // Not found on Macos
    }
  }

  return(0);
}
