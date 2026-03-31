// Fuzz test for LZ4 decompression stream
// Tests robustness of DecompressTWRBuf against malformed compressed data

#include "Decompress.H"
#include "MyException.H"
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <string>
#include <unistd.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    // Write fuzz input to a temporary file (DecompressTWR requires a file)
    char tmpname[] = "/tmp/fuzz_decompress_XXXXXX";
    int fd = mkstemp(tmpname);
    if (fd < 0) return 0;

    FILE* f = fdopen(fd, "wb");
    if (!f) { close(fd); unlink(tmpname); return 0; }
    fwrite(data, 1, size, f);
    fclose(f);

    try {
        // Test compressed path
        DecompressTWR is(tmpname, true);
        char buf[4096];
        while (is.read(buf, sizeof(buf)) || is.gcount()) {
            // Just consume the data
        }
    } catch (const MyException&) {
        // Expected for malformed input
    } catch (const std::exception&) {
        // Other standard exceptions are also acceptable
    }

    try {
        // Test uncompressed path
        DecompressTWR is(tmpname, false);
        char buf[4096];
        while (is.read(buf, sizeof(buf)) || is.gcount()) {
            // Just consume the data
        }
    } catch (const std::exception&) {
        // Acceptable
    }

    unlink(tmpname);
    return 0;
}
