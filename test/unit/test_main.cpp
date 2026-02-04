// Catch2 test runner main
// This file provides the main() function for all unit tests

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    return Catch::Session().run(argc, argv);
}
