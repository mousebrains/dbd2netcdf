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

// Read in a set of Sea Glider engineering files, and
// output them into a netCDF file

#include "SeaGlider.H"
#include "Variables.H"
#include "NetCDF.H"
#include "Tokenize.H"
#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <map>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstring>
#include <cstdlib>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

int
main(int argc,
     char **argv)
{
  SeaGlider sg(argc, argv);
  Variables vars(sg.remapfn(), sg.typefn());

  vars.typeInfo("filename", NC_STRING, std::string(), "hdr_", "h");
  vars.typeInfo("start_index", NC_UINT, std::string(), "hdr_", "h");
  vars.typeInfo("stop_index", NC_UINT, std::string(), "hdr_", "h");

  // Go through and collect all the sensor names
  
  for (int i(optind); i < argc; ++i) {
    std::ifstream is(argv[i]);
    if (!is) {
      std::cerr << "Error opening '" << argv[i] << "', " << strerror(errno) << std::endl;
      return(1);
    }
    for (std::string line; getline(is, line);) {
      if (line.empty() || (line[0] != '%') || (line.substr(1,4) == "data")) {
        break;
      }
        
      const std::string::size_type index(line.find(':'));

      if (index == line.npos) {
        continue;
      }
      const std::string key(line.substr(1, index - 1));

      if (key == "columns") {
	      const dbd::tTokens fields(dbd::tokenize(line.substr(index+1), ", \t\n\r"));
        for (dbd::tTokens::const_iterator it(fields.begin()), et(fields.end()); 
	           it != et; ++it) {
          vars.typeInfo(*it, -1, std::string(), std::string(), "i");
	      }
        sg.file(argv[i]);
      } else if (key.substr(0,3) == "GPS") {
        vars.typeInfo(key + "_lat", NC_DOUBLE, "deg", "hdr_", "h");
        vars.typeInfo(key + "_lon", NC_DOUBLE, "deg", "hdr_", "h");
        vars.typeInfo(key + "_time", NC_DOUBLE, "seconds", "hdr_", "h");
      } else {
        vars.typeInfo(key, -1, std::string(), "hdr_", "h");
      }
    }
  }

  // Construct netCDF file with sensors

  NetCDF nc(sg.ofn());

  vars.setupVars(nc);

  nc.enddef();

  // Walk through files and process them
  
  size_t count(0);

  for (size_t i(0), ie(sg.nFiles()); i < ie; ++i) {
    const std::string& fn(sg.file(i));
    std::ifstream is(fn.c_str());
    if (!is) {
      std::cerr << "Error opening '" << fn << "', " << strerror(errno) << std::endl;
      return(1);
    }

    nc.putVar(vars.varNum("filename"), i, fn);
    nc.putVar(vars.varNum("start_index"), i, (int) count+1);

    std::string comment;
    typedef std::vector<int> tDataVars;
    tDataVars dataVars;

    for (std::string line; getline(is, line);) {
      if (line.empty() || (line[0] != '%') || (line.substr(1,4) == "data")) {
        break;
      }
        
      const std::string::size_type index(line.find(':'));

      if (index == line.npos) {
        continue;
      }
      const std::string key(line.substr(1, index - 1));
      const std::string remainder(line.substr(index+1));

      if (key == "columns") {
	      const dbd::tTokens fields(dbd::tokenize(remainder, ",\n\r\t "));
        for (dbd::tTokens::const_iterator it(fields.begin()), et(fields.end()); it != et; ++it) {
          dataVars.push_back(vars.varNum(*it));
	      }
      } else if (key == "comment") {
        comment += remainder;
      } else if (key.substr(0,3) == "GPS") { // (44.626850 -124.788450) 04/09/08 21:44:25
        std::istringstream iss(remainder);
        std::string prefix;
        if (getline(iss, prefix, '(')) { // skip leading (
          double lat, lon;
          int mon, mday, year;
          int hour, min, sec;
          if ((iss >> lat >> lon >> prefix >> mon) &&  // get lat, lon, ), and month
              getline(iss, prefix, '/') && (iss >> mday) && // Skip / and get day
              getline(iss, prefix, '/') && (iss >> year >> hour) && // skip / and get year and hour
              getline(iss, prefix, ':') && (iss >> min) && // skip / and get minute
              getline(iss, prefix, ':') && (iss >> sec)) { // skip / and get second
            struct tm ti;
            ti.tm_year = year + 100;
            ti.tm_mon = mon - 1;
            ti.tm_mday = mday;
            ti.tm_hour = hour;
            ti.tm_min = min;
            ti.tm_sec = sec;
            const time_t t(timegm(&ti));
            nc.putVar(vars.varNum(key + "_lat"), i, lat);
            nc.putVar(vars.varNum(key + "_lon"), i, lon);
            nc.putVar(vars.varNum(key + "_time"), i, (double) t);
          }
        }
      } else if (key == "start") {
        const time_t start(sg.mkTime(remainder));
        nc.putVar(vars.varNum(key), i, (double) start);
      } else {
        std::istringstream iss(remainder);
        double value;
        if (!(iss >> value)) {
          std::cerr << "Error reading " << key << " from '" << remainder << "', "
                    << strerror(errno) << std::endl;
          exit(1);
        }
        nc.putVar(vars.varNum(key), i, value);
      }
    }

    if (!comment.empty()) {
      nc.putVar(vars.varNum("comment"), i, comment);
    }

    size_t preCount(count);

    for (std::string line; getline(is, line); ++count) { // Read actual data
      for (std::string::size_type index(0), eindex(line.size()); index < eindex; ++index) {
        if (line[index] == ',')
          line[index] = ' ';
      }
      std::istringstream iss(line);
      for (tDataVars::size_type j(0), je(dataVars.size()); j < je; ++j) {
        double value;
        if ((iss >> value) && !std::isnan(value)) {
          nc.putVar(dataVars[j], count, value);
        }
      }
    }

    nc.putVar(vars.varNum("stop_index"), i, (int) count);

    if (sg.qVerbose()) {
      std::cout << "Processed " << (count - preCount) << " records in " << fn << std::endl;
    }

  }

  nc.close();

  return(0);
}
