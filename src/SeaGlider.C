// Nov-2012, Pat Welch, pat@mousebrains.com

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

// Common code for Sea Glider processing

#include "SeaGlider.H"
#include "Tokenize.H"
#include "config.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

SeaGlider::SeaGlider(int argc,
                     char **argv)
  : mOptions("ho:r:t:Vv")
  , mqVerbose(false)
{
  for (int ch; (ch = getopt(argc, argv, mOptions.c_str())) != -1;) { // Process options
    switch (ch) {
      case 'o': // Output filename
        mofn = optarg;
        break;
      case 'r': // variable mapping filename
        mRemapfn = optarg;
        break;
      case 't': // variable type and unit information filename
        mTypefn = optarg;
        break;
      case 'V': // Print out version string
        std::cerr << VERSION << std::endl;
        exit(0);
      case 'v': // Verbose
        mqVerbose = !mqVerbose;
        break;
      default:
        std::cerr << "Unrecognized option '" << ((char) ch) << "'" << std::endl;
      case 'h': // Sensors to select on
        exit(usage(argv[0]));
    }
  }

  if (mofn.empty()) {
    std::cerr << "ERROR: No output filename specified" << std::endl;
    exit(usage(argv[0]));
  }

  if (optind >= argc) {
    std::cerr << "No input files specified!" << std::endl;
    exit(usage(argv[0]));
  }
}

int
SeaGlider::usage(const char *argv0) const
{
  std::cerr << argv0 << " Version " << VERSION << std::endl;
  std::cerr << std::endl;
  std::cerr << "Usage: " << argv0 << " -[" << mOptions << "] files" << std::endl;
  std::cerr << std::endl;
  std::cerr << " -h           display the usage message" << std::endl;
  std::cerr << " -o filename  where to store the data" << std::endl;
  std::cerr << " -r filename  variable name mapping" << std::endl;
  std::cerr << " -t filename  variable type and units" << std::endl;
  std::cerr << " -V           Print out version" << std::endl;
  std::cerr << " -v           Enable some diagnostic output" << std::endl;
  std::cerr << "\nReport bugs to " << MAINTAINER << std::endl;
  return 1;
}

time_t
SeaGlider::mkTime(const std::string& str)
{
  struct tm ti;
  std::istringstream iss(str);
  if (!(iss >> ti.tm_mon >> ti.tm_mday >> ti.tm_year
            >> ti.tm_hour >> ti.tm_min >> ti.tm_sec)) {
    std::cerr << "Error reading start from '" << str << "', "
              << strerror(errno) << std::endl;
    exit(1);
  }
  --ti.tm_mon;
  const time_t t(timegm(&ti));
  return t;
}
