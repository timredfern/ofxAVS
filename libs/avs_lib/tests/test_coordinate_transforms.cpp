// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/coordinate_lookup_table.h"
#include <cmath>

using namespace avs;

TEST_CASE("Polar coordinate script execution", "[coordinate][polar][transform]") {
    CoordinateLookupTable table;
    AudioData dummy_audio = {};
    const int width = 64;
    const int height = 64;
    const int grid_w = 8;
    const int grid_h = 8;

    SECTION("Empty script executes without error") {
        table.generate(width, height, grid_w, grid_h,
                      "", "",
                      false, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(0, 0);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("Script modifying d executes") {
        table.generate(width, height, grid_w, grid_h,
                      "d=d*0.9", "d=d*0.9",
                      false, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(0, 0);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("Script modifying r executes") {
        table.generate(width, height, grid_w, grid_h,
                      "r=r+0.5", "r=r+0.5",
                      false, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(0, 0);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("Script modifying both d and r executes") {
        table.generate(width, height, grid_w, grid_h,
                      "d=d*0.9; r=r+d*0.5", "d=d*0.9; r=r+d*0.5",
                      false, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(3, 3);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("Variable x is accessible in polar mode") {
        // Script that reads x - should not error
        table.generate(width, height, grid_w, grid_h,
                      "d=d+x*0.01", "d=d+x*0.01",
                      false, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(0, 0);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("Variable y is accessible in polar mode") {
        // Script that reads y - should not error
        table.generate(width, height, grid_w, grid_h,
                      "d=d+y*0.01", "d=d+y*0.01",
                      false, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(0, 0);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("All four variables (x, y, d, r) accessible together") {
        // Script using all variables
        table.generate(width, height, grid_w, grid_h,
                      "d=d*0.9+x*0.01+y*0.01; r=r+0.1", "d=d*0.9+x*0.01+y*0.01; r=r+0.1",
                      false, false, dummy_audio, false, InterpolationMode::LINEAR);

        // Check multiple grid points produce valid output
        for (int gy = 0; gy < grid_h; gy++) {
            for (int gx = 0; gx < grid_w; gx++) {
                auto coords = table.get_interpolated_coordinates(gx, gy);
                REQUIRE(std::isfinite(coords.first));
                REQUIRE(std::isfinite(coords.second));
            }
        }
    }

    SECTION("Math functions work in scripts") {
        table.generate(width, height, grid_w, grid_h,
                      "d=d*abs(sin(r))", "d=d*abs(sin(r))",
                      false, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(2, 2);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }
}

TEST_CASE("Rectangular coordinate script execution", "[coordinate][rectangular][transform]") {
    CoordinateLookupTable table;
    AudioData dummy_audio = {};
    const int width = 64;
    const int height = 64;
    const int grid_w = 8;
    const int grid_h = 8;

    SECTION("Identity script executes") {
        table.generate(width, height, grid_w, grid_h,
                      "x=x; y=y", "x=x; y=y",
                      true, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(0, 0);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("Shift script executes") {
        table.generate(width, height, grid_w, grid_h,
                      "x=x-0.1; y=y-0.1", "x=x-0.1; y=y-0.1",
                      true, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(4, 4);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("Scale script executes") {
        table.generate(width, height, grid_w, grid_h,
                      "x=x*0.5; y=y*0.5", "x=x*0.5; y=y*0.5",
                      true, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(4, 4);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("Wave effect using sin executes") {
        table.generate(width, height, grid_w, grid_h,
                      "x=x+sin(y*6.28)*0.05", "x=x+sin(y*6.28)*0.05",
                      true, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto coords = table.get_interpolated_coordinates(4, 4);
        REQUIRE(std::isfinite(coords.first));
        REQUIRE(std::isfinite(coords.second));
    }

    SECTION("All grid points produce valid output") {
        table.generate(width, height, grid_w, grid_h,
                      "x=x*0.9; y=y*0.9", "x=x*0.9; y=y*0.9",
                      true, false, dummy_audio, false, InterpolationMode::LINEAR);

        for (int gy = 0; gy < grid_h; gy++) {
            for (int gx = 0; gx < grid_w; gx++) {
                auto coords = table.get_interpolated_coordinates(gx, gy);
                REQUIRE(std::isfinite(coords.first));
                REQUIRE(std::isfinite(coords.second));
            }
        }
    }
}

TEST_CASE("Coordinate transform mode switching", "[coordinate][transform]") {
    CoordinateLookupTable table;
    AudioData dummy_audio = {};
    const int width = 64;
    const int height = 64;
    const int grid_w = 8;
    const int grid_h = 8;

    SECTION("Can switch between polar and rectangular modes") {
        // First generate in polar mode
        table.generate(width, height, grid_w, grid_h,
                      "d=d*0.9", "d=d*0.9",
                      false, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto polar_coords = table.get_interpolated_coordinates(2, 2);
        REQUIRE(std::isfinite(polar_coords.first));

        // Then regenerate in rectangular mode
        table.generate(width, height, grid_w, grid_h,
                      "x=x*0.9; y=y*0.9", "x=x*0.9; y=y*0.9",
                      true, false, dummy_audio, false, InterpolationMode::LINEAR);

        auto rect_coords = table.get_interpolated_coordinates(2, 2);
        REQUIRE(std::isfinite(rect_coords.first));
    }
}
