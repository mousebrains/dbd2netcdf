# dbd2netcdf Architecture

This document describes the architecture of the dbd2netcdf project, which converts Dinkum Binary Data (DBD) files from Slocum gliders to NetCDF or CSV format.

## Overview

The project consists of several command-line tools that share a common library (`dbd_common`):

```
┌─────────────────────────────────────────────────────────────────┐
│                      Command-Line Tools                         │
├──────────────┬──────────────┬──────────────┬───────────────────┤
│  dbd2netCDF  │   dbd2csv    │  dbdSensors  │   pd02netCDF      │
│  (DBD→NC)    │  (DBD→CSV)   │  (list sens) │   (PD0→NC)        │
├──────────────┴──────────────┴──────────────┴───────────────────┤
│                    Core Library (dbd_common)                    │
├──────────────┬──────────────┬──────────────┬───────────────────┤
│    Header    │   Sensors    │    Data      │   KnownBytes      │
│  (file hdr)  │ (sensor def) │  (data rec)  │   (byte order)    │
├──────────────┴──────────────┴──────────────┼───────────────────┤
│                SensorsMap                   │    Decompress     │
│           (sensor caching/mapping)          │    (LZ4 files)    │
└─────────────────────────────────────────────┴───────────────────┘
```

## DBD File Format

Dinkum Binary Data files consist of:

1. **ASCII Header** - Key-value pairs describing the file (14 lines by default)
2. **Sensor Definitions** - List of sensors with name, size, units
3. **Known Bytes Block** - 16 bytes for byte-order detection
4. **Binary Data Records** - Variable-length records with sensor values

### File Types

| Extension | Description |
|-----------|-------------|
| `.dbd`    | Dive data (science bay) |
| `.sbd`    | Surface data |
| `.tbd`    | Trajectory data |
| `.ecd`    | Extended data |
| `.dcd`    | Dive controller data |
| `.scd`    | Surface controller data |
| `.tcd`    | Trajectory controller data |

## Core Classes

### Header

Parses the ASCII header section of DBD files.

```cpp
class Header {
    // Stores key-value pairs from header
    tRecords mRecords;  // map<string, string>

    // Key methods:
    string find(key);      // Get header value
    int findInt(key);      // Get integer value
    string crc();          // Sensor list CRC
    int nSensors();        // Total sensor count
};
```

### Sensor

Represents a single sensor definition.

```cpp
class Sensor {
    string mName;      // Sensor name (e.g., "m_depth")
    string mUnits;     // Units (e.g., "m")
    int mSize;         // Bytes: 1, 2, 4, or 8
    int mIndex;        // Position in data array
    bool mqAvailable;  // Present in data stream
    bool mqKeep;       // Include in output
    bool mqCriteria;   // Selection criteria
};
```

### Sensors

Collection of Sensor objects with filtering and caching.

```cpp
class Sensors {
    vector<Sensor> mSensors;
    string mCRC;           // From header
    size_t mLength;        // Bytes in sensor block
    size_t mnToStore;      // Sensors to output

    // Caching methods:
    bool dump(dir);        // Save to cache file
    bool load(dir, hdr);   // Load from cache

    // Filtering:
    void qKeep(names);     // Select output sensors
    void qCriteria(names); // Select filter sensors
};
```

### SensorsMap

Manages sensors across multiple files with different sensor configurations.

```cpp
class SensorsMap {
    string mDir;              // Cache directory
    map<string, Sensors> mMap;  // CRC → Sensors
    Sensors mAllSensors;      // Union of all sensors

    // Combines sensors from all files into unified output
    void setUpForData();
};
```

### KnownBytes

Handles byte-order detection and binary reading.

```cpp
class KnownBytes {
    bool mFlip;  // Need byte swapping?

    // Read methods handle endianness:
    int8_t read8(is);
    int16_t read16(is);
    float read32(is);
    double read64(is);
};
```

### Data

Loads and stores binary data records.

```cpp
class Data {
    vector<vector<double>> mData;  // [record][sensor]

    void load(is, kb, sensors, qRepair, nBytes);
};
```

### Decompress

Handles LZ4-compressed files transparently.

```cpp
class DecompressTWR {
    // Wraps istream for transparent decompression
    // Works with both compressed and uncompressed files
};
```

## Data Flow

### dbd2netCDF

```
┌─────────────┐
│ Input Files │ (.dbd, .sbd, .tbd, etc.)
└──────┬──────┘
       │
       ▼
┌──────────────────┐
│  First Pass      │ Read headers, collect sensor definitions
│  (SensorsMap)    │ Load/create sensor cache files
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│  setUpForData()  │ Create unified sensor list from all files
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│  Create NetCDF   │ Define dimensions, variables, attributes
│  Structure       │
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│  Second Pass     │ Read binary data, write to NetCDF
│  (Data → NetCDF) │ Handle sensor mapping between files
└──────┬───────────┘
       │
       ▼
┌─────────────┐
│ Output .nc  │
└─────────────┘
```

## Sensor Caching

The sensor cache avoids re-parsing sensor definitions:

1. Each unique sensor configuration has a CRC
2. Cache files stored as `{crc}.cac` in cache directory
3. Speeds up processing of multiple files with same sensors

## Error Handling

- `MyException`: Custom exception class for domain errors
- Strict mode (`-S`): Fail immediately on any error
- Normal mode: Log warnings, skip problematic files/records

## Logging

Uses spdlog for structured logging:

- Log levels: trace, debug, info, warn, error, critical
- Default level: warn
- Set via `-l/--log-level` option

## Thread Safety

The code is not thread-safe. Files should be processed sequentially.

## Testing

```
test/
├── unit/           # Catch2 unit tests
│   ├── test_sensor.cpp
│   ├── test_sensors.cpp
│   └── test_header.cpp
├── fuzz/           # libFuzzer fuzz tests
│   ├── fuzz_sensor.cpp
│   └── fuzz_header.cpp
├── benchmark/      # Catch2 benchmarks
│   └── benchmark_main.cpp
└── data/           # Real test data files
    └── 00300000.*
```

## Dependencies

- **CLI11**: Command-line parsing
- **spdlog**: Logging
- **NetCDF-C**: NetCDF file writing
- **LZ4**: Decompression (bundled)
- **Catch2**: Testing (optional)
