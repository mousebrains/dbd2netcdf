// Unit tests for the Header class
// Uses Catch2 testing framework

#include <catch2/catch_test_macros.hpp>
#include "Header.H"
#include <sstream>
#include <limits>
#include <ctime>

TEST_CASE("Header::trim removes whitespace", "[header]") {
    SECTION("Trim leading whitespace") {
        CHECK(Header::trim("   hello") == "hello");
        CHECK(Header::trim("\t\tworld") == "world");
    }

    SECTION("Trim trailing whitespace") {
        CHECK(Header::trim("hello   ") == "hello");
        CHECK(Header::trim("world\t\t") == "world");
    }

    SECTION("Trim both leading and trailing whitespace") {
        CHECK(Header::trim("  hello world  ") == "hello world");
        CHECK(Header::trim("\t mixed \t") == "mixed");
    }

    SECTION("Empty and all-whitespace strings return empty") {
        CHECK(Header::trim("") == "");
        CHECK(Header::trim("   ") == "");
        CHECK(Header::trim("\t\n\t") == "");
    }

    SECTION("String without whitespace unchanged") {
        CHECK(Header::trim("hello") == "hello");
    }
}

TEST_CASE("Header mission filtering", "[header]") {
    SECTION("addMission stores lowercased mission name") {
        Header::tMissions missions;

        // Add a mission - stored as-is (lowercased)
        Header::addMission("test-2024-001-0-0", missions);
        CHECK(missions.count("test-2024-001-0-0") == 1);

        // Add another mission
        Header::addMission("Mariner-2024-322-0-0", missions);
        CHECK(missions.count("mariner-2024-322-0-0") == 1);  // Lowercased
        CHECK(missions.size() == 2);
    }

    SECTION("addMission lowercases mission names") {
        Header::tMissions missions;

        // Mixed case should be lowercased
        Header::addMission("TestMission", missions);
        CHECK(missions.count("testmission") == 1);

        // All caps should be lowercased
        Header::addMission("UPPERCASE", missions);
        CHECK(missions.count("uppercase") == 1);
    }
}

TEST_CASE("Header::parseFileOpenTime", "[header]") {
    SECTION("Parses valid fileopen_time string") {
        // Tue_Sep_20_17:39:01_2011 => 2011-09-20 17:39:01 UTC
        time_t t = Header::parseFileOpenTime("Tue_Sep_20_17:39:01_2011");
        REQUIRE(t != std::numeric_limits<time_t>::max());
        struct tm* utc = gmtime(&t);
        CHECK(utc->tm_year + 1900 == 2011);
        CHECK(utc->tm_mon + 1 == 9);
        CHECK(utc->tm_mday == 20);
        CHECK(utc->tm_hour == 17);
        CHECK(utc->tm_min == 39);
        CHECK(utc->tm_sec == 1);
    }

    SECTION("Parses another valid timestamp") {
        // Mon_Nov_18_09:40:04_2024
        time_t t = Header::parseFileOpenTime("Mon_Nov_18_09:40:04_2024");
        REQUIRE(t != std::numeric_limits<time_t>::max());
        struct tm* utc = gmtime(&t);
        CHECK(utc->tm_year + 1900 == 2024);
        CHECK(utc->tm_mon + 1 == 11);
        CHECK(utc->tm_mday == 18);
    }

    SECTION("Ordering: earlier time < later time") {
        time_t t1 = Header::parseFileOpenTime("Tue_Sep_20_17:39:01_2011");
        time_t t2 = Header::parseFileOpenTime("Mon_Nov_18_09:40:04_2024");
        REQUIRE(t1 < t2);
    }

    SECTION("Empty string returns max sentinel") {
        CHECK(Header::parseFileOpenTime("") == std::numeric_limits<time_t>::max());
    }

    SECTION("Garbage string returns max sentinel") {
        CHECK(Header::parseFileOpenTime("not_a_timestamp") == std::numeric_limits<time_t>::max());
    }
}
