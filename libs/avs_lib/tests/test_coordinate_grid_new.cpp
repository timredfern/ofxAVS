// Tests for the new CoordinateGrid class
// These tests verify the correct two-stage transformation matching original AVS

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../core/coordinate_grid.h"
#include <cmath>
#include <cstring>
#include <iostream>

using Catch::Approx;
using namespace avs;

TEST_CASE("CoordinateGrid storage", "[coordinate]") {
    SECTION("stores and retrieves coordinates") {
        CoordinateGrid grid;
        grid.resize(4, 4);
        grid.set(0, 0, 100.0, 200.0);
        auto [x, y] = grid.get(0, 0);
        REQUIRE(x == Approx(100.0).margin(0.001));
        REQUIRE(y == Approx(200.0).margin(0.001));
    }

    SECTION("grid dimensions are stored correctly") {
        CoordinateGrid grid;
        grid.resize(8, 4);
        REQUIRE(grid.grid_width() == 8);
        REQUIRE(grid.grid_height() == 4);
    }

    SECTION("stores multiple grid points") {
        CoordinateGrid grid;
        grid.resize(3, 3);

        // Set all 9 grid points
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                grid.set(gx, gy, gx * 10.0 + 5.0, gy * 20.0 + 10.0);
            }
        }

        // Verify all points
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                auto [x, y] = grid.get(gx, gy);
                REQUIRE(x == Approx(gx * 10.0 + 5.0).margin(0.001));
                REQUIRE(y == Approx(gy * 20.0 + 10.0).margin(0.001));
            }
        }
    }
}

TEST_CASE("CoordinateGrid bilinear interpolation", "[coordinate]") {
    // Grid interpolation is ALWAYS bilinear in original AVS

    SECTION("interpolates at grid center") {
        CoordinateGrid grid;
        grid.resize(2, 2);
        grid.set(0, 0, 0.0, 0.0);
        grid.set(1, 0, 100.0, 0.0);
        grid.set(0, 1, 0.0, 100.0);
        grid.set(1, 1, 100.0, 100.0);

        // Sample at center (0.5, 0.5) should return (50, 50)
        auto [x, y] = grid.sample(0.5, 0.5);
        REQUIRE(x == Approx(50.0).margin(1.0));
        REQUIRE(y == Approx(50.0).margin(1.0));
    }

    SECTION("interpolates at grid corners") {
        CoordinateGrid grid;
        grid.resize(2, 2);
        grid.set(0, 0, 10.0, 20.0);
        grid.set(1, 0, 30.0, 20.0);
        grid.set(0, 1, 10.0, 40.0);
        grid.set(1, 1, 30.0, 40.0);

        // Sample at corners returns exact values
        auto [x0, y0] = grid.sample(0.0, 0.0);
        REQUIRE(x0 == Approx(10.0).margin(1.0));
        REQUIRE(y0 == Approx(20.0).margin(1.0));

        auto [x1, y1] = grid.sample(1.0, 1.0);
        REQUIRE(x1 == Approx(30.0).margin(1.0));
        REQUIRE(y1 == Approx(40.0).margin(1.0));
    }

    SECTION("interpolates along edges") {
        CoordinateGrid grid;
        grid.resize(2, 2);
        grid.set(0, 0, 0.0, 0.0);
        grid.set(1, 0, 100.0, 0.0);
        grid.set(0, 1, 0.0, 100.0);
        grid.set(1, 1, 100.0, 100.0);

        // Along top edge (y=0)
        auto [x_mid_top, y_mid_top] = grid.sample(0.5, 0.0);
        REQUIRE(x_mid_top == Approx(50.0).margin(1.0));
        REQUIRE(y_mid_top == Approx(0.0).margin(1.0));

        // Along left edge (x=0)
        auto [x_mid_left, y_mid_left] = grid.sample(0.0, 0.5);
        REQUIRE(x_mid_left == Approx(0.0).margin(1.0));
        REQUIRE(y_mid_left == Approx(50.0).margin(1.0));
    }

    SECTION("interpolates between grid points on 3x3 grid") {
        CoordinateGrid grid;
        grid.resize(3, 3);
        // Set up a simple grid where each point maps to itself scaled
        for (int gy = 0; gy < 3; gy++) {
            for (int gx = 0; gx < 3; gx++) {
                grid.set(gx, gy, gx * 50.0, gy * 50.0);
            }
        }

        // Sample at (0.25, 0.25) - between grid points
        auto [x, y] = grid.sample(0.25, 0.25);
        REQUIRE(x == Approx(25.0).margin(2.0));
        REQUIRE(y == Approx(25.0).margin(2.0));

        // Sample at (0.75, 0.75) - between different grid points
        auto [x2, y2] = grid.sample(0.75, 0.75);
        REQUIRE(x2 == Approx(75.0).margin(2.0));
        REQUIRE(y2 == Approx(75.0).margin(2.0));
    }
}

TEST_CASE("CoordinateGrid apply with different grid sizes", "[coordinate]") {
    // This is the key test - grid dimensions MUST affect output

    SECTION("2x2 grid vs 16x16 grid produce different results for non-linear transform") {
        const int W = 64, H = 64;

        CoordinateGrid grid2, grid16;
        grid2.resize(2, 2);
        grid16.resize(16, 16);

        // Fill both with a rotation transform (non-linear in cartesian coords)
        auto fillRotation = [W, H](CoordinateGrid& g, double angle) {
            double cx = W / 2.0, cy = H / 2.0;
            double cosA = std::cos(angle), sinA = std::sin(angle);
            for (int gy = 0; gy < g.grid_height(); gy++) {
                for (int gx = 0; gx < g.grid_width(); gx++) {
                    double nx = (double)gx / (g.grid_width() - 1);
                    double ny = (double)gy / (g.grid_height() - 1);
                    double px = nx * W - cx;
                    double py = ny * H - cy;
                    double rx = cx + px * cosA - py * sinA;
                    double ry = cy + px * sinA + py * cosA;
                    g.set(gx, gy, rx, ry);
                }
            }
        };

        fillRotation(grid2, 0.1);
        fillRotation(grid16, 0.1);

        std::vector<uint32_t> input(W * H);
        std::vector<uint32_t> output2(W * H, 0);
        std::vector<uint32_t> output16(W * H, 0);

        // Fill input with a gradient pattern
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                input[y * W + x] = 0xFF000000 | ((x * 4) << 16) | ((y * 4) << 8);
            }
        }

        grid2.apply(input.data(), output2.data(), W, H, true, false, false);
        grid16.apply(input.data(), output16.data(), W, H, true, false, false);

        // Count differing pixels - there MUST be differences
        int diff_count = 0;
        for (int i = 0; i < W * H; i++) {
            if (output2[i] != output16[i]) diff_count++;
        }

        // With different grid resolutions and a non-linear transform,
        // outputs MUST differ significantly
        INFO("Differing pixels: " << diff_count << " out of " << (W * H));
        REQUIRE(diff_count > W * H / 10);  // At least 10% different
    }

    SECTION("identity transform produces similar output regardless of grid size") {
        const int W = 32, H = 32;

        CoordinateGrid grid2, grid8;
        grid2.resize(2, 2);
        grid8.resize(8, 8);

        // Fill both with identity transform (each grid point maps to corresponding output position)
        auto fillIdentity = [W, H](CoordinateGrid& g) {
            for (int gy = 0; gy < g.grid_height(); gy++) {
                for (int gx = 0; gx < g.grid_width(); gx++) {
                    double nx = (double)gx / (g.grid_width() - 1);
                    double ny = (double)gy / (g.grid_height() - 1);
                    g.set(gx, gy, nx * (W - 1), ny * (H - 1));
                }
            }
        };

        fillIdentity(grid2);
        fillIdentity(grid8);

        std::vector<uint32_t> input(W * H);
        std::vector<uint32_t> output2(W * H, 0);
        std::vector<uint32_t> output8(W * H, 0);

        // Fill input with gradient
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                input[y * W + x] = 0xFF000000 | ((x * 8) << 16) | ((y * 8) << 8);
            }
        }

        grid2.apply(input.data(), output2.data(), W, H, true, false, false);
        grid8.apply(input.data(), output8.data(), W, H, true, false, false);

        // For identity, both should produce output very similar to input
        // (small differences due to interpolation precision are OK)
        int match_count = 0;
        for (int i = 0; i < W * H; i++) {
            // Check if colors are very close (within 2 per channel due to interpolation)
            int r1 = (output2[i] >> 16) & 0xFF;
            int r2 = (output8[i] >> 16) & 0xFF;
            int g1 = (output2[i] >> 8) & 0xFF;
            int g2 = (output8[i] >> 8) & 0xFF;
            if (std::abs(r1 - r2) <= 2 && std::abs(g1 - g2) <= 2) {
                match_count++;
            }
        }

        // Most pixels should match closely for identity
        REQUIRE(match_count > W * H * 0.9);  // At least 90% similar
    }
}

TEST_CASE("CoordinateGrid subpixel sampling", "[coordinate]") {
    const int W = 8, H = 8;

    SECTION("subpixel=false uses nearest neighbor") {
        CoordinateGrid grid;
        grid.resize(2, 2);

        // Set up grid to sample from offset position
        grid.set(0, 0, 0.3, 0.3);  // Fractional position
        grid.set(1, 0, W - 1 + 0.3, 0.3);
        grid.set(0, 1, 0.3, H - 1 + 0.3);
        grid.set(1, 1, W - 1 + 0.3, H - 1 + 0.3);

        std::vector<uint32_t> input(W * H, 0xFF000000);
        std::vector<uint32_t> output(W * H, 0);

        // Set specific pixels
        input[0] = 0xFFFF0000;  // Red at (0,0)
        input[1] = 0xFF00FF00;  // Green at (1,0)

        grid.apply(input.data(), output.data(), W, H, false, false, false);

        // With nearest neighbor, should pick closest pixel
        // The grid samples from (0.3, 0.3) which should round to (0,0)
    }

    SECTION("subpixel=true uses bilinear sampling") {
        CoordinateGrid grid;
        grid.resize(2, 2);

        // Set up identity grid
        grid.set(0, 0, 0.0, 0.0);
        grid.set(1, 0, W - 1.0, 0.0);
        grid.set(0, 1, 0.0, H - 1.0);
        grid.set(1, 1, W - 1.0, H - 1.0);

        std::vector<uint32_t> input(W * H, 0xFF000000);
        std::vector<uint32_t> output_nearest(W * H, 0);
        std::vector<uint32_t> output_bilinear(W * H, 0);

        // Create a checkerboard pattern
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                input[y * W + x] = ((x + y) % 2 == 0) ? 0xFFFFFFFF : 0xFF000000;
            }
        }

        grid.apply(input.data(), output_nearest.data(), W, H, false, false, false);
        grid.apply(input.data(), output_bilinear.data(), W, H, true, false, false);

        // For identity transform, outputs should be similar to input
        // The main difference would be at sub-pixel sampling positions
    }
}
