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

#include "NetCDF.H"
#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <map>
#include <vector>
#include <ctime>
#include <cmath>

namespace {
  int usage(const char *argv0, const char *options) {
    std::cerr << argv0 << " Version " << VERSION << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage: " << argv0 << " -[" << options << "] files" << std::endl;
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

  typedef std::map<std::string, std::string> tMap;
  tMap variableNameMap;

  typedef std::vector<std::string> tTokens;
  tTokens tokenize(std::string value) {
    tTokens tokens;
    while (!value.empty()) {
      const std::string::size_type index(value.find(','));
      std::string field(value.substr(0,index));
      field = field.substr(field.find_first_not_of(" \t\n\r"));
      field = field.substr(0, field.find_last_not_of(" \t\n\r")+1);
      if (!field.empty()) {
        tMap::const_iterator it(variableNameMap.find(field));
        if (it != variableNameMap.end()) { // Rename the variable if in the map
          field = it->second;
        }
        tokens.push_back(field);
      }
      if (index == value.npos) {
        break;
      }
      value = value.substr(index+1);
    }
    return tokens;
  }
} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  const char *options("ho:r:t:Vv"); 

  const char *ofn(0);
  const char *mapfn(0);
  const char *typfn(0);
  bool qVerbose(false);

  for (int ch; (ch = getopt(argc, argv, options)) != -1;) { // Process options
    switch (ch) {
      case 'o': // Output filename
        ofn = optarg;
        break;
      case 'r': // variable mapping filename
        mapfn = optarg;
        break;
      case 't': // variable type and unit information filename
        typfn = optarg;
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

  if (mapfn) { // Load in variable name mapping information
    std::ifstream is(mapfn);
    if (!is) {
      std::cerr << "Error opening '" << mapfn << "', " << strerror(errno) << std::endl;
      return(1);
    }
    for (std::string line; getline(is, line);) {
      std::istringstream iss(line);
      std::string name0, name1;
      if ((iss >> name0 >> name1)) {
        variableNameMap.insert(std::make_pair(name0, name1));
      }
    }
  }

  // Go through and collect all the sensor names
  
  typedef std::map<std::string, size_t> tSensors;
  tSensors sensors;
  tSensors headers;

  typedef std::vector<std::string> tFiles;
  tFiles files;

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
	      const tTokens fields(tokenize(line.substr(index+1)));
        for (tTokens::const_iterator it(fields.begin()), et(fields.end()); 
	           it != et; ++it) {
	        tSensors::const_iterator jt(sensors.find(*it));
	        if (jt == sensors.end()) {
	          sensors.insert(std::make_pair(*it, sensors.size()));
	        }
	      }
        files.push_back(argv[i]);
      } else if (key.substr(0,3) == "GPS") {
        headers.insert(std::make_pair(key + "_lat", headers.size()));
        headers.insert(std::make_pair(key + "_lon", headers.size()));
        headers.insert(std::make_pair(key + "_time", headers.size()));
      } else {
        headers.insert(std::make_pair(key, headers.size()));
      }
    }
  }

  typedef std::pair<nc_type, std::string> tTypeUnits;
  typedef std::map<std::string, tTypeUnits> tTypeUnitsMap;
  tTypeUnitsMap typeUnitsMap;

  if (typfn) { // If specified, get typing and unit information for the variables
    std::ifstream is(typfn);
    if (!is) {
      std::cerr << "Error opening '" << typfn << "', " << strerror(errno) << std::endl;
      return(1);
    }
    for (std::string line; getline(is, line);) {
      std::istringstream iss(line);
      std::string name, dataType, units;
      if ((iss >> name >> dataType)) {
        iss >> units; // Optional third argument
        tTypeUnits tu(std::make_pair(NC_DOUBLE, units));
        if (dataType == "double") {
          tu.first = NC_DOUBLE;
        } else if (dataType == "float") {
          tu.first = NC_FLOAT;
        } else if (dataType == "uint32") {
          tu.first = NC_UINT;
        } else if (dataType == "int32") {
          tu.first = NC_INT;
        } else if (dataType == "uint16") {
          tu.first = NC_USHORT;
        } else if (dataType == "int16") {
          tu.first = NC_SHORT;
        } else if (dataType == "uint8") {
          tu.first = NC_UBYTE;
        } else if (dataType == "int8") {
          tu.first = NC_BYTE;
        } else {
          std::cerr << "Unrecognized data type '" << dataType << "'" << std::endl;
          return(1);
        }
        typeUnitsMap.insert(std::make_pair(name, tu));
      }
    }
  }

  // Construct netCDF file with sensors

  NetCDF nc(ofn);
  const int iDim(nc.createDim("i"));
  const int hDim(nc.createDim("h"));

  const int hdrFilename(nc.createVar("hdr_filename", NC_STRING, hDim, std::string()));
  const int hdrStartIndex(nc.createVar("hdr_start_index", NC_UINT, hDim, std::string()));
  const int hdrStopIndex(nc.createVar("hdr_stop_index", NC_UINT, hDim, std::string()));

  typedef std::vector<int> tVars;
  tVars vars(sensors.size(), -1);
  tVars hdrs(headers.size(), -1);

  for (tSensors::const_iterator it(headers.begin()), et(headers.end()); it != et; ++it) {
    const std::string& name("hdr_" + it->first);
    if (name == "hdr_comment") {
      hdrs[it->second] = nc.createVar(name, NC_STRING, hDim, std::string());
    } else {
      tTypeUnitsMap::const_iterator jt(typeUnitsMap.find(name));
      if (jt == typeUnitsMap.end()) {
        hdrs[it->second] = nc.createVar(name, NC_DOUBLE, hDim, std::string());
      } else {
        hdrs[it->second] = nc.createVar(name, jt->second.first, hDim, jt->second.second);
      }
    }
  }

  for (tSensors::const_iterator it(sensors.begin()), et(sensors.end()); it != et; ++it) {
    const std::string& name(it->first);
    tTypeUnitsMap::const_iterator jt(typeUnitsMap.find(name));
    if (jt == typeUnitsMap.end()) {
      vars[it->second] = nc.createVar(name, NC_DOUBLE, iDim, std::string());
    } else {
      vars[it->second] = nc.createVar(name, jt->second.first, iDim, jt->second.second);
    }
  }

  nc.enddef();

  // Walk through files and process them
  
  size_t count(0);

  for (tFiles::size_type i(0), ie(files.size()); i < ie; ++i) {
    std::ifstream is(files[i].c_str());
    if (!is) {
      std::cerr << "Error opening '" << files[i] << "', " << strerror(errno) << std::endl;
      return(1);
    }

    typedef std::vector<size_t> tDataIndices;
    tDataIndices dataIndices;

    nc.putVar(hdrFilename, i, files[i]);
    nc.putVar(hdrStartIndex, i, (int) count+1);

    std::string comment;

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

      if (key.substr(0,3) == "GPS") { // (44.626850 -124.788450) 04/09/08 21:44:25
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
            nc.putVar(hdrs[headers.find(key + "_lat")->second], i, lat);
            nc.putVar(hdrs[headers.find(key + "_lon")->second], i, lon);
            nc.putVar(hdrs[headers.find(key + "_time")->second], i, (double) t);
          }
        }
      } else if (key == "comment") {
        comment += remainder;
      } else if (key == "columns") {
	      const tTokens fields(tokenize(remainder));
        for (tTokens::const_iterator it(fields.begin()), et(fields.end()); it != et; ++it) {
	        tSensors::const_iterator jt(sensors.find(*it));
	        dataIndices.push_back(jt->second);
	      }
      } else if (key == "start") {
	      struct tm ti;
        std::istringstream iss(remainder);
        if (!(iss >> ti.tm_mon >> ti.tm_mday >> ti.tm_year
		              >> ti.tm_hour >> ti.tm_min >> ti.tm_sec)) {
          std::cerr << "Error reading start from '" << line << "', " 
		                << strerror(errno) << std::endl;
	        return(1);
	      }
	      --ti.tm_mon;
	      const time_t start = timegm(&ti);
        nc.putVar(hdrs[headers.find(key)->second], i, (double) start);
      } else {
        tSensors::const_iterator it(headers.find(key));

        if (it != headers.end()) {
          std::istringstream iss(remainder);
          double value;
          if (!(iss >> value)) {
            std::cerr << "Error reading " << key << " from '" << line << "', " 
		                  << strerror(errno) << std::endl;
	          return(1);
	        }
          nc.putVar(hdrs[it->second], i, value);
        }
      }
    }

    if (!comment.empty()) {
      nc.putVar(hdrs[headers.find("comment")->second], i, comment);
    }

    size_t preCount(count);

    for (std::string line; getline(is, line); ++count) { // Read actual data
      for (std::string::size_type index; (index = line.find(',')) != line.npos;) { // Commas
        line[index] = ' ';
      }
      std::istringstream iss(line);
      for (tDataIndices::size_type j(0), je(dataIndices.size()); j < je; ++j) {
        double value;
        if ((iss >> value) && !std::isnan(value)) {
          nc.putVar(vars[dataIndices[j]], count, value);
        }
      }
    }

    nc.putVar(hdrStopIndex, i, (int) count);

    if (qVerbose) {
      std::cout << "Processed " << (count - preCount) << " records in " << files[i] << std::endl;
    }

  }

  nc.close();

  return(0);
}
