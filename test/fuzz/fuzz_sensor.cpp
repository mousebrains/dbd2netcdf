// Fuzz test for Sensor parsing
// This tests the robustness of Sensor line parsing against malformed input

#include "Sensor.H"
#include "MyException.H"
#include <string>
#include <sstream>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Convert input to string
    std::string input(reinterpret_cast<const char*>(data), size);

    // Test string constructor
    try {
        Sensor sensor(input);
        // If parsing succeeds, exercise the API
        (void)sensor.name();
        (void)sensor.units();
        (void)sensor.size();
        (void)sensor.index();
        (void)sensor.qAvailable();
        (void)sensor.qKeep();
        (void)sensor.qCriteria();
        (void)sensor.toStr(123.456);
    } catch (const MyException&) {
        // Expected for malformed input
    } catch (const std::exception&) {
        // Other standard exceptions are also acceptable
    }

    // Test stream constructor
    try {
        std::istringstream iss(input);
        Sensor sensor(iss);
        (void)sensor.name();
    } catch (const MyException&) {
        // Expected for malformed input
    } catch (const std::exception&) {
        // Other standard exceptions are also acceptable
    }

    return 0;
}
