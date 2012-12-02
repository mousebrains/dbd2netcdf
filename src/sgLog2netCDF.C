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

// Read in a set of Sea Glider log files, and
// output them into a netCDF file

#include "SeaGlider.H"
#include "Variables.H"
#include "NetCDF.H"
#include "Tokenize.H"
#include "config.h"
#include <iostream>
#include <fstream>
#include <cerrno>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstdlib>

namespace {
  double mkNum(const std::string& str) {
    const char *ptr(str.c_str());
    char *eptr;
    const double val(std::strtod(ptr, &eptr));
    if ((eptr - ptr) != str.size()) {
      std::cerr << "Error converting '" << str << "' to a double" << std::endl;
    }
    return val;
  }

  double mkDegrees(const std::string& str) {
    const double val(mkNum(str));
    const double sign(val < 0 ? -1 : 1);
    const double aval(fabs(val));
    const double deg(floor(aval / 100));
    const double min(aval - deg * 100);
    return sign * (deg + min / 60);
  }

  double mkGPSTime(const std::string& date, const std::string& time) {
    const double dval(mkNum(date));
    const double tval(mkNum(time));
    struct tm ti;
    ti.tm_mday = (int) fmod(dval / 10000, 100);
    ti.tm_mon  = (int) fmod(dval / 100, 100) - 1;
    ti.tm_year = (int) fmod(dval, 100) + 100;
    ti.tm_hour = (int) fmod(tval / 10000, 100);
    ti.tm_min = (int) fmod(tval / 100, 100);
    ti.tm_sec = (int) fmod(tval, 100);
    const time_t t(timegm(&ti));
    return (double) t;
  }

  bool chkTokens(const dbd::tTokens& tokens, const dbd::tTokens::size_type n, 
                 const std::string& line, const std::string& fn) {
    if (n != tokens.size()) {
      std::cerr << "Incorrect number of tokens, " << tokens.size()
                << " != " << n << ", in " << fn << std::endl;
      std::cerr << line << std::endl;
      std::cerr << tokens << std::endl;
      return false;
    }
    return true;
  }

} // Anonymous namespace

int
main(int argc,
     char **argv)
{
  SeaGlider sg(argc, argv);
  Variables vars(sg.remapfn(), sg.typefn());

  vars.typeInfo("filename", NC_STRING, std::string(), "hdr_", "iHdr");
  vars.typeInfo("start_iGC", NC_UINT, std::string(), "hdr_", "iHdr");
  vars.typeInfo("stop_iGC", NC_UINT, std::string(), "hdr_", "iHdr");
  vars.typeInfo("start_iState", NC_UINT, std::string(), "hdr_", "iHdr");
  vars.typeInfo("stop_iState", NC_UINT, std::string(), "hdr_", "iHdr");

  // Go through and collect all the sensor names
  
  for (int i(optind); i < argc; ++i) {
    std::ifstream is(argv[i]);

    if (!is) {
      std::cerr << "Error opening '" << argv[i] << "', " << strerror(errno) << std::endl;
      return(1);
    }

    for (std::string line; getline(is, line);) { // Go through header
      if (line.empty() || (line == "data:")) {
        break;
      }
        
      const std::string::size_type index(line.find(':'));

      if (index == line.npos) {
        continue;
      }
      const std::string key(line.substr(0, index));
      vars.typeInfo(key, -1, std::string(), "hdr_", "iHdr");
    }

    sg.file(argv[i]);

    for (std::string line; getline(is, line);) { // Go through body
      if (line.empty() || (line[0] != '$') || (line.rfind('$') != 0)) {
        continue;
      }
      const std::string::size_type index(line.find(','));
      const std::string key(line.substr(1,line.find(',')-1));
      dbd::tTokens tokens(dbd::tokenize(line.substr(index+1), ",\n\r"));

      if (key == "GCHEAD") {
        for (dbd::tTokens::const_iterator it(tokens.begin()), et(tokens.end()); it != et; ++it) {
          vars.typeInfo(*it, -1, std::string(), std::string(), "iGC");
        }
      } else if (key == "GC") { // Skip these here
      } else if (key == "STATE") {
        vars.typeInfo("STATE_TIME", NC_DOUBLE, "seconds", std::string(), "iState");
        vars.typeInfo("STATE_STATE", NC_STRING, std::string(), std::string(), "iState");
      } else if (key.substr(2) == "V_AH") {
        const std::string voltage(key.substr(0,2));
        vars.typeInfo("VOLTS_" + voltage + "V", NC_DOUBLE, "Volts", std::string(), "iHdr");
        vars.typeInfo("AMPS_" + voltage + "V", NC_DOUBLE, "Amps-Hours", std::string(), "iHdr");
      } else if (key == "RECOV_CODE") {
        // Do an indirect reference to avoid a net CDF issues with multiple strings
        // which I don't understand yet.
        vars.typeInfo(key, NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("STRINGS", NC_STRING, std::string(), std::string(), "iStrings");
      } else if (key == "SPEED_LIMITS") {
        vars.typeInfo("SPEED_HORZ_MIN", NC_DOUBLE, "cm/sec", std::string(), "iHdr");
        vars.typeInfo("SPEED_HORZ_MAX", NC_DOUBLE, "cm/sec", std::string(), "iHdr");
      } else if (key == "TGT_NAME") {
        if (chkTokens(tokens, 1, line, argv[i]))
          vars.typeInfo(key, NC_STRING, std::string(), std::string(), "iHdr");
      } else if (key == "TGT_LATLONG") {
        vars.typeInfo("TGT_LAT", NC_DOUBLE, "deg", std::string(), "iHdr");
        vars.typeInfo("TGT_LON", NC_DOUBLE, "deg", std::string(), "iHdr");
      } else if (key == "KALMAN_CONTROL") {
        vars.typeInfo(key + "_V", NC_DOUBLE, "m/s", std::string(), "iHdr");
        vars.typeInfo(key + "_U", NC_DOUBLE, "m/s", std::string(), "iHdr");
      } else if ((key == "KALMAN_X") || (key == "KALMAN_Y")) {
        vars.typeInfo(key + "_MEAN", NC_DOUBLE, "m", std::string(), "iHdr");
        vars.typeInfo(key + "_DIURNAL", NC_DOUBLE, "m", std::string(), "iHdr");
        vars.typeInfo(key + "_SEMIDIURNAL", NC_DOUBLE, "m", std::string(), "iHdr");
        vars.typeInfo(key + "_SPEED", NC_DOUBLE, "m", std::string(), "iHdr");
        vars.typeInfo(key + "_DISPLACEMENT", NC_DOUBLE, "m", std::string(), "iHdr");
      } else if (key == "MHEAD_RNG_PITCHd_Wd") {
        vars.typeInfo("TGT_HEADING", NC_DOUBLE, "deg", std::string(), "iHdr");
        vars.typeInfo("TGT_RANGE", NC_DOUBLE, "m", std::string(), "iHdr");
        vars.typeInfo("TGT_PITCH", NC_DOUBLE, "deg", std::string(), "iHdr");
        vars.typeInfo("TGT_W", NC_DOUBLE, "cm/s", std::string(), "iHdr");
      } else if (key == "FINISH") {
        vars.typeInfo("FINISH_DEPTH", NC_DOUBLE, "m", std::string(), "iHdr");
        vars.typeInfo("FINISH_DENSITY", NC_DOUBLE, "g/ml", std::string(), "iHdr");
      } else if (key == "SM_CCo") {
        vars.typeInfo("SM_TIME", NC_DOUBLE, "seconds", std::string(), "iHdr");
        vars.typeInfo("SM_PUMP_TIME", NC_DOUBLE, "seconds", std::string(), "iHdr");
        vars.typeInfo("SM_AMPS", NC_DOUBLE, "amps", std::string(), "iHdr");
        vars.typeInfo("SM_RETRIES", NC_DOUBLE, std::string(), std::string(), "iHdr");
        vars.typeInfo("SM_ERRORS", NC_DOUBLE, std::string(), std::string(), "iHdr");
        vars.typeInfo("SM_POSITION_COUNTS", NC_DOUBLE, std::string(), std::string(), "iHdr");
        vars.typeInfo("SM_POSITION", NC_DOUBLE, "ml", std::string(), "iHdr");
      } else if (key == "SM_GC") {
        // std::cout << tokens.size() << " " << line << std::endl; // TPW
      } else if (key == "IRIDIUM_FIX") {
        vars.typeInfo(key + "_LAT", NC_DOUBLE, "deg", std::string(), "iHdr");
        vars.typeInfo(key + "_LON", NC_DOUBLE, "deg", std::string(), "iHdr");
        vars.typeInfo(key + "_TIME", NC_DOUBLE, "seconds", std::string(), "iHdr");
      } else if (key == "DEVICES") {
        for (dbd::tTokens::const_iterator it(tokens.begin()), et(tokens.end()); it != et; ++it) {
          vars.typeInfo("DEV_" + *it + "_TIME", -1, "seconds", std::string(), "iHdr");
          vars.typeInfo("DEV_" + *it + "_CURRENT", -1, "mAmps", std::string(), "iHdr");
        }
      } else if (key == "DEVICE_SECS") { // Skip this here
      } else if (key == "DEVICE_MAMPS") { // Skip this here
      } else if (key == "SENSORS") {
        for (dbd::tTokens::const_iterator it(tokens.begin()), et(tokens.end()); it != et; ++it) {
          if (*it != "nil") {
            vars.typeInfo("SEN_" + *it + "_TIME", -1, "seconds", std::string(), "iHdr");
            vars.typeInfo("SEN_" + *it + "_CURRENT", -1, "mAmps", std::string(), "iHdr");
          }
        }
      } else if (key == "SENSOR_SECS") { // Skip this here
      } else if (key == "SENSOR_MAMPS") { // Skip this here
      } else if (key == "DATA_FILE_SIZE") {
        vars.typeInfo("DATA_FILE_SIZE", NC_UINT, "bytes", std::string(), "iHdr");
        vars.typeInfo("DATA_FILE_FREE", NC_UINT, "bytes", std::string(), "iHdr");
      } else if (key == "CAP_FILE_SIZE") {
        vars.typeInfo("CAP_FILE_SIZE", NC_UINT, "bytes", std::string(), "iHdr");
        vars.typeInfo("CAP_FILE_FREE", NC_UINT, "bytes", std::string(), "iHdr");
      } else if (key == "CFSIZE") {
        vars.typeInfo("CFSIZE_TOTAL", NC_UINT, "bytes", std::string(), "iHdr");
        vars.typeInfo("CFSIZE_FREE", NC_UINT, "bytes", std::string(), "iHdr");
      } else if (key == "ERRORS") {
        vars.typeInfo("ERRORS_BUFFER_OVERRUNS", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_SPURIOUS_INTERRUPTS", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_CF8_OPEN", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_CF8_WRITE", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_CF8_CLOSE", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_CF8_RETRIES", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_PITCH", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_ROLL", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_VBD", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_PITCH_RETRIES", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_ROLL_RETRIES", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_VBD_RETRIES", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_GPS", NC_UINT, std::string(), std::string(), "iHdr");
        vars.typeInfo("ERRORS_SENSOR", NC_UINT, std::string(), std::string(), "iHdr");
      } else if ((key.substr(0,3) == "GPS") && (key != "GPS_DEVICE")) { // GPS but not GPS_DEVICE
        vars.typeInfo(key + "_TIME", NC_DOUBLE, "seconds", std::string(), "iHdr");
        vars.typeInfo(key + "_LAT", NC_DOUBLE, "deg", std::string(), "iHdr");
        vars.typeInfo(key + "_LON", NC_DOUBLE, "deg", std::string(), "iHdr");
        vars.typeInfo(key + "_TO_FIX", NC_DOUBLE, "seconds", std::string(), "iHdr");
        vars.typeInfo(key + "_HDOP", NC_DOUBLE, std::string(), std::string(), "iHdr");
        vars.typeInfo(key + "_TIME_TO_AQUIRE", NC_DOUBLE, "seconds", std::string(), "iHdr");
        vars.typeInfo(key + "_GotMe", NC_DOUBLE, std::string(), std::string(), "iHdr");
      } else if (key == "ALTIM_BOTTOM_PING") {
        vars.typeInfo(key + "_DEPTH", NC_DOUBLE, "m", std::string(), "iHdr");
        vars.typeInfo(key + "_ALTITUDE", NC_DOUBLE, "m", std::string(), "iHdr");
      } else {
        if (tokens.size() != 1) {
          std::cout << tokens.size() << " " << line << std::endl;
        }
        vars.typeInfo(key, -1, std::string(), std::string(), "iHdr");
      }
    }
  }

  // Construct netCDF file with sensors

  NetCDF nc(sg.ofn());

  vars.setupVars(nc);

  nc.enddef();

  // Walk through files and process them
  
  size_t countGC(0), countState(0);

  typedef std::map<std::string, size_t> tStringIndices;
  tStringIndices stringIndices;

  for (size_t i(0), ie(sg.nFiles()); i < ie; ++i) {
    const std::string& fn(sg.file(i));
    std::ifstream is(fn.c_str());
    if (!is) {
      std::cerr << "Error opening '" << fn << "', " << strerror(errno) << std::endl;
      return(1);
    }

    typedef std::vector<int> tDataVars;
    tDataVars gcVars, senTimes, senCurrent, devTimes, devCurrent;

    nc.putVar(vars.varNum("filename"), i, fn);
    nc.putVar(vars.varNum("start_iGC"), i, (int) countGC+1);
    nc.putVar(vars.varNum("start_iState"), i, (int) countState+1);

    time_t tStart(0);

    for (std::string line; getline(is, line);) {
      if (line.empty() || (line == "data:")) {
        break;
      }
       
      const std::string::size_type index(line.find(':'));

      if (index == line.npos) {
        continue;
      }

      const std::string key(line.substr(0, index));
      const std::string remainder(line.substr(index+1));

      if (key == "start") {
        tStart = sg.mkTime(remainder);
        nc.putVar(vars.varNum(key), i, (double) tStart);
      } else {
        const double val(mkNum(remainder));
        nc.putVar(vars.varNum(key), i, val);
      }
    }

    for (std::string line; getline(is, line); ) { // Read actual data
      if (line.empty() || (line[0] != '$') || (line.rfind('$') != 0)) {
        continue;
      }

      const std::string::size_type index(line.find(','));
      const std::string key(line.substr(1,line.find(',')-1));
      dbd::tTokens tokens(dbd::tokenize(line.substr(index+1), ",\t\n\r"));

      if (key == "GCHEAD") {
        gcVars.resize(tokens.size(), -1);

        for (dbd::tTokens::size_type j(0), je(tokens.size()); j < je; ++j) {
          gcVars[j] = vars.varNum(tokens[j]);
        }
      } else if (key == "GC") { // Skip these here
        if (chkTokens(tokens, gcVars.size(), line, argv[i])) {
          for (dbd::tTokens::size_type j(0), je(tokens.size()); j < je; ++j) {
            nc.putVar(gcVars[j], countGC, mkNum(tokens[j]));
          }
          ++countGC;
        }
      } else if (key == "STATE") {
        if ((tokens.size() == 2) || (tokens.size() == 3)) {
          nc.putVar(vars.varNum("STATE_TIME"), countState, (double) tStart + mkNum(tokens[0]));
          const std::string str(tokens[1] + ((tokens.size() > 2) ? (" " + tokens[2]) : ""));
          nc.putVar(vars.varNum("STATE_STATE"), countState, str);
          ++countState;
        } else {
          std::cerr << "Incorrect number of tokens, " << tokens.size()
                    << " != 2 or 3, in " << fn << std::endl;
          std::cerr << line << std::endl;
          std::cerr << tokens << std::endl;
        }
      } else if (key.substr(2) == "V_AH") {
        if (chkTokens(tokens, 2, line, argv[i])) {
          const std::string voltage(key.substr(0,2));
          nc.putVar(vars.varNum("VOLTS_" + voltage + "V"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum("AMPS_" + voltage + "V"), i, mkNum(tokens[1]));
        }
      } else if (key == "SPEED_LIMITS") {
        if (chkTokens(tokens, 2, line, argv[i])) {
          nc.putVar(vars.varNum("SPEED_HORZ_MIN"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum("SPEED_HORZ_MAX"), i, mkNum(tokens[1]));
        }
      } else if (key == "RECOV_CODE") {
        if (chkTokens(tokens, 1, line, argv[i])) {
          tStringIndices::const_iterator it(stringIndices.find(tokens[0]));
          if (it == stringIndices.end()) {
            nc.putVar(vars.varNum(key), i, (double) stringIndices.size());
            nc.putVar(vars.varNum("STRINGS"), stringIndices.size(), tokens[0]);
            stringIndices.insert(std::make_pair(tokens[0], stringIndices.size()));
          } else {
            nc.putVar(vars.varNum(key), i, (double) it->second);
          }
        }
      } else if (key == "TGT_NAME") {
        if (chkTokens(tokens, 1, line, argv[i]))
          nc.putVar(vars.varNum(key), i, tokens[0]);
      } else if (key == "TGT_LATLONG") {
        if (chkTokens(tokens, 2, line, argv[i])) {
          nc.putVar(vars.varNum("TGT_LAT"), i, mkDegrees(tokens[0]));
          nc.putVar(vars.varNum("TGT_LON"), i, mkDegrees(tokens[1]));
        }
      } else if (key == "KALMAN_CONTROL") {
        if (chkTokens(tokens, 2, line, argv[i])) {
          nc.putVar(vars.varNum(key + "_V"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum(key + "_U"), i, mkNum(tokens[1]));
        }
      } else if ((key == "KALMAN_X") || (key == "KALMAN_Y")) {
        if (chkTokens(tokens, 5, line, argv[i])) {
          nc.putVar(vars.varNum(key + "_MEAN"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum(key + "_DIURNAL"), i, mkNum(tokens[1]));
          nc.putVar(vars.varNum(key + "_SEMIDIURNAL"), i, mkNum(tokens[2]));
          nc.putVar(vars.varNum(key + "_SPEED"), i, mkNum(tokens[3]));
          nc.putVar(vars.varNum(key + "_DISPLACEMENT"), i, mkNum(tokens[4]));
        }
      } else if (key == "MHEAD_RNG_PITCHd_Wd") {
        if (chkTokens(tokens, 4, line, argv[i])) {
          nc.putVar(vars.varNum("TGT_HEADING"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum("TGT_RANGE"), i, mkNum(tokens[1]));
          nc.putVar(vars.varNum("TGT_PITCH"), i, mkNum(tokens[2]));
          nc.putVar(vars.varNum("TGT_W"), i, mkNum(tokens[3]));
        }
      } else if (key == "FINISH") {
        if (chkTokens(tokens, 2, line, argv[i])) {
          nc.putVar(vars.varNum("FINISH_DEPTH"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum("FINISH_DENSITY"), i, mkNum(tokens[1]));
        }
      } else if (key == "SM_CCo") {
        if (chkTokens(tokens, 7, line, argv[i])) {
          nc.putVar(vars.varNum("SM_TIME"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum("SM_PUMP_TIME"), i, mkNum(tokens[1]));
          nc.putVar(vars.varNum("SM_AMPS"), i, mkNum(tokens[2]));
          nc.putVar(vars.varNum("SM_RETRIES"), i, mkNum(tokens[3]));
          nc.putVar(vars.varNum("SM_ERRORS"), i, mkNum(tokens[4]));
          nc.putVar(vars.varNum("SM_POSITION_COUNTS"), i, mkNum(tokens[5]));
          nc.putVar(vars.varNum("SM_POSITION"), i, mkNum(tokens[6]));
        }
      } else if (key == "SM_GC") {
        // std::cout << tokens.size() << " " << line << std::endl; // TPW
      } else if (key == "IRIDIUM_FIX") {
        if (chkTokens(tokens, 4, line, argv[i])) {
          nc.putVar(vars.varNum(key + "_LAT"), i, mkDegrees(tokens[0]));
          nc.putVar(vars.varNum(key + "_LON"), i, mkDegrees(tokens[1]));
          nc.putVar(vars.varNum(key + "_TIME"), i, mkGPSTime(tokens[2], tokens[3]));
        }
      } else if (key == "DEVICES") {
        devTimes.resize(tokens.size(), -1);
        devCurrent.resize(tokens.size(), -1);

        for (dbd::tTokens::size_type j(0), je(tokens.size()); j < je; ++j) {
          devTimes[j] = vars.varNum("DEV_" + tokens[j] + "_TIME");
          devCurrent[j] = vars.varNum("DEV_" + tokens[j] + "_CURRENT");
        }
      } else if (key == "DEVICE_SECS") { 
        if (chkTokens(tokens, devTimes.size(), line, argv[i])) {
          for (dbd::tTokens::size_type j(0), je(tokens.size()); j < je; ++j) {
            nc.putVar(devTimes[j], i, mkNum(tokens[j]));
          }
        }
      } else if (key == "DEVICE_MAMPS") {
        if (chkTokens(tokens, devCurrent.size(), line, argv[i])) {
          for (dbd::tTokens::size_type j(0), je(tokens.size()); j < je; ++j) {
            nc.putVar(devCurrent[j], i, mkNum(tokens[j]));
          }
        }
      } else if (key == "SENSORS") {
        senTimes.resize(tokens.size(), -1);
        senCurrent.resize(tokens.size(), -1);

        for (dbd::tTokens::size_type j(0), je(tokens.size()); j < je; ++j) {
          if (tokens[j] != "nil") {
            senTimes[j] = vars.varNum("SEN_" + tokens[j] + "_TIME");
            senCurrent[j] = vars.varNum("SEN_" + tokens[j] + "_CURRENT");
          }
        }
      } else if (key == "SENSOR_SECS") {
        if (chkTokens(tokens, senTimes.size(), line, argv[i])) {
          for (dbd::tTokens::size_type j(0), je(tokens.size()); j < je; ++j) {
            if (senTimes[j] != -1) {
              nc.putVar(senTimes[j], i, mkNum(tokens[j]));
            }
          }
        }
      } else if (key == "SENSOR_MAMPS") {
        if (chkTokens(tokens, senCurrent.size(), line, argv[i])) {
          for (dbd::tTokens::size_type j(0), je(tokens.size()); j < je; ++j) {
            if (senCurrent[j] != -1) {
              nc.putVar(senCurrent[j], i, mkNum(tokens[j]));
            }
          }
        }
      } else if (key == "DATA_FILE_SIZE") {
        if (chkTokens(tokens, 2, line, argv[i])) {
          nc.putVar(vars.varNum("DATA_FILE_SIZE"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum("DATA_FILE_FREE"), i, mkNum(tokens[1]));
        }
      } else if (key == "CAP_FILE_SIZE") {
        if (chkTokens(tokens, 2, line, argv[i])) {
          nc.putVar(vars.varNum("CAP_FILE_SIZE"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum("CAP_FILE_FREE"), i, mkNum(tokens[1]));
        }
      } else if (key == "CFSIZE") {
        if (chkTokens(tokens, 2, line, argv[i])) {
          nc.putVar(vars.varNum("CFSIZE_TOTAL"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum("CFSIZE_FREE"), i, mkNum(tokens[1]));
        }
      } else if (key == "ERRORS") {
        nc.putVar(vars.varNum("ERRORS_BUFFER_OVERRUNS"), i, mkNum(tokens[0]));
        nc.putVar(vars.varNum("ERRORS_SPURIOUS_INTERRUPTS"), i, mkNum(tokens[1]));
        nc.putVar(vars.varNum("ERRORS_CF8_OPEN"), i, mkNum(tokens[2]));
        nc.putVar(vars.varNum("ERRORS_CF8_WRITE"), i, mkNum(tokens[3]));
        nc.putVar(vars.varNum("ERRORS_CF8_CLOSE"), i, mkNum(tokens[4]));
        nc.putVar(vars.varNum("ERRORS_CF8_RETRIES"), i, mkNum(tokens[5]));
        nc.putVar(vars.varNum("ERRORS_PITCH"), i, mkNum(tokens[6]));
        nc.putVar(vars.varNum("ERRORS_ROLL"), i, mkNum(tokens[7]));
        nc.putVar(vars.varNum("ERRORS_VBD"), i, mkNum(tokens[8]));
        nc.putVar(vars.varNum("ERRORS_PITCH_RETRIES"), i, mkNum(tokens[9]));
        nc.putVar(vars.varNum("ERRORS_ROLL_RETRIES"), i, mkNum(tokens[10]));
        nc.putVar(vars.varNum("ERRORS_VBD_RETRIES"), i, mkNum(tokens[11]));
        nc.putVar(vars.varNum("ERRORS_GPS"), i, mkNum(tokens[12]));
        nc.putVar(vars.varNum("ERRORS_SENSOR"), i, mkNum(tokens[13]));
      } else if ((key.substr(0,3) == "GPS") && (key != "GPS_DEVICE")) { // GPS but not GPS_DEVICE
        if (chkTokens(tokens, 8, line, argv[i])) {
          nc.putVar(vars.varNum(key + "_TIME"), i, mkGPSTime(tokens[0], tokens[1]));
          nc.putVar(vars.varNum(key + "_LAT"), i, mkDegrees(tokens[2]));
          nc.putVar(vars.varNum(key + "_LON"), i, mkDegrees(tokens[3]));
          nc.putVar(vars.varNum(key + "_TO_FIX"), i, mkNum(tokens[4]));
          nc.putVar(vars.varNum(key + "_HDOP"), i, mkNum(tokens[5]));
          nc.putVar(vars.varNum(key + "_TIME_TO_AQUIRE"), i, mkNum(tokens[6]));
          nc.putVar(vars.varNum(key + "_GotMe"), i, mkNum(tokens[7]));
        }
      } else if (key == "ALTIM_BOTTOM_PING") {
        if (chkTokens(tokens, 2, line, argv[i])) {
          nc.putVar(vars.varNum(key + "_DEPTH"), i, mkNum(tokens[0]));
          nc.putVar(vars.varNum(key + "_ALTITUDE"), i, mkNum(tokens[1]));
        }
      } else {
        if (chkTokens(tokens, 1, line, argv[i])) 
          nc.putVar(vars.varNum(key), i, mkNum(tokens[0]));
      }
    }

    nc.putVar(vars.varNum("stop_iGC"), i, (int) countGC);
    nc.putVar(vars.varNum("stop_iState"), i, (int) countState);

    if (sg.qVerbose()) {
      std::cout << "Processed " << fn << std::endl;
    }

  }

  nc.close();

  return(0);
}
