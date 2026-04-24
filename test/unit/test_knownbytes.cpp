// Unit tests for the KnownBytes class

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "KnownBytes.H"
#include "MyException.H"
#include <sstream>
#include <cstdint>
#include <cstring>

using Catch::Approx;

namespace {
    // Build a valid 16-byte "known bytes" block in native byte order.
    // Because the first three bytes are "sa" followed by int16_t(0x1234),
    // the KnownBytes parser sees native-endian data and sets mFlip=false.
    std::string makeNativeKnownBytes() {
        std::ostringstream oss;
        oss.put('s');
        oss.put('a');
        const int16_t tag = 0x1234;
        oss.write(reinterpret_cast<const char*>(&tag), 2);
        const float f = 123.456f;
        oss.write(reinterpret_cast<const char*>(&f), 4);
        const double d = 123456789.12345;
        oss.write(reinterpret_cast<const char*>(&d), 8);
        return oss.str();
    }
}

TEST_CASE("KnownBytes constructor validates the 16-byte header", "[knownbytes]") {
    SECTION("Valid native-order header succeeds") {
        const std::string block = makeNativeKnownBytes();
        std::istringstream iss(block, std::ios::binary);
        CHECK_NOTHROW(KnownBytes(iss));
    }

    SECTION("Wrong tag byte throws") {
        std::string block = makeNativeKnownBytes();
        block[0] = 'x';
        std::istringstream iss(block, std::ios::binary);
        CHECK_THROWS_AS(KnownBytes(iss), MyException);
    }

    SECTION("Wrong second byte throws") {
        std::string block = makeNativeKnownBytes();
        block[1] = 'b';
        std::istringstream iss(block, std::ios::binary);
        CHECK_THROWS_AS(KnownBytes(iss), MyException);
    }

    SECTION("Corrupted int16 throws") {
        std::string block = makeNativeKnownBytes();
        block[2] = 0x00;
        block[3] = 0x00;
        std::istringstream iss(block, std::ios::binary);
        CHECK_THROWS_AS(KnownBytes(iss), MyException);
    }

    SECTION("Truncated stream throws") {
        std::istringstream iss("sa", std::ios::binary);
        CHECK_THROWS_AS(KnownBytes(iss), MyException);
    }

    SECTION("Length is always 16") {
        const std::string block = makeNativeKnownBytes();
        std::istringstream iss(block, std::ios::binary);
        KnownBytes kb(iss);
        CHECK(kb.length() == 16);
    }
}

TEST_CASE("KnownBytes read methods round-trip native-order values", "[knownbytes]") {
    // Construct a valid kb first, then use it to read from an arbitrary stream.
    const std::string header = makeNativeKnownBytes();
    std::istringstream headerStream(header, std::ios::binary);
    KnownBytes kb(headerStream);

    SECTION("read8") {
        std::ostringstream oss;
        const int8_t sent = -42;
        oss.write(reinterpret_cast<const char*>(&sent), 1);
        std::istringstream iss(oss.str(), std::ios::binary);
        CHECK(kb.read8(iss) == sent);
    }

    SECTION("read16") {
        std::ostringstream oss;
        const int16_t sent = -12345;
        oss.write(reinterpret_cast<const char*>(&sent), 2);
        std::istringstream iss(oss.str(), std::ios::binary);
        CHECK(kb.read16(iss) == sent);
    }

    SECTION("read32 float") {
        std::ostringstream oss;
        const float sent = 3.14159f;
        oss.write(reinterpret_cast<const char*>(&sent), 4);
        std::istringstream iss(oss.str(), std::ios::binary);
        CHECK(kb.read32(iss) == Approx(sent));
    }

    SECTION("read64 double") {
        std::ostringstream oss;
        const double sent = -2.718281828459045;
        oss.write(reinterpret_cast<const char*>(&sent), 8);
        std::istringstream iss(oss.str(), std::ios::binary);
        CHECK(kb.read64(iss) == Approx(sent));
    }

    SECTION("read8 on empty stream throws") {
        std::istringstream iss("", std::ios::binary);
        CHECK_THROWS_AS(kb.read8(iss), MyException);
    }

    SECTION("read16 on short stream throws") {
        std::istringstream iss("x", std::ios::binary);
        CHECK_THROWS_AS(kb.read16(iss), MyException);
    }
}
