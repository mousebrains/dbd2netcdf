// Unit tests for the Sensor class
// Uses Catch2 testing framework

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Sensor.H"
#include "MyException.H"
#include <sstream>

using Catch::Approx;

TEST_CASE("Sensor parsing from string", "[sensor]") {
    SECTION("Parse valid sensor line") {
        std::string line = "s: T 0 5 4 m_depth m";
        Sensor sensor(line);

        CHECK(sensor.name() == "m_depth");
        CHECK(sensor.units() == "m");
        CHECK(sensor.size() == 4);
        CHECK(sensor.index() == 5);
        CHECK(sensor.qAvailable() == true);
        CHECK(sensor.qKeep() == true);
        CHECK(sensor.qCriteria() == true);
    }

    SECTION("Parse sensor with different sizes") {
        // 1-byte sensor
        Sensor sensor1("s: T 0 0 1 sci_flag bool");
        CHECK(sensor1.size() == 1);
        CHECK(sensor1.name() == "sci_flag");
        CHECK(sensor1.units() == "bool");

        // 2-byte sensor
        Sensor sensor2("s: T 0 1 2 short_val enum");
        CHECK(sensor2.size() == 2);

        // 8-byte sensor (double/timestamp)
        Sensor sensor8("s: T 0 2 8 m_present_time timestamp");
        CHECK(sensor8.size() == 8);
        CHECK(sensor8.units() == "timestamp");
    }

    SECTION("Parse unavailable sensor") {
        Sensor sensor("s: F 0 3 4 unused_sensor nodim");
        CHECK(sensor.qAvailable() == false);
        CHECK(sensor.name() == "unused_sensor");
    }

    SECTION("Malformed sensor line throws exception") {
        CHECK_THROWS_AS(Sensor("invalid line"), MyException);
        CHECK_THROWS_AS(Sensor("x: T 0 0 4 name units"), MyException);
        CHECK_THROWS_AS(Sensor("s: T 0 0"), MyException);  // Missing fields
    }
}

TEST_CASE("Sensor parsing from stream", "[sensor]") {
    SECTION("Parse valid sensor from stream") {
        std::istringstream iss("s: T 0 10 4 sci_water_temp degc\n");
        Sensor sensor(iss);

        CHECK(sensor.name() == "sci_water_temp");
        CHECK(sensor.units() == "degc");
        CHECK(sensor.size() == 4);
        CHECK(sensor.index() == 10);
    }
}

TEST_CASE("Sensor flags can be modified", "[sensor]") {
    Sensor sensor("s: T 0 0 4 test_sensor nodim");

    SECTION("qKeep can be set") {
        CHECK(sensor.qKeep() == true);
        sensor.qKeep(false);
        CHECK(sensor.qKeep() == false);
    }

    SECTION("qCriteria can be set") {
        CHECK(sensor.qCriteria() == true);
        sensor.qCriteria(false);
        CHECK(sensor.qCriteria() == false);
    }

    SECTION("index can be modified") {
        sensor.index(42);
        CHECK(sensor.index() == 42);
    }
}

TEST_CASE("Sensor value formatting", "[sensor]") {
    SECTION("8-byte (double) formatting uses high precision") {
        Sensor sensor("s: T 0 0 8 timestamp timestamp");
        std::string result = sensor.toStr(123456789.12345);
        // Should use %.15g format - high precision
        // Check that it contains the full integer part
        CHECK(result.find("123456789") != std::string::npos);
    }

    SECTION("4-byte (float) formatting uses medium precision") {
        Sensor sensor("s: T 0 0 4 depth m");
        std::string result = sensor.toStr(123.456789);
        // Should use %.7g format - check for reasonable precision
        CHECK((result == "123.4568" || result == "123.4567"));
    }

    SECTION("1-byte and 2-byte formatting uses integer format") {
        Sensor sensor1("s: T 0 0 1 flag bool");
        CHECK(sensor1.toStr(5.9) == "6");  // Rounded to integer

        Sensor sensor2("s: T 0 0 2 count enum");
        CHECK(sensor2.toStr(100.4) == "100");
    }
}

TEST_CASE("Sensor dump output", "[sensor]") {
    Sensor sensor("s: T 0 7 4 m_depth m");

    std::ostringstream oss;
    sensor.dump(oss);

    std::string output = oss.str();
    CHECK(output.find("s:") != std::string::npos);
    CHECK(output.find("T") != std::string::npos);
    CHECK(output.find("m_depth") != std::string::npos);
    CHECK(output.find("m") != std::string::npos);
}

TEST_CASE("Sensor stream output operator", "[sensor]") {
    Sensor sensor("s: T 0 7 4 m_depth m");

    std::ostringstream oss;
    oss << sensor;

    std::string output = oss.str();
    CHECK(output.find("7") != std::string::npos);  // index
    CHECK(output.find("4") != std::string::npos);  // size
    CHECK(output.find("m_depth") != std::string::npos);
    CHECK(output.find(" m") != std::string::npos);  // units
}
