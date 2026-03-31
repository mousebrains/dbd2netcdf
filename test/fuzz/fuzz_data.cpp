// Fuzz test for Data binary record parsing
// Tests robustness of Data::load against malformed binary input

#include "Data.H"
#include "KnownBytes.H"
#include "Sensors.H"
#include "Sensor.H"
#include "MyException.H"
#include <sstream>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 16) return 0; // Need enough for KnownBytes + at least one record

    std::string input(reinterpret_cast<const char*>(data), size);
    std::istringstream iss(input, std::ios_base::in | std::ios_base::binary);

    try {
        // Build a minimal sensor list from the first few bytes
        const size_t nSensors = (data[0] % 4) + 1; // 1-4 sensors
        Sensors sensors;
        for (size_t i = 0; i < nSensors; ++i) {
            const int sizeVal = (1 << (i % 4)); // 1, 2, 4, 8
            std::ostringstream line;
            line << "s: T " << i << " " << i << " " << sizeVal << " sensor" << i << " units";
            sensors.insert(Sensor(line.str()));
        }
        sensors.nToStore(nSensors);

        // Skip past the bytes we used for sensor config
        iss.seekg(1, std::ios::beg);

        KnownBytes kb(iss);
        Data result;
        result.load(iss, kb, sensors, true, size);

        // Exercise accessors
        (void)result.size();
        (void)result.empty();
        if (!result.empty() && result.nColumns() > 0) {
            (void)result(0, 0);
        }
    } catch (const MyException&) {
        // Expected for malformed input
    } catch (const std::exception&) {
        // Other standard exceptions are also acceptable
    }

    return 0;
}
