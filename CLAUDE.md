# AVS Project Instructions for Claude

## Critical Build Requirements

**NEVER hardcode paths in config.make or other files. ALWAYS use environment variables:**

```bash
export OF_ROOT=/path/to/openFrameworks
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

**NEVER be overly positive or self-congratulatory.** Always:
- Highlight potential issues, risks, and limitations
- Point out what could go wrong
- Mention incomplete or problematic areas
- Be skeptical about claimed functionality
- Focus on what still needs work rather than what's "complete"

## Copyright Headers for Source Files

**All new source files MUST include appropriate copyright headers:**

### For AVS-derived files (libs/avs_lib/)
Files that implement AVS functionality or are based on original AVS algorithms:

```cpp
// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License
```

### For ofxAVS and original files
Files that are OpenFrameworks-specific or original implementations:

```cpp
// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License
```

### Requirements
- **ALL** new .cpp and .h files must include appropriate header
- Choose the correct header based on whether the file is AVS-derived or original
- Place header at the very top of the file, before any includes
- Update year if adding to files in future years