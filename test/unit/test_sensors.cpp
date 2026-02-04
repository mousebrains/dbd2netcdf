// Unit tests for the Sensors class
// Uses Catch2 testing framework

#include <catch2/catch_test_macros.hpp>
#include "Sensors.H"
#include "Sensor.H"
#include <sstream>

TEST_CASE("Sensors container operations", "[sensors]") {
    Sensors sensors;

    SECTION("Empty sensors collection") {
        CHECK(sensors.empty() == true);
        CHECK(sensors.size() == 0);
    }

    SECTION("Insert and access sensors") {
        Sensor sensor1("s: T 0 0 4 m_depth m");
        Sensor sensor2("s: T 0 1 8 m_present_time timestamp");

        sensors.insert(sensor1);
        CHECK(sensors.size() == 1);
        CHECK(sensors.empty() == false);

        sensors.insert(sensor2);
        CHECK(sensors.size() == 2);

        // Access by index
        CHECK(sensors[0].name() == "m_depth");
        CHECK(sensors[1].name() == "m_present_time");
    }

    SECTION("Clear sensors") {
        sensors.insert(Sensor("s: T 0 0 4 test_sensor nodim"));
        CHECK(sensors.size() == 1);

        sensors.clear();
        CHECK(sensors.empty() == true);
        CHECK(sensors.size() == 0);
    }

    SECTION("Iterator access") {
        sensors.insert(Sensor("s: T 0 0 4 sensor_a nodim"));
        sensors.insert(Sensor("s: T 0 1 4 sensor_b nodim"));
        sensors.insert(Sensor("s: T 0 2 4 sensor_c nodim"));

        int count = 0;
        for (auto it = sensors.begin(); it != sensors.end(); ++it) {
            count++;
        }
        CHECK(count == 3);
    }
}

TEST_CASE("Sensors qKeep filtering", "[sensors]") {
    Sensors sensors;
    sensors.insert(Sensor("s: T 0 0 4 m_depth m"));
    sensors.insert(Sensor("s: T 0 1 4 m_roll rad"));
    sensors.insert(Sensor("s: T 0 2 4 m_pitch rad"));

    SECTION("Filter to specific sensor names") {
        Sensors::tNames namesToKeep;
        namesToKeep.insert("m_depth");
        namesToKeep.insert("m_pitch");

        sensors.qKeep(namesToKeep);

        CHECK(sensors[0].qKeep() == true);   // m_depth - in list
        CHECK(sensors[1].qKeep() == false);  // m_roll - not in list
        CHECK(sensors[2].qKeep() == true);   // m_pitch - in list
    }

    SECTION("Empty filter sets all sensors to not keep") {
        Sensors::tNames emptyNames;
        sensors.qKeep(emptyNames);

        // With empty list, qKeep is set to false for all
        CHECK(sensors[0].qKeep() == false);
        CHECK(sensors[1].qKeep() == false);
        CHECK(sensors[2].qKeep() == false);
    }
}

TEST_CASE("Sensors qCriteria filtering", "[sensors]") {
    Sensors sensors;
    sensors.insert(Sensor("s: T 0 0 4 m_depth m"));
    sensors.insert(Sensor("s: T 0 1 4 m_roll rad"));

    SECTION("Set criteria sensors") {
        Sensors::tNames criteriaNames;
        criteriaNames.insert("m_depth");

        sensors.qCriteria(criteriaNames);

        CHECK(sensors[0].qCriteria() == true);   // m_depth - in list
        CHECK(sensors[1].qCriteria() == false);  // m_roll - not in list
    }
}

TEST_CASE("Sensors nToStore tracking", "[sensors]") {
    Sensors sensors;

    SECTION("Default nToStore is 0") {
        CHECK(sensors.nToStore() == 0);
    }

    SECTION("nToStore can be set") {
        sensors.nToStore(42);
        CHECK(sensors.nToStore() == 42);
    }
}

TEST_CASE("Sensors stream output", "[sensors]") {
    Sensors sensors;
    sensors.insert(Sensor("s: T 0 0 4 m_depth m"));
    sensors.insert(Sensor("s: T 0 1 8 m_present_time timestamp"));

    std::ostringstream oss;
    oss << sensors;

    std::string output = oss.str();
    CHECK(output.find("m_depth") != std::string::npos);
    CHECK(output.find("m_present_time") != std::string::npos);
}
