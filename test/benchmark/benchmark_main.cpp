// Performance benchmarks for dbd2netcdf
// Uses Catch2's benchmarking feature

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch_all.hpp>
#include "Sensor.H"
#include "Sensors.H"
#include "Header.H"
#include "MyException.H"
#include <sstream>
#include <string>
#include <vector>

// Sample sensor lines for benchmarking
const std::vector<std::string> SAMPLE_SENSOR_LINES = {
    "s: T 0 0 4 m_depth m",
    "s: T 0 1 8 m_present_time timestamp",
    "s: T 0 2 4 m_roll rad",
    "s: T 0 3 4 m_pitch rad",
    "s: T 0 4 8 m_gps_lat lat",
    "s: T 0 5 8 m_gps_lon lon",
    "s: T 0 6 4 sci_water_temp degc",
    "s: T 0 7 4 sci_water_pressure bar",
    "s: T 0 8 1 sci_flag bool",
    "s: F 0 9 4 unused_sensor nodim"
};

// Sample strings for trim benchmarking
const std::vector<std::string> TRIM_SAMPLES = {
    "hello",
    "  hello",
    "hello  ",
    "  hello  ",
    "\t\thello\t\t",
    "   this is a longer string with spaces   ",
    "",
    "no_whitespace_here"
};

TEST_CASE("Sensor parsing benchmark", "[benchmark][sensor]") {
    BENCHMARK("Parse single sensor line from string") {
        return Sensor(SAMPLE_SENSOR_LINES[0]);
    };

    BENCHMARK("Parse 10 sensor lines from string") {
        std::vector<Sensor> sensors;
        sensors.reserve(10);
        for (const auto& line : SAMPLE_SENSOR_LINES) {
            sensors.emplace_back(line);
        }
        return sensors.size();
    };

    BENCHMARK("Parse sensor from stream") {
        std::istringstream iss(SAMPLE_SENSOR_LINES[0] + "\n");
        return Sensor(iss);
    };
}

TEST_CASE("Sensor toStr benchmark", "[benchmark][sensor]") {
    Sensor sensor4("s: T 0 0 4 depth m");
    Sensor sensor8("s: T 0 0 8 time timestamp");
    Sensor sensor1("s: T 0 0 1 flag bool");

    BENCHMARK("toStr 4-byte float") {
        return sensor4.toStr(123.456789);
    };

    BENCHMARK("toStr 8-byte double") {
        return sensor8.toStr(1234567890.123456);
    };

    BENCHMARK("toStr 1-byte integer") {
        return sensor1.toStr(42);
    };
}

TEST_CASE("Header::trim benchmark", "[benchmark][header]") {
    BENCHMARK("Trim short string with whitespace") {
        return Header::trim("  hello  ");
    };

    BENCHMARK("Trim long string with whitespace") {
        return Header::trim("   this is a longer string with spaces on both sides   ");
    };

    BENCHMARK("Trim string without whitespace") {
        return Header::trim("no_whitespace");
    };

    BENCHMARK("Trim multiple strings") {
        std::vector<std::string> results;
        results.reserve(TRIM_SAMPLES.size());
        for (const auto& s : TRIM_SAMPLES) {
            results.push_back(Header::trim(s));
        }
        return results.size();
    };
}

TEST_CASE("Header::addMission benchmark", "[benchmark][header]") {
    BENCHMARK("Add single mission") {
        Header::tMissions missions;
        Header::addMission("mariner-2024-322-0-0", missions);
        return missions.size();
    };

    BENCHMARK("Add 10 missions") {
        Header::tMissions missions;
        for (int i = 0; i < 10; i++) {
            Header::addMission("mission-2024-" + std::to_string(i) + "-0-0", missions);
        }
        return missions.size();
    };
}

TEST_CASE("Sensors container operations benchmark", "[benchmark][sensors]") {
    BENCHMARK("Build Sensors collection with 10 sensors") {
        Sensors sensors;
        for (const auto& line : SAMPLE_SENSOR_LINES) {
            sensors.insert(Sensor(line));
        }
        return sensors.size();
    };

    // Pre-build a sensors collection for filtering benchmarks
    Sensors sensors;
    for (const auto& line : SAMPLE_SENSOR_LINES) {
        sensors.insert(Sensor(line));
    }

    BENCHMARK("qKeep with 3 matching names") {
        Sensors s = sensors;  // Copy
        Sensors::tNames names;
        names.insert("m_depth");
        names.insert("m_roll");
        names.insert("m_pitch");
        s.qKeep(names);
        return s.nToStore();
    };

    BENCHMARK("qCriteria with 2 matching names") {
        Sensors s = sensors;  // Copy
        Sensors::tNames names;
        names.insert("m_depth");
        names.insert("sci_water_temp");
        s.qCriteria(names);
        return s.size();
    };
}
