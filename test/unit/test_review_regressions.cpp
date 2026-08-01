// Regression tests for defects found by the adversarial code review.
//
// Each test below fails against the code as it was before the accompanying
// fix. They are grouped here rather than spread across the per-class files so
// the link back to the review stays obvious; move them if a file outgrows this.

#include <catch2/catch_test_macros.hpp>
#include "Header.H"
#include "Sensors.H"
#include "MyException.H"
#include <sstream>
#include <string>

// ---------------------------------------------------------------- Header

TEST_CASE("Header::trim strips carriage returns", "[header][regression]") {
    // A CRLF header otherwise leaves '\r' on every value. sensor_list_crc is
    // one of those values and becomes the sensor cache filename, so the stray
    // byte forked the cache by line ending.
    CHECK(Header::trim("ab12\r") == "ab12");
    CHECK(Header::trim("\r\nab12\r\n") == "ab12");
    CHECK(Header::trim("\r") == "");
    CHECK(Header::trim(" \r\n\t") == "");
}

TEST_CASE("Header parse loop is bounded by lines, not by map size",
          "[header][regression]") {
    // mRecords is a std::map, so a repeated key does not grow it. When the loop
    // was bounded on mRecords.size() a header with duplicate keys never reached
    // num_ascii_tags and kept consuming lines past the ASCII header into the
    // binary sensor block.
    std::ostringstream oss;
    oss << "num_ascii_tags: 5\n"
        << "dupe: one\n"
        << "dupe: two\n"
        << "dupe: three\n"
        << "sensor_list_crc: ab12\n";
    const std::string trailing("BINARY-SHOULD-NOT-BE-READ\n");
    const std::string payload(oss.str() + trailing);

    std::istringstream is(payload);
    const Header hdr(is, "dupe.sbd");

    // Exactly the 5 declared header lines are consumed, so the very next thing
    // readable from the stream is the binary block, untouched.
    std::string next;
    REQUIRE(std::getline(is, next));
    CHECK(next == "BINARY-SHOULD-NOT-BE-READ");

    // A duplicate key keeps its first value, and the later keys still parse.
    CHECK(hdr.find("dupe") == "one");
    CHECK(hdr.find("sensor_list_crc") == "ab12");
}

// ---------------------------------------------------------------- Sensors

TEST_CASE("Sensors::safeCRC rejects crc values that escape the cache directory",
          "[sensors][regression][security]") {
    // sensor_list_crc is ASCII content from a third-party file and is used to
    // build a path under -C. fs::path's operator/ REPLACES the base when the
    // right-hand side is absolute, so an absolute value escaped without any
    // "../" being involved.
    const auto crcOf = [](const std::string& crc) {
        std::ostringstream oss;
        oss << "num_ascii_tags: 3\n"
            << "sensor_list_factored: 1\n"
            << "sensor_list_crc: " << crc << "\n";
        std::istringstream is(oss.str());
        const Header hdr(is, "crc.sbd");
        return Sensors(is, hdr); // factored, so no sensor lines are consumed
    };

    SECTION("A normal hex CRC is accepted and lowercased") {
        CHECK(crcOf("AB12cd").safeCRC() == "ab12cd");
    }

    SECTION("Traversal and absolute paths are rejected") {
        CHECK_THROWS_AS(crcOf("../../etc/passwd").safeCRC(), MyException);
        CHECK_THROWS_AS(crcOf("/tmp/pwned").safeCRC(), MyException);
        CHECK_THROWS_AS(crcOf("..").safeCRC(), MyException);
        CHECK_THROWS_AS(crcOf("a/b").safeCRC(), MyException);
    }

    SECTION("An empty CRC is rejected rather than yielding a bare .cac") {
        CHECK_THROWS_AS(crcOf("").safeCRC(), MyException);
    }
}
