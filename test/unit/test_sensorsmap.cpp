// Unit tests for SensorsMap — focused on the cross-CRC consistency rules
// that drive correct sensor indexing when input files carry different CRC
// groups with overlapping sensor names.

#include <catch2/catch_test_macros.hpp>
#include "SensorsMap.H"
#include "Sensors.H"
#include "Header.H"
#include "MyException.H"
#include <sstream>
#include <string>
#include <unordered_set>

namespace {
// Build a minimal DBD ASCII header with the four fields SensorsMap needs:
// a CRC identifier, the sensor count, and the non-factored flag.
std::string makeHeader(const std::string& crc, int nSensors) {
    std::ostringstream oss;
    oss << "num_ascii_tags: 4\n"
        << "total_num_sensors: " << nSensors << "\n"
        << "sensor_list_factored: 0\n"
        << "sensor_list_crc: " << crc << "\n";
    return oss.str();
}
} // namespace

TEST_CASE("SensorsMap rejects same sensor at different sizes across CRC groups",
          "[sensorsmap]") {
    SensorsMap map;

    { // First CRC group: m_depth as a 4-byte float
        std::istringstream hs(makeHeader("AAAA", 1));
        Header h(hs, "groupA");
        REQUIRE(h.crc() == "AAAA");
        REQUIRE(h.nSensors() == 1);
        std::istringstream ss("s: T 0 0 4 m_depth m\n");
        map.insert(ss, h, false);
    }

    { // Second CRC group: m_depth as an 8-byte double — a schema conflict
        std::istringstream hs(makeHeader("BBBB", 1));
        Header h(hs, "groupB");
        std::istringstream ss("s: T 0 0 8 m_depth m\n");
        map.insert(ss, h, false);
    }

    CHECK_THROWS_AS(map.setUpForData(), MyException);
}

TEST_CASE("SensorsMap merges overlapping sensors into a single index space",
          "[sensorsmap]") {
    SensorsMap map;

    // Group A carries m_depth and m_roll.
    {
        std::istringstream hs(makeHeader("AAAA", 2));
        Header h(hs, "groupA");
        std::istringstream ss(
            "s: T 0 0 4 m_depth m\n"
            "s: T 0 1 4 m_roll rad\n");
        map.insert(ss, h, false);
    }

    // Group B carries m_depth (shared) and m_pitch (new).
    {
        std::istringstream hs(makeHeader("BBBB", 2));
        Header h(hs, "groupB");
        std::istringstream ss(
            "s: T 0 0 4 m_depth m\n"
            "s: T 0 1 4 m_pitch rad\n");
        map.insert(ss, h, false);
    }

    REQUIRE_NOTHROW(map.setUpForData());

    const Sensors& all(map.allSensors());
    REQUIRE(all.size() == 3);

    std::unordered_set<std::string> names;
    for (Sensors::size_type i(0); i < all.size(); ++i) {
        names.insert(all[i].name());
    }
    CHECK(names.count("m_depth") == 1);
    CHECK(names.count("m_roll") == 1);
    CHECK(names.count("m_pitch") == 1);
}

TEST_CASE("SensorsMap with a single CRC group assigns sequential indices",
          "[sensorsmap]") {
    SensorsMap map;

    std::istringstream hs(makeHeader("CCCC", 3));
    Header h(hs, "solo");
    std::istringstream ss(
        "s: T 0 0 4 m_depth m\n"
        "s: T 0 1 4 m_roll rad\n"
        "s: T 0 2 8 m_present_time timestamp\n");
    map.insert(ss, h, false);

    REQUIRE_NOTHROW(map.setUpForData());

    const Sensors& all(map.allSensors());
    CHECK(all.size() == 3);
    CHECK(all.nToStore() == 3);
}
