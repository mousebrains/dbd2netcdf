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
make fuzz_sensor fuzz_header
```

## Running

Each fuzz target can be run with a corpus directory:

```bash
# Run sensor parser fuzzer for 60 seconds
./bin/fuzz_sensor ../test/fuzz/corpus/sensor -max_total_time=60

# Run header utilities fuzzer for 60 seconds
./bin/fuzz_header ../test/fuzz/corpus/header -max_total_time=60
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

## Corpus

Seed corpus files are provided in `corpus/` subdirectories:
- `corpus/sensor/`: Sample sensor definition lines
- `corpus/header/`: Sample header strings

## Crash Reproduction

When the fuzzer finds a crash, it saves the input to a file. To reproduce:

```bash
./bin/fuzz_sensor crash-HASH
```
