// Regression tests for defects found by the adversarial code review.
//
// Each test below fails against the code as it was before the accompanying
// fix. They are grouped here rather than spread across the per-class files so
// the link back to the review stays obvious; move them if a file outgrows this.

#include <catch2/catch_test_macros.hpp>
#include "Header.H"
#include "Sensors.H"
#include "Decompress.H"
#include "MyException.H"
#include "lz4.h"
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
// Same convention as test_netcdf.cpp: a unique path the caller owns.
std::string tempPath(const std::string& stem, const std::string& ext) {
    std::mt19937 rng{std::random_device{}()};
    return (fs::temp_directory_path() /
            ("dbd2netcdf_test_" + stem + "_" + std::to_string(rng()) + ext)).string();
}

struct ScopedPath {
    std::string path;
    explicit ScopedPath(std::string p) : path(std::move(p)) {}
    ~ScopedPath() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    ScopedPath(const ScopedPath&) = delete;
    ScopedPath& operator=(const ScopedPath&) = delete;
};

// Append one TWR-framed LZ4 block: a 2-byte big-endian compressed length
// followed by that many bytes of LZ4 block data.
void appendFrame(std::ofstream& os, const std::string& payload) {
    std::vector<char> compressed(static_cast<size_t>(
        LZ4_compressBound(static_cast<int>(payload.size()))) + 1);
    const int n = LZ4_compress_default(payload.data(), compressed.data(),
                                       static_cast<int>(payload.size()),
                                       static_cast<int>(compressed.size()));
    REQUIRE(n > 0);
    const unsigned char hdr[2] = {
        static_cast<unsigned char>((n >> 8) & 0xff),
        static_cast<unsigned char>(n & 0xff)};
    os.write(reinterpret_cast<const char*>(hdr), sizeof(hdr));
    os.write(compressed.data(), n);
}
} // namespace

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

// ------------------------------------------------------------- Decompress

TEST_CASE("DecompressTWR reports an open failure at construction",
          "[decompress][regression]") {
    // Before the fix the istream stayed good until the first read, so the
    // seven `if (!is)` checks that follow construction across the four tools
    // were all dead and a missing file was silently treated as empty.
    const std::string missing =
        (fs::temp_directory_path() / "dbd2netcdf_definitely_absent_file.sbd").string();
    REQUIRE_FALSE(fs::exists(missing));

    DecompressTWR is(missing, false);
    CHECK_FALSE(static_cast<bool>(is));
    CHECK(is.fail());
}

TEST_CASE("DecompressTWR opens a file that exists", "[decompress][regression]") {
    // Guards against the check above being satisfied by a stream that is
    // always in a failed state.
    ScopedPath file(tempPath("plain", ".sbd"));
    {
        std::ofstream os(file.path, std::ios::binary);
        os << "hello";
    }

    DecompressTWR is(file.path, false);
    REQUIRE(static_cast<bool>(is));
    std::string got;
    is >> got;
    CHECK(got == "hello");
}

TEST_CASE("DecompressTWR skips a zero-length LZ4 block",
          "[decompress][regression]") {
    // LZ4_compress_default of an empty input yields a valid block that
    // decompresses to zero bytes. underflow() used to setg() an empty get area
    // and still return a character, which breaks the streambuf contract:
    // uflow() then gbump()s past egptr() and later reads run off mBuffer.
    ScopedPath file(tempPath("emptyblock", ".scd"));
    {
        std::ofstream os(file.path, std::ios::binary);
        appendFrame(os, "");            // decompresses to nothing
        appendFrame(os, "first-half;"); // real payload
        appendFrame(os, "");            // and again, mid-stream
        appendFrame(os, "second-half");
    }

    DecompressTWR is(file.path, true);
    REQUIRE(static_cast<bool>(is));

    std::ostringstream all;
    all << is.rdbuf();
    CHECK(all.str() == "first-half;second-half");
}

TEST_CASE("DecompressTWR stops cleanly on a corrupt LZ4 block",
          "[decompress][regression]") {
    ScopedPath file(tempPath("badblock", ".scd"));
    {
        std::ofstream os(file.path, std::ios::binary);
        appendFrame(os, "good;");
        // A length header promising 8 bytes of LZ4 data that is not valid LZ4.
        const unsigned char hdr[2] = {0x00, 0x08};
        os.write(reinterpret_cast<const char*>(hdr), sizeof(hdr));
        os.write("\xff\xff\xff\xff\xff\xff\xff\xff", 8);
    }

    DecompressTWR is(file.path, true);
    REQUIRE(static_cast<bool>(is));

    std::ostringstream all;
    all << is.rdbuf();
    // The good block is delivered; the corrupt one ends the stream rather than
    // throwing or looping.
    CHECK(all.str() == "good;");
}

TEST_CASE("DecompressTWR stops cleanly on a truncated LZ4 frame",
          "[decompress][regression]") {
    ScopedPath file(tempPath("truncated", ".scd"));
    {
        std::ofstream os(file.path, std::ios::binary);
        appendFrame(os, "good;");
        // Header promises 32 bytes; supply 4.
        const unsigned char hdr[2] = {0x00, 0x20};
        os.write(reinterpret_cast<const char*>(hdr), sizeof(hdr));
        os.write("\x01\x02\x03\x04", 4);
    }

    DecompressTWR is(file.path, true);
    std::ostringstream all;
    all << is.rdbuf();
    CHECK(all.str() == "good;");
}

// ------------------------------------------------- Sensors cache integration

TEST_CASE("Sensors::mkFilename routes the CRC through safeCRC",
          "[sensors][regression][security]") {
    // safeCRC() is tested directly above, but that does not prove mkFilename
    // actually uses it. Without this, the path-traversal fix could be reverted
    // at the call site with every other test still green.
    ScopedPath dir(tempPath("cachedir", ""));
    REQUIRE(fs::create_directories(dir.path));

    const auto sensorsFor = [](const std::string& crc) {
        std::ostringstream oss;
        oss << "num_ascii_tags: 3\n"
            << "sensor_list_factored: 1\n"
            << "sensor_list_crc: " << crc << "\n";
        std::istringstream is(oss.str());
        const Header hdr(is, "crc.sbd");
        return Sensors(is, hdr);
    };

    SECTION("A well-formed CRC yields a path inside the cache directory") {
        const std::string got = sensorsFor("AB12").mkFilename(dir.path);
        CHECK(fs::path(got).parent_path() == fs::path(dir.path));
        CHECK(fs::path(got).filename().string() == "ab12.cac");
    }

    SECTION("An absolute CRC is rejected instead of escaping the directory") {
        // fs::path::operator/ REPLACES the base when the right side is
        // absolute, so this escaped with no "../" involved.
        CHECK_THROWS_AS(sensorsFor("/tmp/pwned").mkFilename(dir.path), MyException);
    }

    SECTION("A traversal CRC is rejected") {
        CHECK_THROWS_AS(sensorsFor("../../etc/passwd").mkFilename(dir.path),
                        MyException);
    }
}

TEST_CASE("Sensors::load rejects a corrupt sensor cache",
          "[sensors][regression]") {
    // Sensor positions index the per-cycle state bitmap, so skipping a corrupt
    // line shifted every later sensor and decoded the file into the wrong
    // columns. Failing loudly is the only safe recovery.
    ScopedPath dir(tempPath("badcache", ""));
    REQUIRE(fs::create_directories(dir.path));

    std::ostringstream hdrText;
    hdrText << "num_ascii_tags: 3\n"
            << "sensor_list_factored: 1\n"
            << "sensor_list_crc: dead\n";
    std::istringstream hdrStream(hdrText.str());
    const Header hdr(hdrStream, "cache.sbd");

    {
        std::ofstream os((fs::path(dir.path) / "dead.cac").string());
        os << "s: T 0 0 4 sensor_one nodim\n"
           << "this is not a sensor line\n"
           << "s: T 2 2 4 sensor_three nodim\n";
    }

    Sensors sensors;
    CHECK_THROWS_AS(sensors.load(dir.path, hdr), MyException);
}

TEST_CASE("Sensors::load accepts a well-formed sensor cache",
          "[sensors][regression]") {
    // Companion to the test above: proves the throw is specific to corruption
    // rather than the cache path being broken outright.
    ScopedPath dir(tempPath("goodcache", ""));
    REQUIRE(fs::create_directories(dir.path));

    std::ostringstream hdrText;
    hdrText << "num_ascii_tags: 3\n"
            << "sensor_list_factored: 1\n"
            << "sensor_list_crc: beef\n";
    std::istringstream hdrStream(hdrText.str());
    const Header hdr(hdrStream, "cache.sbd");

    {
        std::ofstream os((fs::path(dir.path) / "beef.cac").string());
        os << "s: T 0 0 4 sensor_one nodim\n"
           << "s: T 1 1 4 sensor_two nodim\n";
    }

    Sensors sensors;
    REQUIRE(sensors.load(dir.path, hdr));
    CHECK(sensors.size() == 2);
}
