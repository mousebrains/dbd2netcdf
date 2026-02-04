# Claude Code Project Notes

Project-specific context for Claude Code when working on dbd2netcdf.

## Project Overview

dbd2netcdf converts Dinkum Binary Data (DBD) files from Slocum gliders to NetCDF or CSV format. See [doc/ARCHITECTURE.md](doc/ARCHITECTURE.md) for detailed architecture documentation.

## MSVC Compatibility

When adding new code, watch for these MSVC warnings (treated as errors with /WX):

| Warning | Issue | Fix |
|---------|-------|-----|
| C4267 | size_t to int conversion | `static_cast<int>()` |
| C4244 | int to smaller type (uint8_t, float) | explicit casts |
| C4456 | Variable shadowing | rename inner variables |
| C4996 | strerror deprecation | `_CRT_SECURE_NO_WARNINGS` is defined |
| C4127 | Constant conditional | use `static_assert` for compile-time checks |

## CI Configuration

- **Linux**: GCC 9+14, Clang 16+19 (oldest/newest strategy)
- **macOS**: macos-latest + macos-26 (beta)
- **Containers**: AlmaLinux 8 (GCC 8.5), Rocky Linux 9
- **Windows**: MSVC via conda-forge for NetCDF
- **Fuzz testing**: Weekly via `.github/workflows/fuzz.yml`

## Pre-commit Hooks

Files excluded from trailing-whitespace hook (ncdump outputs trailing spaces):
- `*.netCDF`, `*.ncdump`, `*.csv`
- Binary glider files: `*.sbd`, `*.tbd`, `*.dbd`, `*.ebd`, `*.mbd`, `*.scd`, `*.tcd`, `*.dcd`, `*.ecd`, `*.mcd`

## Dependencies (via FetchContent)

- CLI11 v2.6.1
- spdlog v1.15.0
- Catch2 v3.7.1

## Test Data

- Real Slocum glider data in `test/data/` (00300000.*)
- Reference files regenerated with ncdump must preserve trailing whitespace

## Fuzz Testing

- Requires Clang with libFuzzer (`-DBUILD_FUZZ_TESTS=ON`)
- Targets: `fuzz_sensor`, `fuzz_header`, `fuzz_knownbytes`

## C++20 Migration Notes

When dropping GCC 8.x support, consider:
- `std::span` for buffer handling in `Decompress.C`, `KnownBytes.C`
- `std::format` to replace spdlog's fmt
