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
#include <vector>
#include <random>
#include <CLI/CLI.hpp>

namespace {
  std::string mkOutputFilename(const std::string& dir, const std::string& ifn) {
    const fs::path inPath(ifn);
    const std::string fn(inPath.filename().string());
    fs::path outPath = dir.empty() ? fs::path(fn) : (fs::path(dir) / fn);
    std::string ext(outPath.extension().string());
    if ((ext.size() == 4) && (tolower(ext[2]) == 'c')) {
      switch (ext[3]) {
        case 'g': ext[2] = 'l'; break;
        case 'G': ext[2] = 'L'; break;
        case 'd': ext[2] = 'b'; break;
        case 'D': ext[2] = 'B'; break;
      }
    }
    outPath.replace_extension(ext);
    return outPath.string();
  }

  // Cross-platform unique ID generation (replaces getpid())
  std::string uniqueSuffix() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    return std::to_string(dis(gen));
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  std::string directory;
  std::vector<std::string> inputFiles;
  bool qStdOut(false);
  bool qVerbose(false);

  CLI::App app{"Decompress TWR Slocum lz4 compressed files", "decompressTWR"};
  app.footer(std::string("\nReport bugs to ") + MAINTAINER);

  app.add_option("-o,--output", directory, "Directory where to store the data");
  app.add_flag("-s,--stdout", qStdOut, "Output to stdout");
  app.add_flag("-v,--verbose", qVerbose, "Enable some diagnostic output");
  app.add_option("files", inputFiles, "Input files")->required()->check(CLI::ExistingFile);
  app.set_version_flag("-V,--version", VERSION);

  CLI11_PARSE(app, argc, argv);

  for (const auto& ifn : inputFiles) {
    DecompressTWR is(ifn, qCompressed(ifn));
    if (!is) {
      std::cerr << "Error opening '" << ifn << "', " << strerror(errno) << std::endl;
      return(1);
    }

    if (qStdOut) {
        constexpr size_t BUFFER_SIZE = 1024 * 1024;
        std::vector<char> buffer(BUFFER_SIZE);  // RAII heap allocation (1MB)
        while (is) { // Loop unitl EOF
            if (is.read(buffer.data(), buffer.size()) || is.gcount()) {
		    std::cout.write(buffer.data(), is.gcount());
	    }
	    std::cout.flush();
        }
	continue;
    }

    const std::string ofn(mkOutputFilename(directory, ifn));
    const std::string tfn(ofn + "." + uniqueSuffix());
    try {
      std::ofstream os(tfn.c_str());
      if (!os) {
        std::cerr << "Error opening '" << tfn << "', " << strerror(errno) << std::endl;
	return 1;
      }
      constexpr size_t BUFFER_SIZE = 1024 * 1024;
      std::vector<char> buffer(BUFFER_SIZE);  // RAII heap allocation (1MB)
      while (is) { // Loop until EOF
          if (is.read(buffer.data(), buffer.size()) || is.gcount()) {
	    os.write(buffer.data(), is.gcount());
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
