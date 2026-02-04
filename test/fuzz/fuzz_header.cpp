// Fuzz test for Header utility functions
// This tests the robustness of Header::trim and Header::addMission

#include "Header.H"
#include <string>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Convert input to string
    std::string input(reinterpret_cast<const char*>(data), size);

    // Test trim function - should never crash
    std::string trimmed = Header::trim(input);
    (void)trimmed.size();

    // Test addMission function - should never crash
    Header::tMissions missions;
    Header::addMission(input, missions);
    (void)missions.size();

    return 0;
}
