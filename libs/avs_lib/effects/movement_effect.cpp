// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "movement_effect.h"
#include "core/binary_reader.h"
#include "core/parameter.h"
#include "core/plugin_manager.h"
#include "core/script/script_engine.h"
#include "core/ui.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace avs {

// Blend table for bilinear interpolation - g_blendtable[color][weight] = (color/255.0) * weight
static uint8_t g_blendtable[256][256];
static bool g_blendtable_initialized = false;

static void init_blendtable() {
    if (g_blendtable_initialized) return;
    for (int i = 0; i < 256; i++) {
        double scale = i / 255.0;
        for (int j = 0; j < 256; j++) {
            g_blendtable[i][j] = static_cast<uint8_t>(scale * j);
        }
    }
    g_blendtable_initialized = true;
}

// Preset names for UI
static const char* preset_names[] = {
    "none",
    "slight fuzzify",
    "shift rotate left",
    "big swirl out",
    "medium swirl",
    "sunburster",
    "swirl to center",
    "blocky partial out",
    "swirling around both ways at once",
    "bubbling outward",
    "bubbling outward with swirl",
    "5 pointed distro",
    "tunneling",
    "bleedin'",
    "shifted big swirl out",
    "psychotic beaming outward",
    "cosine radial 3-way",
    "spinny tube",
    "radial swirlies",
    "swill",
    "gridley",
    "grapevine",
    "quadrant",
    "6-way kaleida (use wrap!)",
    "(user defined)"
};

MovementEffect::MovementEffect()
    : table_valid_(false), subpixel_enabled_(false),
      last_width_(0), last_height_(0)
{
    init_blendtable();  // Initialize bilinear blend table
    init_parameters_from_layout(effect_info.ui_layout);

    // Add string parameter for custom expression (not in layout)
    parameters().add_parameter(std::make_shared<Parameter>("custom_expr", ParameterType::STRING,
        std::string("d = d * 0.95; r = r + 0.1")));
}


bool MovementEffect::needs_table_regeneration(int w, int h) const {
    // table_valid_ is set to false by on_parameter_changed when params change
    // We only need to check dimensions here
    return !table_valid_ || w != last_width_ || h != last_height_;
}

void MovementEffect::generate_lookup_table(int w, int h, AudioData visdata) {
    // Resize lookup table for full resolution
    lookup_table_.resize(w * h);

    int preset_index = parameters().get_int("preset", 0);
    bool rectangular = parameters().get_bool("rectangular", false);
    bool wrap = parameters().get_bool("wrap", false);
    bool subpixel = parameters().get_bool("subpixel", true);

    // Subpixel only works for certain effects and if resolution isn't too large
    // (need to fit base index in 22 bits = 4M pixels max)
    bool can_subpixel = subpixel && (w * h < (1 << 22)) &&
                        preset_index != 0 && preset_index != 1 &&
                        preset_index != 2 && preset_index != 7;
    subpixel_enabled_ = can_subpixel;

    double max_d = sqrt((double)(w*w + h*h)) / 2.0;
    double divmax_d = 1.0 / max_d;
    double w2 = w / 2.0;
    double h2 = h / 2.0;
    double xsc = 1.0 / w2;
    double ysc = 1.0 / h2;

    int p = 0;

    // Generate lookup for every pixel
    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            int ow, oh;

            // Special hardcoded effects (not script-based)
            if (preset_index == 0) {
                // None - identity transform
                lookup_table_[p] = p;
                p++;
                continue;
            }
            else if (preset_index == 1) {
                // Slight fuzzify - random pixel offset
                int r_val = p + (rand() % 3) - 1 + ((rand() % 3) - 1) * w;
                lookup_table_[p] = std::min(w * h - 1, std::max(r_val, 0));
                p++;
                continue;
            }
            else if (preset_index == 2) {
                // Shift rotate left
                int lp = w / 64;
                ow = px + lp;
                if (ow >= w) ow -= w;
                oh = py;
                lookup_table_[p] = ow + oh * w;
                p++;
                continue;
            }
            else if (preset_index == 7) {
                // Blocky partial out
                if (px & 2 || py & 2) {
                    lookup_table_[p] = px + py * w;
                } else {
                    int xp = w / 2 + (((px & ~1) - w / 2) * 7) / 8;
                    int yp = h / 2 + (((py & ~1) - h / 2) * 7) / 8;
                    lookup_table_[p] = xp + yp * w;
                }
                p++;
                continue;
            }

            // For radial/script effects: compute coordinates
            double xd = px - w2;
            double yd = py - h2;

            // Normalized x,y for scripts (-1 to 1)
            double x = xd * xsc;
            double y = yd * ysc;

            // Polar coordinates (AVS convention: r offset by PI/2, d normalized 0..1)
            double d = sqrt(xd * xd + yd * yd) * divmax_d;
            double r = atan2(yd, xd) + M_PI * 0.5;

            // Check if this preset uses native radial function or script
            bool use_native = (preset_index >= 3 && preset_index <= 17 &&
                              preset_index != 7 && !uses_eval(preset_index));

            double tmp1, tmp2;

            if (use_native) {
                // Use native C++ radial effect functions (more accurate)
                int xo = 0, yo = 0;
                // Convert d back to pixel distance for native functions
                double d_pixels = d * max_d;
                apply_radial_effect(preset_index, r, d_pixels, max_d, xo, yo);

                // Convert back to screen coordinates
                tmp1 = (h2 + sin(r) * d_pixels + 0.5) + (yo * h) * (1.0 / 256.0);
                tmp2 = (w2 + cos(r) * d_pixels + 0.5) + (xo * w) * (1.0 / 256.0);
            }
            else {
                // Use script evaluation
                std::string script;
                bool is_rect = rectangular;

                if (preset_index >= 24) {
                    // Custom expression
                    script = parameters().get_string("custom_expr");
                } else {
                    // Built-in preset script
                    script = get_preset_script(preset_index);
                    // Some presets force rectangular mode
                    is_rect = uses_rect(preset_index);
                }

                if (!script.empty()) {
                    evaluate_movement_script(script, x, y, r, d, visdata, w, h);
                }

                // Convert back to screen coordinates
                if (!is_rect) {
                    // Polar: undo the PI/2 offset and scale d back
                    d *= max_d;
                    r -= M_PI / 2.0;
                    tmp1 = h2 + sin(r) * d;
                    tmp2 = w2 + cos(r) * d;
                } else {
                    // Rectangular
                    tmp1 = (y + 1.0) * h2;
                    tmp2 = (x + 1.0) * w2;
                }
            }

            // Store coordinates (common path for both native and script)
            if (can_subpixel) {
                // Store with subpixel precision
                oh = (int)tmp1;
                ow = (int)tmp2;
                int xpartial = (int)(32.0 * (tmp2 - ow));
                int ypartial = (int)(32.0 * (tmp1 - oh));

                if (wrap) {
                    ow = ow % (w - 1);
                    oh = oh % (h - 1);
                    if (ow < 0) ow += w - 1;
                    if (oh < 0) oh += h - 1;
                } else {
                    if (ow < 0) { xpartial = 0; ow = 0; }
                    if (ow >= w - 1) { xpartial = 31; ow = w - 2; }
                    if (oh < 0) { ypartial = 0; oh = 0; }
                    if (oh >= h - 1) { ypartial = 31; oh = h - 2; }
                }
                lookup_table_[p] = (ow + oh * w) | (ypartial << 22) | (xpartial << 27);
            } else {
                tmp1 += 0.5;
                tmp2 += 0.5;
                oh = (int)tmp1;
                ow = (int)tmp2;

                if (wrap) {
                    ow = ow % w;
                    oh = oh % h;
                    if (ow < 0) ow += w;
                    if (oh < 0) oh += h;
                } else {
                    if (ow < 0) ow = 0;
                    if (ow >= w) ow = w - 1;
                    if (oh < 0) oh = 0;
                    if (oh >= h) oh = h - 1;
                }
                lookup_table_[p] = ow + oh * w;
            }

            p++;
        }
    }

    // Update dimension tracking and mark table valid
    last_width_ = w;
    last_height_ = h;
    table_valid_ = true;
}

bool MovementEffect::uses_eval(int preset_index) const {
    // Presets 18-23 use script evaluation even though they have native descriptions
    return (preset_index >= 18 && preset_index <= 23);
}

bool MovementEffect::uses_rect(int preset_index) const {
    // Presets that use rectangular coordinates
    static const bool rect_flags[] = {
        false, // 0: none
        false, // 1: slight fuzzify
        true,  // 2: shift rotate left
        false, // 3: big swirl out
        false, // 4: medium swirl
        false, // 5: sunburster
        false, // 6: swirl to center
        false, // 7: blocky partial out
        false, // 8: swirling around both ways
        false, // 9: bubbling outward
        false, // 10: bubbling outward with swirl
        false, // 11: 5 pointed distro
        false, // 12: tunneling
        false, // 13: bleedin'
        true,  // 14: shifted big swirl out
        false, // 15: psychotic beaming outward
        false, // 16: cosine radial 3-way
        false, // 17: spinny tube
        false, // 18: radial swirlies
        false, // 19: swill
        true,  // 20: gridley
        true,  // 21: grapevine
        true,  // 22: quadrant
        true,  // 23: 6-way kaleida
    };
    if (preset_index >= 0 && preset_index < 24) {
        return rect_flags[preset_index];
    }
    return parameters().get_bool("rectangular", false);
}

void MovementEffect::apply_radial_effect(int preset_index, double& r, double& d, double max_d, int& xo, int& yo) {
    // Native radial effect functions from original AVS (more accurate than scripts)
    // These operate on r (angle) and d (pixel distance), with max_d as parameter
    switch (preset_index) {
        case 3:  // big swirl out
            r += 0.1 - 0.2 * (d / max_d);
            d *= 0.96;
            break;
        case 4:  // medium swirl
            d *= 0.99 * (1.0 - sin(r) / 32.0);
            r += 0.03 * sin(d / max_d * M_PI * 4);
            break;
        case 5:  // sunburster
            d *= 0.94 + (cos(r * 32.0) * 0.06);
            break;
        case 6:  // swirl to center
            d *= 1.01 + (cos(r * 4.0) * 0.04);
            r += 0.03 * sin(d / max_d * M_PI * 4);
            break;
        case 8:  // swirling around both ways at once
            r += 0.1 * sin(d / max_d * M_PI * 5);
            break;
        case 9:  // bubbling outward
            {
                double t = sin(d / max_d * M_PI);
                d -= 8 * t * t * t * t * t;
            }
            break;
        case 10: // bubbling outward with swirl
            {
                double t = sin(d / max_d * M_PI);
                d -= 8 * t * t * t * t * t;
                t = cos((d / max_d) * M_PI / 2.0);
                r += 0.1 * t * t * t;
            }
            break;
        case 11: // 5 pointed distro
            d *= 0.95 + (cos(r * 5.0 - M_PI / 2.50) * 0.03);
            break;
        case 12: // tunneling
            r += 0.04;
            d *= 0.96 + cos(d / max_d * M_PI) * 0.05;
            break;
        case 13: // bleedin'
            {
                double t = cos(d / max_d * M_PI);
                r += 0.07 * t;
                d *= 0.98 + t * 0.10;
            }
            break;
        case 14: // shifted big swirl out
            r += 0.1 - 0.2 * (d / max_d);
            d *= 0.96;
            xo = 8;
            break;
        case 15: // psychotic beaming outward
            d = max_d * 0.15;
            break;
        case 16: // cosine radial 3-way
            r = cos(r * 3);
            break;
        case 17: // spinny tube
            d *= (1 - (((d / max_d) - .35) * .5));
            r += .1;
            break;
        default:
            break;
    }
}

std::string MovementEffect::get_preset_script(int preset_index) const {
    // Preset scripts from original AVS r_trans.cpp descriptions[]
    // Note: presets 0, 1, 2, 7 are native effects - scripts shown are for documentation only
    static const std::vector<std::string> preset_scripts = {
        "// none - identity transform (native)",  // 0: none
        "// slight fuzzify - random +-1 pixel offset (native)",  // 1: slight fuzzify
        "x = x + 1/32; // use wrap for this one", // 2: shift rotate left
        "r = r + (0.1 - (0.2 * d)); d = d * 0.96", // 3: big swirl out
        "d = d * (0.99 * (1.0 - sin(r-$PI*0.5) / 32.0)); r = r + (0.03 * sin(d * $PI * 4))", // 4: medium swirl
        "d = d * (0.94 + (cos((r-$PI*0.5) * 32.0) * 0.06))", // 5: sunburster
        "d = d * (1.01 + (cos((r-$PI*0.5) * 4) * 0.04)); r = r + (0.03 * sin(d * $PI * 4))", // 6: swirl to center
        "// blocky partial out - grid zoom effect (native)",  // 7: blocky partial out
        "r = r + (0.1 * sin(d * $PI * 5))", // 8: swirling around both ways at once
        "t = sin(d * $PI); d = d - (8*t*t*t*t*t)/sqrt((sw*sw+sh*sh)/4)", // 9: bubbling outward
        "t = sin(d * $PI); d = d - (8*t*t*t*t*t)/sqrt((sw*sw+sh*sh)/4); t=cos(d*$PI/2.0); r= r + 0.1*t*t*t", // 10: bubbling outward with swirl
        "d = d * (0.95 + (cos(((r-$PI*0.5) * 5.0) - ($PI / 2.50)) * 0.03))", // 11: 5 pointed distro
        "r = r + 0.04; d = d * (0.96 + cos(d * $PI) * 0.05)", // 12: tunneling
        "t = cos(d * $PI); r = r + (0.07 * t); d = d * (0.98 + t * 0.10)", // 13: bleedin'
        "d=sqrt(x*x+y*y); r=atan2(y,x); r=r+0.1-0.2*d; d=d*0.96; x=cos(r)*d + 8/128; y=sin(r)*d", // 14: shifted big swirl out
        "d = 0.15", // 15: psychotic beaming outward
        "r = cos(r * 3)", // 16: cosine radial 3-way
        "d = d * (1 - ((d - .35) * .5)); r = r + .1", // 17: spinny tube
        "d = d * (1 - (sin((r-$PI*0.5) * 7) * .03)); r = r + (cos(d * 12) * .03)", // 18: radial swirlies
        "d = d * (1 - (sin((r - $PI*0.5) * 12) * .05)); r = r + (cos(d * 18) * .05); d = d * (1-((d - .4) * .03)); r = r + ((d - .4) * .13)", // 19: swill
        "x = x + (cos(y * 18) * .02); y = y + (sin(x * 14) * .03)", // 20: gridley
        "x = x + (cos(abs(y-.5) * 8) * .02); y = y + (sin(abs(x-.5) * 8) * .05); x = x * .95; y = y * .95", // 21: grapevine
        "y = y * ( 1 + (sin(r + $PI/2) * .3) ); x = x * ( 1 + (cos(r + $PI/2) * .3) ); x = x * .995; y = y * .995", // 22: quadrant
        "y = (r*6)/($PI); x = d" // 23: 6-way kaleida
    };
    
    if (preset_index >= 0 && preset_index < static_cast<int>(preset_scripts.size())) {
        return preset_scripts[preset_index];
    }
    return "";
}

void MovementEffect::evaluate_movement_script(const std::string& script, double& x, double& y, double& r, double& d, 
                                             AudioData visdata, int w, int h) {
    if (script.empty()) {
        return; // Identity transform
    }
    
    // Set up script variables
    static ScriptEngine engine;
    
    // Set coordinate variables
    engine.set_variable("x", x);
    engine.set_variable("y", y);
    engine.set_variable("r", r);
    engine.set_variable("d", d);
    
    // Set constants
    engine.set_variable("$PI", M_PI);
    engine.set_variable("$E", M_E);
    engine.set_variable("sw", (double)w);  // screen width
    engine.set_variable("sh", (double)h);  // screen height
    
    // Set audio variables if available
    // TODO: Extract audio data from visdata
    
    // Execute script
    engine.evaluate(script);
    
    // Get results back
    x = engine.get_variable("x");
    y = engine.get_variable("y");
    r = engine.get_variable("r");
    d = engine.get_variable("d");
}

// BLEND_AVG: 50/50 blend of two pixels (matching original AVS r_defs.h)
static inline uint32_t blend_avg(uint32_t a, uint32_t b) {
    return ((a >> 1) & 0x7f7f7f7f) + ((b >> 1) & 0x7f7f7f7f);
}

// BLEND_MAX: take maximum of each channel (for source mapped mode)
static inline uint32_t blend_max(uint32_t a, uint32_t b) {
    uint32_t result = 0;
    result |= std::max(a & 0xFF, b & 0xFF);
    result |= std::max(a & 0xFF00, b & 0xFF00);
    result |= std::max(a & 0xFF0000, b & 0xFF0000);
    result |= (a & 0xFF000000); // Preserve alpha from first pixel
    return result;
}

// BLEND4: Bilinear interpolation of 4 pixels (matching original AVS r_defs.h)
// p1 points to top-left pixel, w is framebuffer width
// xp, yp are 0-248 (stored as 0-31 * 8)
static inline uint32_t blend4(uint32_t* p1, int w, int xp, int yp) {
    // Calculate corner weights
    uint8_t a1 = g_blendtable[255 - xp][255 - yp];  // Top-left: (1-dx)*(1-dy)
    uint8_t a2 = g_blendtable[xp][255 - yp];        // Top-right: dx*(1-dy)
    uint8_t a3 = g_blendtable[255 - xp][yp];        // Bottom-left: (1-dx)*dy
    uint8_t a4 = g_blendtable[xp][yp];              // Bottom-right: dx*dy

    uint32_t tl = p1[0];
    uint32_t tr = p1[1];
    uint32_t bl = p1[w];
    uint32_t br = p1[w + 1];

    // Blend each color channel separately
    uint32_t b = g_blendtable[tl & 0xFF][a1] +
                 g_blendtable[tr & 0xFF][a2] +
                 g_blendtable[bl & 0xFF][a3] +
                 g_blendtable[br & 0xFF][a4];

    uint32_t g = g_blendtable[(tl >> 8) & 0xFF][a1] +
                 g_blendtable[(tr >> 8) & 0xFF][a2] +
                 g_blendtable[(bl >> 8) & 0xFF][a3] +
                 g_blendtable[(br >> 8) & 0xFF][a4];

    uint32_t r = g_blendtable[(tl >> 16) & 0xFF][a1] +
                 g_blendtable[(tr >> 16) & 0xFF][a2] +
                 g_blendtable[(bl >> 16) & 0xFF][a3] +
                 g_blendtable[(br >> 16) & 0xFF][a4];

    // Preserve alpha from top-left pixel
    return (tl & 0xFF000000) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

void MovementEffect::apply_transformation(uint32_t* input, uint32_t* output, int w, int h) {
    bool source_mapped = parameters().get_bool("source_mapped", false);
    bool blend = parameters().get_bool("blend", false);

    if (source_mapped) {
        // Forward mapping: for each source pixel, place it at the transformed location
        // In source mapped mode, original AVS:
        // - Clears output if no blend, else copies input to output
        // - Uses BLEND_MAX when placing pixels
        // Note: source mapped mode doesn't use subpixel filtering
        if (!blend) {
            memset(output, 0, w * h * sizeof(uint32_t));
        } else {
            memcpy(output, input, w * h * sizeof(uint32_t));
        }

        for (int i = 0; i < w * h; i++) {
            int dest_index = lookup_table_[i] & 0x3FFFFF;  // Mask off subpixel bits if present
            if (dest_index >= 0 && dest_index < w * h) {
                output[dest_index] = blend_max(input[i], output[dest_index]);
            }
        }

        // If blend mode, also average with original framebuffer
        if (blend) {
            for (int i = 0; i < w * h; i++) {
                output[i] = blend_avg(output[i], input[i]);
            }
        }
    } else {
        // Inverse mapping (default): for each output pixel, pull from the transformed location
        if (subpixel_enabled_) {
            // Subpixel mode: use bilinear interpolation
            // Lookup table format: bits 0-21 = base index, 22-26 = y frac, 27-31 = x frac
            for (int i = 0; i < w * h; i++) {
                int entry = lookup_table_[i];
                int base_idx = entry & 0x3FFFFF;      // bits 0-21
                int yp = ((entry >> 22) & 31) << 3;   // bits 22-26, scale to 0-248
                int xp = ((entry >> 27) & 31) << 3;   // bits 27-31, scale to 0-248

                uint32_t transformed;
                if (base_idx >= 0 && base_idx < w * h - w - 1) {
                    // Use bilinear interpolation of 4 neighboring pixels
                    transformed = blend4(input + base_idx, w, xp, yp);
                } else if (base_idx >= 0 && base_idx < w * h) {
                    // Near edge, fall back to direct lookup
                    transformed = input[base_idx];
                } else {
                    transformed = 0;
                }

                if (blend) {
                    output[i] = blend_avg(input[i], transformed);
                } else {
                    output[i] = transformed;
                }
            }
        } else {
            // Non-subpixel mode: direct index lookup
            for (int i = 0; i < w * h; i++) {
                int src_index = lookup_table_[i];
                uint32_t transformed;
                if (src_index >= 0 && src_index < w * h) {
                    transformed = input[src_index];
                } else {
                    transformed = 0; // Black for out-of-bounds
                }

                if (blend) {
                    // Blend transformed with original
                    output[i] = blend_avg(input[i], transformed);
                } else {
                    output[i] = transformed;
                }
            }
        }
    }
}

void MovementEffect::on_parameter_changed(const std::string& param_name) {
    if (param_name == "preset") {
        int preset_index = parameters().get_int("preset", 0);
        // Only sync for built-in presets (0-23), not "(user defined)" (24)
        if (preset_index < 24) {
            std::string script = get_preset_script(preset_index);
            parameters().set_string("custom_expr", script);
        }
        table_valid_ = false;
    }
    // Invalidate lookup table when any table-affecting parameter changes
    else if (param_name == "custom_expr" || param_name == "rectangular" ||
             param_name == "wrap" || param_name == "subpixel") {
        table_valid_ = false;
    }
    // Note: source_mapped and blend don't affect the table, only rendering
}

int MovementEffect::render(AudioData visdata, int isBeat,
                          uint32_t* framebuffer, uint32_t* fbout,
                          int w, int h) {
    if (!is_enabled()) return 0;

    // Check if we need to regenerate the lookup table
    if (needs_table_regeneration(w, h)) {
        generate_lookup_table(w, h, visdata);
    }

    // Apply transformation
    apply_transformation(framebuffer, fbout, w, h);

    return 1; // Use fbout
}

// Binary config loading from legacy AVS presets
void MovementEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    // Read effect index (preset 0-23, or 32767 for custom)
    int effect = 0;
    if (reader.remaining() >= 4) {
        effect = reader.read_i32();
    }

    std::string custom_expr;
    bool rectangular = false;

    // Handle custom expression (effect == 32767)
    if (effect == 32767) {
        // Check for "!rect " prefix (sets rectangular mode)
        if (reader.peek_match("!rect ", 6)) {
            reader.skip(6);
            rectangular = true;
        }

        // Check format: 1 = new format with length-prefixed string
        if (reader.remaining() > 0 && reader.read_u8() == 1) {
            // New format: length-prefixed string
            custom_expr = reader.read_length_prefixed_string();
        } else {
            // Old format: fixed 256-byte buffer (minus "!rect " if present)
            int buf_len = 256 - (rectangular ? 6 : 0);
            if (reader.remaining() >= static_cast<size_t>(buf_len)) {
                // Go back one byte since we consumed the format byte
                BinaryReader old_reader(data);
                old_reader.skip(4);  // Skip effect int
                if (rectangular) old_reader.skip(6);  // Skip "!rect "
                custom_expr = old_reader.read_string_fixed(buf_len);
                reader.skip(buf_len - 1);  // We already read 1 byte
            }
        }
    }

    // Read remaining config
    int blend = 0, sourcemapped = 0, subpixel = 1, wrap = 0;

    if (reader.remaining() >= 4) blend = reader.read_i32();
    if (reader.remaining() >= 4) sourcemapped = reader.read_i32();
    if (reader.remaining() >= 4) rectangular = reader.read_i32() != 0;
    if (reader.remaining() >= 4) subpixel = reader.read_i32();
    if (reader.remaining() >= 4) wrap = reader.read_i32();

    // If effect was stored as 0 (for backwards compatibility), read real value at end
    if (effect == 0 && reader.remaining() >= 4) {
        effect = reader.read_i32();
    }

    // Validate effect range
    // Stored value is 1-indexed: 0=disabled, 1=preset0, 2=preset1, etc.
    // Max stored value is 24 (preset 23 = 6-way kaleida)
    if (effect != 32767 && (effect < 0 || effect > 24)) {
        effect = 0;
    }

    // Set parameters
    if (effect == 32767) {
        // Custom expression mode (preset index 24 in our UI)
        parameters().set_int("preset", 24);
        parameters().set_string("custom_expr", custom_expr);
    } else if (effect > 0) {
        // Convert from 1-indexed stored value to 0-indexed preset
        parameters().set_int("preset", effect - 1);
        // Sync custom_expr with preset script
        on_parameter_changed("preset");
    } else {
        // effect == 0: use preset 0 (none/identity)
        parameters().set_int("preset", 0);
        // Sync custom_expr with preset script
        on_parameter_changed("preset");
    }

    parameters().set_bool("rectangular", rectangular);
    parameters().set_bool("blend", blend != 0);
    parameters().set_bool("source_mapped", (sourcemapped & 1) != 0);
    parameters().set_bool("subpixel", subpixel != 0);
    parameters().set_bool("wrap", wrap != 0);
}

// Static member definition
const PluginInfo MovementEffect::effect_info {
    .name = "Movement",
    .category = "Trans",
    .description = "Coordinate transformations with presets",
    .author = "",
    .version = 1,
    .legacy_index = 15,  // R_Trans
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<MovementEffect>();
    },
    .ui_layout = {
        {
            // Enabled (implicit in most effects but needed for parameter)
            {
                .id = "enabled",
                .text = "Enabled",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 0, .h = 0,  // Hidden, but parameter needed
                .default_val = 1
            },
            // Preset listbox - from res.rc: IDC_LIST1 at 101,0 132x142
            {
                .id = "preset",
                .text = "Preset",
                .type = ControlType::LISTBOX,
                .x = 101, .y = 0, .w = 132, .h = 142,
                .range = {0, 24},
                .default_val = 0,
                .options = {
                    "none", "slight fuzzify", "shift rotate left", "big swirl out",
                    "medium swirl", "sunburster", "swirl to center", "blocky partial out",
                    "swirling around both ways at once", "bubbling outward",
                    "bubbling outward with swirl", "5 pointed distro", "tunneling",
                    "bleedin'", "shifted big swirl out", "psychotic beaming outward",
                    "cosine radial 3-way", "spinny tube", "radial swirlies", "swill",
                    "gridley", "grapevine", "quadrant", "6-way kaleida (use wrap!)",
                    "(user defined)"
                }
            },
            // Custom expression edit box - from res.rc: IDC_EDIT1 at 0,144 233x70
            {
                .id = "custom_expr",
                .text = "User defined code:",
                .type = ControlType::EDITTEXT,
                .x = 0, .y = 144, .w = 233, .h = 70
            },
            // Source map checkbox - from res.rc: IDC_CHECK2 at 0,0 54x10
            {
                .id = "source_mapped",
                .text = "Source map",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 0, .w = 54, .h = 10,
                .default_val = 0
            },
            // Wrap checkbox - from res.rc: IDC_WRAP at 0,9 33x10
            {
                .id = "wrap",
                .text = "Wrap",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 9, .w = 33, .h = 10,
                .default_val = 0
            },
            // Blend checkbox - from res.rc: IDC_CHECK1 at 0,18 34x10
            {
                .id = "blend",
                .text = "Blend",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 18, .w = 34, .h = 10,
                .default_val = 0
            },
            // Bilinear (subpixel) checkbox - from res.rc: IDC_CHECK4 at 0,27 61x10
            {
                .id = "subpixel",
                .text = "Bilinear filtering",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 27, .w = 61, .h = 10,
                .default_val = 1
            },
            // Rectangular coords checkbox - from res.rc: IDC_CHECK3 at 0,114 100x15
            {
                .id = "rectangular",
                .text = "Rectangular coordinates",
                .type = ControlType::CHECKBOX,
                .x = 0, .y = 114, .w = 100, .h = 15,
                .default_val = 0
            }
        }
    }
};

// Register effect at startup
static bool register_movement = []() {
    PluginManager::instance().register_effect(MovementEffect::effect_info);
    return true;
}();

} // namespace avs