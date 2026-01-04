# AVS Project Instructions for Claude

## Critical Development Rules

**NEVER claim something is "fixed" until the user has tested and verified it working.**

The user handles all building and testing. After making code changes, wait for the user to confirm whether the fix works before claiming success.

# Project Context
This is the ORIGINAL AVS (Advanced Visualization Studio) source code being ported to modern C++/OpenFrameworks.
It contains the original Windows dialog procedures and UI layout data from the Winamp plugin.
## Source Code Locations
- **Modern C++ port**: `./libs/avs_lib/effects/` - New effect implementations
- **Original AVS source**: `../vis_avs/avs/vis_avs/` - Contains g_DlgProc functions and Windows UI layouts
- **Resource layouts**: Look in `../vis_avs/avs/vis_avs/res.rc` for dialog coordinates

The original source files use naming like `r_bright.cpp` for brightness effect, `r_dmove.cpp` for movement, etc.
Each contains the `g_DlgProc` function with the original Windows dialog control layouts and IDs.

## Alpha Channel Handling

**CRITICAL: Effects MUST preserve the alpha channel when modifying pixels.**

The original AVS BLEND macro (in `r_defs.h` lines 100-111) explicitly handles alpha:
```cpp
r=(a&0xff000000)+(b&0xff000000);
return t|min(r,0xff000000);
```

When writing effects that modify pixel colors:
- **Always preserve alpha**: Use `(pix & 0xFF000000) |` when outputting RGB values
- **Pixels are ARGB format**: `0xAARRGGBB` - alpha in high byte
- **Forgetting alpha causes transparent/black output** - effects will appear to "do nothing"

Example (correct):
```cpp
p[i] = (pix & 0xFF000000) | red_tab[(pix >> 16) & 0xff] | green_tab[(pix >> 8) & 0xff] | blue_tab[pix & 0xff];
```

## UI Layout Rules

**DO NOT arbitrarily change UI sizes, positions, or scaling factors.**

- UI element positions and sizes are documented in EFFECTS.md from the original AVS
- The 2x position scaling is intentional and built into the rendering code
- If you think something needs resizing, **test first** and let the user decide
- Never "preemptively" adjust sizes based on guesses about what might fit

## Critical Build Requirements

**NEVER hardcode paths in config.make or other files. ALWAYS use environment variables:**

```bash
export OF_ROOT=~/workspace/openFrameworks
```

This is required because:
1. It keeps the repository clean of filesystem-specific paths
2. Different developers use different openFrameworks locations  
3. Prevents accidental commits of hardcoded paths

## Git Repository

The git repository root is `/Users/tim/workspace/avs/ofxAVS`. Do NOT run `git init` - the repo already exists.

## Building and Testing

**The user handles ALL application builds and manual testing.** Do not attempt to run make, cmake, or any build commands for the main application. Do not add "Build and test" as a todo item - the user will do this.

**ALWAYS run automated tests before committing.** The test suite in `libs/avs_lib/tests/` can and should be run before any commit:
```bash
cd libs/avs_lib/tests/build
make && ./avs_tests
```
All tests must pass before committing. If tests fail, fix them or discuss with the user before proceeding.

### Standalone avs_lib example (no OpenFrameworks)
```bash
cd libs/avs_lib/example
mkdir build && cd build
cmake ..
make
./avs_example
```

### OpenFrameworks projects
The user will build these manually via Xcode or make.

## Library Architecture

**avs_lib** (`libs/avs_lib/`) is a standalone, framework-agnostic C++ library with zero external dependencies. It must remain portable and not include any OpenFrameworks or ImGui code.

**ofxAVS** (`src/`) is the OpenFrameworks addon layer that provides:
- ImGui-based UI rendering (`src/ui.h`, `src/ui.cpp` using `avs_ui::renderImGui()`)
- OpenFrameworks texture/pixel handling
- Integration with ofxImGui

Include paths: `addon_config.mk` adds `libs/avs_lib` to include paths, so use `#include "core/ui.h"` not `#include "../libs/avs_lib/core/ui.h"`.

## Project Context

This is an Advanced Visualization Studio (AVS) port to modern C++ and OpenFrameworks. The current focus is on implementing oscilloscope effects with dynamic movement and feedback for classic AVS-style visualizations.

Key components:
- Oscilloscope effect for audio waveform visualization
- Dynamic movement effect with grid-based transformations  
- Clear effect with feedback for trails
- Grid-based coordinate interpolation for authentic AVS stepping artifacts


## Fidelity to Original AVS

**CRITICAL RULE: ALWAYS stay true to the original AVS implementation. NEVER add your own enhancements or improvements.**

- **ALWAYS consult `libs/avs_lib/EFFECTS.md` first** - it is a comprehensive catalogue of research into the original Windows AVS code, documenting all control types, positions, IDs, blend modes, and effect behaviors. This should be the primary reference and may avoid the need to dig into the original source files.
- **SLIDERS MUST have Range and Default documented in EFFECTS.md** - When researching an effect, always document slider ranges as `Range(min, max), Default(value)`. If this information is missing from EFFECTS.md, research it from the original source and add it to EFFECTS.md BEFORE implementing.
- If EFFECTS.md doesn't have the needed information, research the original Windows AVS code in `../vis_avs/avs/vis_avs/` and UPDATE EFFECTS.md with the findings
- If a feature doesn't exist in the original, don't add it
- If behavior differs from original, change it to match
- Individual effects may have their own enable checkboxes, but there's no universal enable/disable for effects in the main chain
- The main effect chain is add/remove only, not enable/disable
- Dialog layouts should match the original coordinates in `res.rc`
- Control types should match original Windows controls (radio buttons, checkboxes, sliders)

## Effect Implementation

**Before implementing a new effect, READ at least one existing effect file to understand current patterns.** Check constructor pattern, PluginInfo structure, include style, and how parameters are set up.

Use flat includes: `"core/blend.h"` not `"../core/blend.h"`

## Copyright Headers for Source Files

**All new source files MUST include appropriate copyright headers:**

### For AVS-derived files (libs/avs_lib/)
Files that implement AVS functionality or are based on original AVS algorithms:

```cpp
// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root
```

### For ofxAVS and original files
Files that are OpenFrameworks-specific or original implementations:

```cpp
// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root
```

### Requirements
- **ALL** new .cpp and .h files must include appropriate header
- Choose the correct header based on whether the file is AVS-derived or original
- Place header at the very top of the file, before any includes
- Update year if adding to files in future years