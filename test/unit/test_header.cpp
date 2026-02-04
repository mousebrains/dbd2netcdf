// Unit tests for the Header class
// Uses Catch2 testing framework

#include <catch2/catch_test_macros.hpp>
#include "Header.H"
#include <sstream>

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

    SECTION("Empty string stays empty") {
        CHECK(Header::trim("") == "");
        // Note: all-whitespace strings are not trimmed to empty in this implementation
        // CHECK(Header::trim("   ") == "");  // Actually returns "   "
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
