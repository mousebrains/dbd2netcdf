# Performance Benchmarks

This directory contains performance benchmarks for the dbd2netcdf project using Catch2's benchmarking feature.

## Building

Benchmarks are disabled by default. To build them:

```bash
mkdir build && cd build
cmake -DBUILD_BENCHMARKS=ON ..
make benchmarks
```

## Running

Run all benchmarks:
```bash
./bin/benchmarks
```

Run specific benchmark:
```bash
./bin/benchmarks "[sensor]"
./bin/benchmarks "[header]"
./bin/benchmarks "[sensors]"
```

### Options

- `--benchmark-samples=N`: Number of samples per benchmark (default: 100)
- `--benchmark-warmup-time=N`: Warmup time in milliseconds (default: 100)
- `--benchmark-confidence-interval=N`: Confidence level (default: 0.95)

## Benchmarks

### Sensor Parsing
- Parse single sensor line from string
- Parse multiple sensor lines
- Parse sensor from stream

### Sensor toStr
- Format 4-byte float values
- Format 8-byte double values
- Format 1-byte integer values

### Header Utilities
- `Header::trim()` with various inputs
- `Header::addMission()` for mission list management

### Sensors Container
- Building sensor collections
- Filtering with qKeep()
- Filtering with qCriteria()

## Using with CMake Target

```bash
make run_benchmarks
```

This runs benchmarks with 100 samples automatically.
