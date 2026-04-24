// Unit tests for Data::load — focused on edge cases that are hard to trigger
// via the integration suite but that a user can hit via CLI flag combinations.

#include <catch2/catch_test_macros.hpp>
#include "Data.H"
#include "Sensor.H"
#include "Sensors.H"
#include "KnownBytes.H"
#include "MyException.H"
#include <sstream>
#include <string>
#include <cstdint>

namespace {
// KnownBytes requires a 16-byte header that starts with "sa", carries an
// int16 tag of 0x1234 in native byte order, then a float of exactly 123.456
// and a double of exactly 123456789.12345 (validated in the ctor). We only
// need this so Data::load has a valid kb to pass around during the early
// guard it exercises here.
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
} // namespace

TEST_CASE("Data::load rejects a zero-column selection", "[data]") {
    // Build a Sensors object with a real sensor, then force nToStore to 0 to
    // simulate "--output filter matched nothing". The guard at the top of
    // Data::load() must throw before it touches mData[0].
    Sensors sensors;
    sensors.insert(Sensor("s: T 0 0 4 m_depth m"));
    sensors.nToStore(0);

    const std::string header = makeNativeKnownBytes();
    std::istringstream headerStream(header, std::ios::binary);
    KnownBytes kb(headerStream);

    // The stream contents do not matter — the guard fires before we read any
    // record bytes.
    std::istringstream dataStream("", std::ios::binary);
    Data data;
    CHECK_THROWS_AS(data.load(dataStream, kb, sensors, false, 1024), MyException);
}
