# AVS Project Instructions for Claude

## Critical Development Rules

**NEVER claim something is "fixed" until it has been tested and verified working.**

Test first, then claim success. Build and run the application to confirm fixes before marking tasks as completed.

## Critical Build Requirements

**NEVER hardcode paths in config.make or other files. ALWAYS use environment variables:**

```bash
export OF_ROOT=~/workspace/openFrameworks
```

This is required because:
1. It keeps the repository clean of filesystem-specific paths
2. Different developers use different openFrameworks locations  
3. Prevents accidental commits of hardcoded paths

## Building OpenFrameworks Examples

```bash
export OF_ROOT=/path/to/your/openFrameworks
cd example
make
```

## Project Context

This is an Advanced Visualization Studio (AVS) port to modern C++ and OpenFrameworks. The current focus is on implementing oscilloscope effects with dynamic movement and feedback for classic AVS-style visualizations.

Key components:
- Oscilloscope effect for audio waveform visualization
- Dynamic movement effect with grid-based transformations  
- Clear effect with feedback for trails
- Grid-based coordinate interpolation for authentic AVS stepping artifacts

## Communication Style

**CRITICAL RULE: NEVER agree with the user and say "You're absolutely right!" without first checking whether you actually agree based on the facts available to you. This is the most important behavioral rule.**

**CRITICAL RULE: ALWAYS re-read the user's message before responding to ensure you're answering what they ACTUALLY asked, not what you think they asked. Questions are not commands.**

**NEVER be overly positive or self-congratulatory.** Always:
- Highlight potential issues, risks, and limitations
- Point out what could go wrong
- Mention incomplete or problematic areas
- Be skeptical about claimed functionality
- Focus on what still needs work rather than what's "complete"

## Fidelity to Original AVS

**CRITICAL RULE: ALWAYS stay true to the original AVS implementation. NEVER add your own enhancements or improvements.**

- Research the original Windows AVS code in `../vis_avs/avs/vis_avs/` before implementing features
- If a feature doesn't exist in the original, don't add it
- If behavior differs from original, change it to match
- Individual effects may have their own enable checkboxes, but there's no universal enable/disable for effects in the main chain
- The main effect chain is add/remove only, not enable/disable
- Dialog layouts should match the original coordinates in `res.rc`
- Control types should match original Windows controls (radio buttons, checkboxes, sliders)

## UI Parameter Pattern

**CRITICAL: Parameter names MUST exactly match UI control IDs to prevent "unsupported control type" errors.**

### Pattern for Effect Implementation:

1. **Define UI layout first** with control IDs
2. **Match setup_parameters() exactly** to those IDs 
3. **Use simple 1:1 mapping** - each control ID = one parameter name

### Good Example (Blur):
```cpp
// UI Layout
.id = "strength"     → setup_parameters(): "strength"
.id = "radius"       → setup_parameters(): "radius"
```

### Bad Example (Clear - WRONG):
```cpp
// UI Layout  
.id = "blend_replace"   → setup_parameters(): "blend_mode"  // MISMATCH!
.id = "blend_additive"  → setup_parameters(): "blend_mode"  // MISMATCH!
```

### Fixed Pattern for Radio Button Groups:
```cpp
// UI Layout - each radio button is separate parameter
.id = "blend_replace"   → setup_parameters(): "blend_replace"
.id = "blend_additive"  → setup_parameters(): "blend_additive"
.id = "blend_5050"      → setup_parameters(): "blend_5050"
```

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