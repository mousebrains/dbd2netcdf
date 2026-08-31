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
- **macOS**: macos-15 + macos-26
- **Containers**: AlmaLinux 8 (GCC 8.5), Rocky Linux 9
- **Windows**: MSVC via conda-forge for NetCDF
- **Fuzz testing**: Weekly, plus PRs touching `src/**` or `test/fuzz/**`

Runner labels: the **oldest**-compiler jobs pin `ubuntu-24.04` because `gcc-9`
and `clang-16` come from that release's archive, not the runner image; the
**newest** ones use `ubuntu-latest` so they meet new compilers early. Name both
halves of a version matrix explicitly -- `[macos-latest, macos-26]` silently
collapsed to one image once 26 went GA.

Container images: `rockylinux/rockylinux:9`, not the Docker Official Image
`rockylinux:9`, which Rocky can no longer publish updates to (last pushed
2024-05-30). `almalinux:8` is still current.

Every job sets `timeout-minutes`, sized at roughly 3x its observed median so a
hang fails promptly instead of running to the 360-minute default. Cygwin is the
outlier at ~14 minutes warm; everything else is under 7.

## Pre-commit Hooks

Files excluded from trailing-whitespace hook (ncdump outputs trailing spaces):
- `*.netCDF`, `*.ncdump`, `*.csv`
- Binary glider files: `*.sbd`, `*.tbd`, `*.dbd`, `*.ebd`, `*.mbd`, `*.scd`, `*.tcd`, `*.dcd`, `*.ecd`, `*.mcd`

## Dependencies (via FetchContent)

- CLI11 v2.7.2
- spdlog v1.17.0
- Catch2 v3.16.0

Bumped automatically: `.github/workflows/dependency-check.yml` runs on the first
Monday of each month, rewrites the `GIT_TAG` values via
`.github/scripts/bump_fetchcontent.py`, builds and runs `ctest` against them,
and opens a pull request on the `deps/fetchcontent` branch only if that passes.
A bump that breaks the build files an issue instead.

That script also rewrites two prose files, so keep their shapes intact:

- the version list above, matched as `- <Name> <tag>`
- the ChangeLog's `Unreleased` section, where it maintains a
  `<Mon>-<YYYY>, Dependencies` block. One bullet per dependency, not per bump:
  a pin moved twice between releases has its existing bullet rewritten rather
  than duplicated, so the section reads as the net change the release ships.
  If a release cut has renamed `Unreleased` away, it recreates the heading.

`.github/scripts/test_bump_fetchcontent.py` covers the ChangeLog rewriting --
the one part that can fail quietly, since a bad `GIT_TAG` is caught by the build
the workflow runs anyway. It runs in the Shellcheck job on every PR and again
before the monthly bump.

### The `DEPS_PAT` secret

GitHub does not start workflow runs for events raised with `GITHUB_TOKEN`, so a
dependency PR opened with the default token arrives with **no checks at all**.
`DEPS_PAT` is a fine-grained personal access token used for the branch push and
the `gh pr create`, which makes the PR look like it came from a person and lets
`build-test.yml`'s `pull_request` trigger fire — full compiler matrix, CodeQL
and coverage on the bump before it merges.

Create it at Settings -> Developer settings -> Personal access tokens ->
Fine-grained tokens:

- **Repository access**: Only select repositories -> `dbd2netcdf`
- **Repository permissions**: Contents: Read and write; Pull requests: Read and
  write; Issues: Read and write (Metadata: Read is added automatically)

Then store it:

```sh
gh secret set DEPS_PAT --repo mousebrains/dbd2netcdf
```

Fine-grained tokens expire. When it does, the workflow does not break: it falls
back to `GITHUB_TOKEN`, still opens the PR, and emits a `::warning::` saying the
matrix will not run. That warning is the signal to reissue the token.

## Test Data

- Real Slocum glider data in `test/data/` (00300000.*)
- Reference files regenerated with ncdump must preserve trailing whitespace

## Fuzz Testing

- Requires Clang with libFuzzer (`-DBUILD_FUZZ_TESTS=ON`)
- Targets: `fuzz_sensor`, `fuzz_header`, `fuzz_knownbytes`, `fuzz_data`, `fuzz_decompress`

## Performance Notes

- **Data storage**: `Data` uses column-major layout (`mData[sensor][record]`) so the NetCDF write loop accesses contiguous memory per sensor.
- **NetCDF writes**: Each sensor column is written in a single `putVars` call (not split at NaN gaps) to minimize HDF5 per-call overhead (`H5Pcreate`/`H5Pclose`).
- **`--batch-size N`** (default 100): Closes and reopens the NetCDF file every N input files to release HDF5 B-tree chunk metadata (~6 KB per chunk). With 1706 sensors, each file adds ~10 MB of metadata that persists until `nc_close`. Set to 0 to disable batching.

## C++20 Migration Notes

When dropping GCC 8.x support, consider:
- `std::span` for buffer handling in `Decompress.C`, `KnownBytes.C`
- `std::format` to replace spdlog's fmt
