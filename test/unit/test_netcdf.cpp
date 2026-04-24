// Unit tests for NetCDF schema-validation paths in maybeCreateDim /
// maybeCreateVar. The integration suite only exercises the happy reopen path
// (--append on a schema-compatible file); these tests drive the negative
// branches that append mode and batch mode rely on to fail loudly.

#include <catch2/catch_test_macros.hpp>
#include "MyNetCDF.H"
#include "MyException.H"
#include <netcdf.h>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace {
// Produce a temp path the caller is responsible for removing. We avoid a
// fixed name so parallel test runs on the same machine do not collide.
std::string tempNcPath(const std::string& stem) {
    std::mt19937 rng{std::random_device{}()};
    const auto suffix = std::to_string(rng());
    return (fs::temp_directory_path() /
            ("dbd2netcdf_test_" + stem + "_" + suffix + ".nc")).string();
}

struct ScopedFile {
    std::string path;
    explicit ScopedFile(std::string p) : path(std::move(p)) {}
    ~ScopedFile() {
        std::error_code ec;
        fs::remove(path, ec);
    }
    ScopedFile(const ScopedFile&) = delete;
    ScopedFile& operator=(const ScopedFile&) = delete;
};
} // namespace

TEST_CASE("NetCDF::maybeCreateVar round-trips on a compatible reopen",
          "[netcdf][schema]") {
    ScopedFile file(tempNcPath("roundtrip"));

    int initialVar = -1;
    {
        NetCDF nc(file.path, false);
        const int dim = nc.maybeCreateDim("i");
        initialVar = nc.maybeCreateVar("x", NC_DOUBLE, dim, "m");
    }
    {
        NetCDF nc(file.path, true);
        const int dim = nc.maybeCreateDim("i");
        CHECK_NOTHROW(nc.maybeCreateVar("x", NC_DOUBLE, dim, "m"));
        (void)initialVar;
    }
}

TEST_CASE("NetCDF::maybeCreateVar rejects a type mismatch",
          "[netcdf][schema]") {
    ScopedFile file(tempNcPath("type_mismatch"));

    {
        NetCDF nc(file.path, false);
        const int dim = nc.maybeCreateDim("i");
        nc.maybeCreateVar("x", NC_DOUBLE, dim, "m");
    }

    NetCDF nc(file.path, true);
    const int dim = nc.maybeCreateDim("i");
    CHECK_THROWS_AS(nc.maybeCreateVar("x", NC_FLOAT, dim, "m"), MyException);
}

TEST_CASE("NetCDF::maybeCreateVar rejects a wrong dimension id",
          "[netcdf][schema]") {
    ScopedFile file(tempNcPath("dim_id_mismatch"));

    {
        NetCDF nc(file.path, false);
        const int dimI = nc.maybeCreateDim("i");
        nc.maybeCreateDim("j"); // distinct dim so swapping ids matters
        nc.maybeCreateVar("x", NC_DOUBLE, dimI, "m");
    }

    NetCDF nc(file.path, true);
    nc.maybeCreateDim("i");
    const int dimJ = nc.maybeCreateDim("j");
    // 'x' is attached to dim 'i' in the existing file, so passing dimJ must
    // trip the dim-id mismatch branch.
    CHECK_THROWS_AS(nc.maybeCreateVar("x", NC_DOUBLE, dimJ, "m"), MyException);
}

TEST_CASE("NetCDF::maybeCreateVar rejects a wrong dimension count",
          "[netcdf][schema]") {
    ScopedFile file(tempNcPath("ndims_mismatch"));

    {
        NetCDF nc(file.path, false);
        const int dimI = nc.maybeCreateDim("i");
        const int dimJ = nc.maybeCreateDim("j");
        const int dims[] = {dimI, dimJ};
        nc.createVar("x2", NC_DOUBLE, dims, 2, "m");
    }

    NetCDF nc(file.path, true);
    const int dimI = nc.maybeCreateDim("i");
    nc.maybeCreateDim("j");
    CHECK_THROWS_AS(nc.maybeCreateVar("x2", NC_DOUBLE, dimI, "m"), MyException);
}

TEST_CASE("NetCDF::maybeCreateDim rejects a fixed-length existing dim",
          "[netcdf][schema]") {
    ScopedFile file(tempNcPath("fixed_dim"));

    // The NetCDF class only exposes unlimited-dim creation, so drop to the
    // raw nc_* API to set up this precondition.
    {
        int ncid = -1;
        REQUIRE(nc_create(file.path.c_str(), NC_NETCDF4 | NC_CLOBBER, &ncid) == 0);
        int dimid = -1;
        REQUIRE(nc_def_dim(ncid, "i", 10, &dimid) == 0);
        REQUIRE(nc_close(ncid) == 0);
    }

    NetCDF nc(file.path, true);
    CHECK_THROWS_AS(nc.maybeCreateDim("i"), MyException);
}
