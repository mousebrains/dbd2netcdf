// Fuzz test for KnownBytes binary parsing
// This tests the robustness of byte-order detection and binary reading

#include "KnownBytes.H"
#include "MyException.H"
#include <sstream>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Need at least 16 bytes for a valid KnownBytes block
    if (size < 16) {
        return 0;
    }

    // Convert input to stream
    std::string input(reinterpret_cast<const char*>(data), size);
    std::istringstream iss(input);

    try {
        // Test constructor - validates known bytes block
        KnownBytes kb(iss);

        // If constructor succeeds, test the read methods with remaining data
        try {
            (void)kb.read8(iss);
        } catch (const MyException&) {
            // Expected if stream exhausted
        }

        try {
            (void)kb.read16(iss);
        } catch (const MyException&) {
            // Expected if stream exhausted
        }

        try {
            (void)kb.read32(iss);
        } catch (const MyException&) {
            // Expected if stream exhausted
        }

        try {
            (void)kb.read64(iss);
        } catch (const MyException&) {
            // Expected if stream exhausted
        }
    } catch (const MyException&) {
        // Expected for malformed input - KnownBytes validates strictly
    } catch (const std::exception&) {
        // Other standard exceptions are also acceptable
    }

    return 0;
}
