// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "superscope_effect.h"
#include "core/binary_reader.h"
#include "core/plugin_manager.h"
#include "core/ui.h"
#include "core/line_draw.h"
#include <algorithm>
#include <cmath>

namespace avs {

// Preset definitions from original AVS
struct SuperScopePreset {
    const char* name;
    const char* init;
    const char* point;
    const char* frame;
    const char* beat;
};

static const SuperScopePreset presets[] = {
    {"Spiral",
     "n=800",
     "d=i+v*0.2; r=t+i*$PI*4; x=cos(r)*d; y=sin(r)*d",
     "t=t-0.05",
     ""},
    {"3D Scope Dish",
     "n=200",
     "iz=1.3+sin(r+i*$PI*2)*(v+0.5)*0.88; ix=cos(r+i*$PI*2)*(v+0.5)*.88; iy=-0.3+abs(cos(v*$PI)); x=ix/iz;y=iy/iz;",
     "",
     ""},
    {"Rotating Bow Thing",
     "n=80;t=0.0;",
     "r=i*$PI*2; d=sin(r*3)+v*0.5; x=cos(t+r)*d; y=sin(t-r)*d",
     "t=t+0.01",
     ""},
    {"Vertical Bouncing Scope",
     "n=100; t=0; tv=0.1;dt=1;",
     "x=t+v*pow(sin(i*$PI),2); y=i*2-1.0;",
     "t=t*0.9+tv*0.1",
     "tv=((rand(50.0)/50.0))*dt; dt=-dt;"},
    {"Spiral Graph Fun",
     "n=100;t=0;",
     "r=i*$PI*128+t; x=cos(r/64)*0.7+sin(r)*0.3; y=sin(r/64)*0.7+cos(r)*0.3",
     "t=t+0.01;",
     "n=80+rand(120.0)"},
    {"Alternating Diagonal Scope",
     "n=64; t=1;",
     "sc=0.4*sin(i*$PI); x=2*(i-0.5-v*sc)*t; y=2*(i-0.5+v*sc);",
     "",
     "t=-t;"},
    {"Vibrating Worm",
     "n=w; dt=0.01; t=0; sc=1;",
     "x=cos(2*i+t)*0.9*(v*0.5+0.5); y=sin(i*2+t)*0.9*(v*0.5+0.5);",
     "t=t+dt;dt=0.9*dt+0.001; t=if(above(t,$PI*2),t-$PI*2,t);",
     "dt=sc;sc=-sc;"},
    {"Wandering Simple",
     "n=800;xa=-0.5;ya=0.0;xb=-0.0;yb=0.75;c=200;f=0;\nnxa=(rand(100)-50)*.02;nya=(rand(100)-50)*.02;\nnxb=(rand(100)-50)*.02;nyb=(rand(100)-50)*.02;",
     "x=(ex*i)+xa;\ny=(ey*i)+ya;\nx=x+ ( cos(r) * v * dv);\ny=y+ ( sin(r) * v * dv);\nred=i;\ngreen=(1-i);\nblue=abs(v*6);",
     "f=f+1;\nt=1-((cos((f*3.1415)/c)+1)*.5);\nxa=((nxa-lxa)*t)+lxa;\nya=((nya-lya)*t)+lya;\nxb=((nxb-lxb)*t)+lxb;\nyb=((nyb-lyb)*t)+lyb;\nex=(xb-xa);\ney=(yb-ya);\nd=sqrt(sqr(ex)+sqr(ey));\nr=atan(ey/ex)+(3.1415/2);\ndv=d*2",
     "c=f;\nf=0;\nlxa=nxa;\nlya=nya;\nlxb=nxb;\nlyb=nyb;\nnxa=(rand(100)-50)*.02;\nnya=(rand(100)-50)*.02;\nnxb=(rand(100)-50)*.02;\nnyb=(rand(100)-50)*.02"},
    {"Flitterbug",
     "n=180;t=0.0;lx=0;ly=0;vx=rand(200)-100;vy=rand(200)-100;cf=.97;c=200;f=0",
     "x=nx;y=ny;\nr=i*3.14159*2; d=(sin(r*5*(1-s))+i*0.5)*(.3-s);\ntx=(t*(1-(s*(i-.5))));\nx=x+cos(tx+r)*d; y=y+sin(t-y)*d;\nred=abs(x-nx)*5;\ngreen=abs(y-ny)*5;\nblue=1-s-red-green;",
     "f=f+1;t=(f*2*3.1415)/c;\nvx=(vx-(lx*.1))*cf;\nvy=(vy-(ly*.1))*cf;\nlx=lx+vx;ly=ly+vy;\nnx=lx*.001;ny=ly*.001;\ns=abs(nx*ny)",
     "c=f;f=0;\nvx=vx+rand(600)-300;vy=vy+rand(600)-300"},
    {"Spirostar",
     "n=20;t=0;f=0;c=200;mn=10;dv=2;dn=0",
     "r=if(b,0,((i*dv)*3.14159*128)+(t/2));\nx=cos(r)*sz;\ny=sin(r)*sz;",
     "f=f+1;t=(f*3.1415*2)/c;\nsz=abs(sin(t-3.1415));\ndv=if(below(n,12),(n/2)-1,\n    if(equal(12,n),3,\n    if(equal(14,n),6,\n    if(below(n,20),2,4))))",
     "bb = bb + 1;\nbeatdiv = 8;\nc=if(equal(bb%beatdiv,0),f,c);\nf=if(equal(bb%beatdiv,0),0,f);\ng=if(equal(bb%beatdiv,0),g+1,g);\nn=if(equal(bb%beatdiv,0),(abs((g%17)-8) *2)+4,n);"},
    {"Exploding Daisy",
     "n = 380 + rand(200) ; k = 0.0; l = 0.0; m = ( rand( 10 ) + 2 ) * .5; c = 0; f = 0",
     "r=(i*3.14159*2)+(a * 3.1415);\nd=sin(r*m)*.3;\nx=cos(k+r)*d*2;y=(  (sin(k-r)*d) + ( sin(l*(i-.5) ) ) ) * .7;\nred=abs(x);\ngreen=abs(y);\nblue=d",
     "a = a + 0.002 ; k = k + 0.04 ; l = l + 0.03",
     "bb = bb + 1;\nbeatdiv = 16;\nn=if(equal(bb%beatdiv,0),380 + rand(200),n);\nt=if(equal(bb%beatdiv,0),0.0,t);\na=if(equal(bb%beatdiv,0),0.0,a);\nk=if(equal(bb%beatdiv,0),0.0,k);\nl=if(equal(bb%beatdiv,0),0.0,l);\nm=if(equal(bb%beatdiv,0),(( rand( 100  ) + 2 ) * .1) + 2,m);"},
    {"Swirlie Dots",
     "n=45;t=rand(100);u=rand(100)",
     "di = ( i - .5) * 2;\nx = di;sin(u*di) * .4;\ny = cos(u*di) * .6;\nx = x + ( cos(t) * .05 );\ny = y + ( sin(t) * .05 );",
     "t = t + .15; u = u + .05",
     "bb = bb + 1;\nbeatdiv = 16;\nn = if(equal(bb%beatdiv,0),30 + rand( 30 ),n);"},
    {"Sweep",
     "n=180;lsv=100;sv=200;ssv=200;c=200;f=0",
     "sv=(sv*abs(cos(lsv)))+(lsv*abs(cos(sv)));\nfv=fv+(sin(sv)*dv);\nd=i; r=t+(fv * sin(t) * .3)*3.14159*4;\nx=cos(r)*d;\ny=sin(r)*d;\nred=i;\ngreen=abs(sin(r))-(red*.15);\nblue=fv",
     "f=f+1;t=(f*2*3.1415)/c;\nlsv=slsv;sv=ssv;fv=0",
     "bb = bb + 1;\nbeatdiv = 8;\nc=if(equal(bb%beatdiv,0),f,c);\nf=if(equal(bb%beatdiv,0),0,f);\ndv=if(equal(bb%beatdiv,0),((rand(100)*.01) * .1) + .02,dv);\nn=if(equal(bb%beatdiv,0),80+rand(100),n);\nssv=if(equal(bb%beatdiv,0),rand(200)+100,ssv);\nslsv=if(equal(bb%beatdiv,0),rand(200)+100,slsv);"},
    {"Whiplash Spiral",
     "n=80;c=200;f=0",
     "d=i;\nr=t+i*3.14159*4;\nsdt=sin(dt+(i*3.1415*2));\ncdt=cos(dt+(i*3.1415*2));\nx=(cos(r)*d) + (sdt * .6 * sin(t) );\ny=(sin(r)*d) + ( cdt *.6 * sin(t) );\nblue=abs(x);\ngreen=abs(y);\nred=cos(dt*4)",
     "t=t-0.05;f=f+1;dt=(f*2*3.1415)/c",
     "bb = bb + 1;\nbeatdiv = 8;\nc=if(equal(bb%beatdiv,0),f,c);\nf=if(equal(bb%beatdiv,0),0,f);"},
};

static const int NUM_PRESETS = sizeof(presets) / sizeof(presets[0]);

SuperScopeEffect::SuperScopeEffect() {
    init_parameters_from_layout(effect_info.ui_layout);

    // Set default scripts (Spiral preset from original AVS)
    parameters().set_string("init_script", "n=800");
    parameters().set_string("frame_script", "t=t-0.05");
    parameters().set_string("beat_script", "");
    parameters().set_string("point_script", "d=i+v*0.2; r=t+i*$PI*4; x=cos(r)*d; y=sin(r)*d");

    // Initialize engine with default n
    engine_.set_variable("n", 800.0);
    engine_.set_variable("t", 0.0);
}

// SuperScope uses the shared draw_line from core/line_draw.h

int SuperScopeEffect::render(AudioData visdata, int isBeat,
                              uint32_t* framebuffer, uint32_t* fbout,
                              int w, int h) {
    if (!is_enabled()) return 0;
    if (isBeat & 0x80000000) return 0;

    // Check for preset selection
    int preset_idx = parameters().get_int("example_preset");
    if (preset_idx > 0 && preset_idx <= NUM_PRESETS) {
        // Load the preset
        const SuperScopePreset& p = presets[preset_idx - 1];
        parameters().set_string("init_script", p.init);
        parameters().set_string("point_script", p.point);
        parameters().set_string("frame_script", p.frame);
        parameters().set_string("beat_script", p.beat);
        parameters().set_int("example_preset", 0);  // Reset selection
        inited_ = false;  // Force re-init
    }

    // Get parameters
    std::string init_script = parameters().get_string("init_script");
    std::string frame_script = parameters().get_string("frame_script");
    std::string beat_script = parameters().get_string("beat_script");
    std::string point_script = parameters().get_string("point_script");
    int channel = parameters().get_int("channel");  // 0=left, 1=center, 2=right

    // Color cycling - matches original AVS behavior
    int num_colors = parameters().get_int("num_colors");
    if (num_colors < 1) num_colors = 1;
    if (num_colors > 16) num_colors = 16;

    uint32_t current_color;
    if (num_colors == 1) {
        // Single color - no cycling
        current_color = parameters().get_color("color_0");
    } else {
        // Multi-color cycling with interpolation
        // Each color gets 64 frames, then interpolates to next
        color_pos_++;
        if (color_pos_ >= num_colors * 64) color_pos_ = 0;

        int p = color_pos_ / 64;      // Current color index
        int r = color_pos_ & 63;      // Interpolation factor (0-63)

        std::string c1_param = "color_" + std::to_string(p);
        std::string c2_param = "color_" + std::to_string((p + 1 < num_colors) ? p + 1 : 0);
        uint32_t c1 = parameters().get_color(c1_param);
        uint32_t c2 = parameters().get_color(c2_param);

        // Linear interpolation: blend = ((c1 * (63-r)) + (c2 * r)) / 64
        // Color format is 0xAABBGGRR (R in bits 0-7, G in bits 8-15, B in bits 16-23)
        int red = (((c1 & 0xFF) * (63 - r)) + ((c2 & 0xFF) * r)) / 64;
        int green = ((((c1 >> 8) & 0xFF) * (63 - r)) + (((c2 >> 8) & 0xFF) * r)) / 64;
        int blue = ((((c1 >> 16) & 0xFF) * (63 - r)) + (((c2 >> 16) & 0xFF) * r)) / 64;

        current_color = 0xFF000000 | (blue << 16) | (green << 8) | red;
    }


    // Prepare audio data
    // visdata[0] = waveform, visdata[1] = spectrum
    char* audio_data;
    static char center_channel[576];
    int ws = (parameters().get_int("source_mode") == 1) ? 1 : 0;  // 0=waveform, 1=spectrum
    int xorv = (ws == 0) ? 128 : 0;  // Waveform is signed (needs XOR 128), spectrum is unsigned

    if (channel == 1) {  // Center
        for (int i = 0; i < 576; i++) {
            center_channel[i] = visdata[ws][0][i] / 2 + visdata[ws][1][i] / 2;
        }
        audio_data = center_channel;
    } else if (channel == 2) {  // Right
        audio_data = &visdata[ws][1][0];
    } else {  // Left (default)
        audio_data = &visdata[ws][0][0];
    }

    // Set up engine variables
    engine_.set_variable("w", static_cast<double>(w));
    engine_.set_variable("h", static_cast<double>(h));
    engine_.set_variable("b", isBeat ? 1.0 : 0.0);
    // Color format is 0xAABBGGRR (R in bits 0-7, G in bits 8-15, B in bits 16-23)
    engine_.set_variable("red", (current_color & 0xFF) / 255.0);
    engine_.set_variable("green", ((current_color >> 8) & 0xFF) / 255.0);
    engine_.set_variable("blue", ((current_color >> 16) & 0xFF) / 255.0);
    engine_.set_variable("skip", 0.0);
    engine_.set_variable("linesize", 1.0);
    engine_.set_variable("drawmode", parameters().get_int("draw_mode"));

    // Run init script (once)
    if (!inited_ && !init_script.empty()) {
        engine_.evaluate(init_script);
        inited_ = true;
    }

    // Run frame script
    if (!frame_script.empty()) {
        engine_.evaluate(frame_script);
    }

    // Run beat script
    if (isBeat && !beat_script.empty()) {
        engine_.evaluate(beat_script);
    }

    // Run point script for each point
    if (!point_script.empty()) {
        int n = static_cast<int>(engine_.get_variable("n"));
        if (n < 1) n = 1;
        if (n > 128 * 1024) n = 128 * 1024;

        bool can_draw = false;
        int last_x = 0, last_y = 0;

        for (int a = 0; a < n; a++) {
            // Calculate audio value with interpolation
            double r = (a * 576.0) / n;
            double s1 = r - static_cast<int>(r);
            int idx = static_cast<int>(r);
            if (idx >= 575) idx = 574;

            unsigned char sample1 = static_cast<unsigned char>(audio_data[idx]) ^ xorv;
            unsigned char sample2 = static_cast<unsigned char>(audio_data[idx + 1]) ^ xorv;
            double yr = sample1 * (1.0 - s1) + sample2 * s1;

            // Set per-point variables
            engine_.set_variable("v", yr / 128.0 - 1.0);  // -1 to 1
            engine_.set_variable("i", static_cast<double>(a) / static_cast<double>(n - 1));
            engine_.set_variable("skip", 0.0);

            // Execute point script
            engine_.evaluate(point_script);

            // Get output coordinates
            double var_x = engine_.get_variable("x");
            double var_y = engine_.get_variable("y");
            int px = static_cast<int>((var_x + 1.0) * w * 0.5);
            int py = static_cast<int>((var_y + 1.0) * h * 0.5);

            // Check skip
            if (engine_.get_variable("skip") < 0.00001) {
                // Get per-point color (0xAABBGGRR format)
                int point_color = make_color_component(engine_.get_variable("red")) |
                                  (make_color_component(engine_.get_variable("green")) << 8) |
                                  (make_color_component(engine_.get_variable("blue")) << 16);
                point_color |= 0xFF000000;  // Alpha

                double current_drawmode = engine_.get_variable("drawmode");

                if (current_drawmode < 0.00001) {
                    // Dots mode
                    draw_point(framebuffer, px, py, w, h, point_color);
                } else {
                    // Lines mode
                    if (can_draw) {
                        draw_line(framebuffer, last_x, last_y, px, py, w, h, point_color);
                    }
                }
            }

            can_draw = true;
            last_x = px;
            last_y = py;
        }
    }

    return 0;
}

// Build preset names for dropdown
static std::vector<std::string> build_preset_names() {
    std::vector<std::string> names;
    names.push_back("(select example)");
    for (int i = 0; i < NUM_PRESETS; i++) {
        names.push_back(presets[i].name);
    }
    return names;
}

// Static member definition
const PluginInfo SuperScopeEffect::effect_info {
    .name = "SuperScope",
    .category = "Render",
    .description = "Advanced oscilloscope with scripting",
    .author = "",
    .version = 1,
    .legacy_index = 36,  // R_SScope
    .factory = []() -> std::unique_ptr<avs::EffectBase> {
        return std::make_unique<SuperScopeEffect>();
    },
    .ui_layout = {
        {
            // Labels for script boxes (match original: no colons, exact positions)
            {
                .id = "init_label",
                .text = "init",
                .type = ControlType::LABEL,
                .x = 0, .y = 9, .w = 10, .h = 8
            },
            {
                .id = "frame_label",
                .text = "frame",
                .type = ControlType::LABEL,
                .x = 1, .y = 41, .w = 18, .h = 8
            },
            {
                .id = "beat_label",
                .text = "beat",
                .type = ControlType::LABEL,
                .x = 0, .y = 87, .w = 15, .h = 8
            },
            {
                .id = "point_label",
                .text = "point",
                .type = ControlType::LABEL,
                .x = 0, .y = 131, .w = 16, .h = 8
            },
            // Init script - IDC_EDIT4
            {
                .id = "init_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 0, .w = 208, .h = 26
            },
            // Frame script - IDC_EDIT2
            {
                .id = "frame_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 26, .w = 208, .h = 46
            },
            // Beat script - IDC_EDIT3
            {
                .id = "beat_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 72, .w = 208, .h = 38
            },
            // Point/Pixel script - IDC_EDIT1
            {
                .id = "point_script",
                .text = "",
                .type = ControlType::EDITTEXT,
                .x = 25, .y = 110, .w = 208, .h = 56
            },
            // Help button - positioned left of example dropdown
            {
                .id = "help_button",
                .text = "?",
                .type = ControlType::HELP_BUTTON,
                .x = 60, .y = 165, .w = 73, .h = 14
            },
            // Example preset dropdown - IDC_BUTTON1 (was "Load example..." button)
            {
                .id = "example_preset",
                .text = "",
                .type = ControlType::DROPDOWN,
                .x = 145, .y = 170, .w = 88, .h = 14,
                .default_val = 0,
                .options = {"(select)", "Spiral", "3D Scope Dish", "Rotating Bow Thing",
                            "Vertical Bouncing Scope", "Spiral Graph Fun",
                            "Alternating Diagonal Scope", "Vibrating Worm",
                            "Wandering Simple", "Flitterbug", "Spirostar",
                            "Exploding Daisy", "Swirlie Dots", "Sweep", "Whiplash Spiral"}
            },
            // Source data label
            {
                .id = "source_label",
                .text = "Source data:",
                .type = ControlType::LABEL,
                .x = 0, .y = 168, .w = 44, .h = 8
            },
            // Source mode: Waveform vs Spectrum - IDC_WAVE, IDC_SPEC
            {
                .id = "source_mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Waveform", 5, 178, 49, 10},
                    {"Spectrum", 55, 178, 46, 10}
                },
                .default_val = 0  // Waveform
            },
            // Channel selection - IDC_LEFTCH, IDC_MIDCH, IDC_RIGHTCH
            {
                .id = "channel",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Left", 5, 189, 28, 10},
                    {"Center", 36, 189, 37, 10},
                    {"Right", 76, 189, 33, 10}
                },
                .default_val = 1  // Center
            },
            // Draw as label
            {
                .id = "drawas_label",
                .text = "Draw as:",
                .type = ControlType::LABEL,
                .x = 132, .y = 188, .w = 29, .h = 8
            },
            // Draw mode: Dots vs Lines - IDC_DOT, IDC_LINES
            {
                .id = "draw_mode",
                .type = ControlType::RADIO_GROUP,
                .radio_options = {
                    {"Dots", 166, 188, 31, 10},
                    {"Lines", 200, 188, 33, 10}
                },
                .default_val = 1  // Lines
            },
            // Colors labels
            {
                .id = "cycle_label",
                .text = "Cycle through",
                .type = ControlType::LABEL,
                .x = 0, .y = 204, .w = 46, .h = 8
            },
            // Number of colors - IDC_NUMCOL
            {
                .id = "num_colors",
                .text = "",
                .type = ControlType::TEXT_INPUT,
                .x = 47, .y = 202, .w = 19, .h = 12,
                .range = {1, 16},
                .default_val = 1
            },
            {
                .id = "colors_label",
                .text = "colors (max 16)",
                .type = ControlType::LABEL,
                .x = 71, .y = 204, .w = 48, .h = 8
            },
            // Color array - IDC_DEFCOL
            {
                .id = "colors",
                .text = "",
                .type = ControlType::COLOR_ARRAY,
                .x = 125, .y = 202, .w = 108, .h = 11,
                .default_val = 0xFFFFFFFF,
                .max_items = 16
            }
        }
    },
    .help_text =
        "SuperScope - Scripted Oscilloscope\n"
        "\n"
        "Scripts run in order: init (once), frame,\n"
        "beat, then point (for each point).\n"
        "\n"
        "Read-only variables:\n"
        "  n     Number of points (set in init)\n"
        "  i     Point index (0 to 1)\n"
        "  v     Audio sample (-1 to 1)\n"
        "  b     Beat flag (1 on beat, else 0)\n"
        "  w,h   Screen dimensions\n"
        "\n"
        "Output variables (set in point script):\n"
        "  x,y   Position (-1 to 1, center=0)\n"
        "  red,green,blue  Color (0 to 1)\n"
        "  skip  Set >0 to skip this point\n"
        "  drawmode  0=dots, 1=lines\n"
        "\n"
        "Constants: $PI (3.14159...)\n"
        "\n"
        "Functions: sin,cos,tan,sqrt,pow,log,\n"
        "abs,min,max,if,rand,sqr,sign,atan,\n"
        "atan2,floor,ceil,invsqrt,equal,\n"
        "above,below,assign\n"
        "\n"
        "Example (circle):\n"
        "  init: n=100\n"
        "  point: r=i*$PI*2; x=cos(r); y=sin(r)\n"
};

void SuperScopeEffect::on_parameter_changed(const std::string& param_name) {
    // Re-run init script when it changes
    if (param_name == "init_script") {
        inited_ = false;
    }
}

// Binary config loading from legacy AVS presets
void SuperScopeEffect::load_parameters(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    BinaryReader reader(data);

    std::string point_script, frame_script, beat_script, init_script;

    // Check format: new format starts with 1, old format is raw data
    if (data[0] == 1) {
        // New format with length-prefixed strings
        reader.skip(1);  // Skip the format marker
        point_script = reader.read_length_prefixed_string();  // effect_exp[0]
        frame_script = reader.read_length_prefixed_string();  // effect_exp[1]
        beat_script = reader.read_length_prefixed_string();   // effect_exp[2]
        init_script = reader.read_length_prefixed_string();   // effect_exp[3]
    } else {
        // Old format: 4 x 256-byte fixed buffers
        if (data.size() >= 1024) {
            point_script = reader.read_string_fixed(256);
            frame_script = reader.read_string_fixed(256);
            beat_script = reader.read_string_fixed(256);
            init_script = reader.read_string_fixed(256);
        }
    }

    // Set scripts
    parameters().set_string("point_script", point_script);
    parameters().set_string("frame_script", frame_script);
    parameters().set_string("beat_script", beat_script);
    parameters().set_string("init_script", init_script);

    // which_ch: bits 0-1 = channel (0=left, 1=right, 2=center), bit 2 = spectrum (1) vs waveform (0)
    if (reader.remaining() >= 4) {
        uint32_t which_ch = reader.read_u32();
        int channel = which_ch & 3;  // 0=left, 1=right, 2=center
        int source_mode = (which_ch & 4) ? 1 : 0;  // 0=waveform, 1=spectrum
        parameters().set_int("channel", channel);
        parameters().set_int("source_mode", source_mode);
    }

    // num_colors
    int num_colors = 1;
    if (reader.remaining() >= 4) {
        num_colors = static_cast<int>(reader.read_u32());
        if (num_colors < 1) num_colors = 1;
        if (num_colors > 16) num_colors = 16;
        parameters().set_int("num_colors", num_colors);
    }

    // colors array
    for (int i = 0; i < num_colors && reader.remaining() >= 4; i++) {
        uint32_t color = reader.read_u32();
        std::string color_param = "color_" + std::to_string(i);
        parameters().set_color(color_param, BinaryReader::bgr_add_alpha(color));
    }

    // mode (0=dots, 1=lines)
    if (reader.remaining() >= 4) {
        int mode = static_cast<int>(reader.read_u32());
        parameters().set_int("draw_mode", mode ? 1 : 0);
    }

    // Reset init state so init script runs
    inited_ = false;
}

// Register effect at startup
static bool register_superscope = []() {
    PluginManager::instance().register_effect(SuperScopeEffect::effect_info);
    return true;
}();

} // namespace avs
