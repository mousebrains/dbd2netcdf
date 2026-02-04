# Fuzz Testing

This directory contains fuzz tests for the dbd2netcdf project using LLVM's libFuzzer.

## Requirements

- Clang compiler with libFuzzer support (typically Clang 6.0+)
- AddressSanitizer and UndefinedBehaviorSanitizer support

## Building

Fuzz tests are disabled by default. To build them:

```bash
mkdir build && cd build
cmake -DBUILD_FUZZ_TESTS=ON -DCMAKE_CXX_COMPILER=clang++ ..
make fuzz_sensor fuzz_header fuzz_knownbytes
```

## Running

Each fuzz target can be run with a corpus directory:

```bash
# Run sensor parser fuzzer for 60 seconds
./bin/fuzz_sensor ../test/fuzz/corpus/sensor -max_total_time=60

# Run header utilities fuzzer for 60 seconds
./bin/fuzz_header ../test/fuzz/corpus/header -max_total_time=60

# Run KnownBytes binary parser fuzzer for 60 seconds
./bin/fuzz_knownbytes ../test/fuzz/corpus/knownbytes -max_total_time=60
```

### Common Options

- `-max_total_time=N`: Run for N seconds
- `-jobs=N`: Run N parallel workers
- `-workers=N`: Number of concurrent fuzzers
- `-dict=FILE`: Use dictionary file
- `-help=1`: Show all options

## Fuzz Targets

### fuzz_sensor
Tests the Sensor class constructor that parses sensor definition lines.
Exercises both string and stream constructors.

### fuzz_header
Tests Header utility functions:
- `Header::trim()`: Whitespace trimming
- `Header::addMission()`: Mission name handling

### fuzz_knownbytes
Tests the KnownBytes class which handles binary byte-order detection and reading.
This is a critical parser for DBD files as it validates the 16-byte "known bytes"
block that determines endianness for all subsequent binary data.

## Corpus

Seed corpus files are provided in `corpus/` subdirectories:
- `corpus/sensor/`: Sample sensor definition lines
- `corpus/header/`: Sample header strings
- `corpus/knownbytes/`: Sample 16-byte binary blocks

## Crash Reproduction

When the fuzzer finds a crash, it saves the input to a file. To reproduce:

```bash
./bin/fuzz_sensor crash-HASH
```
