## What's New

**New Effects:**
- Color Modifier (scripted color channel modification)
- Color Clip (replace pixels by color threshold)
- Water Bump (water ripple displacement)
- 47 of 52 portable effects now implemented (90%)

**Examples Refactored:**
- Renamed `simple` → `AVS_simple` (minimal full-window visualizer)
- Renamed `chain` → `AVS_standard` (full UI with effect editing)
- AVS_simple now works standalone with mic input, no UI panels

**API Changes:**
- Session load/save now explicit via `loadSession()`/`saveSession()`
- Added `getRoot()` to access effect chain parameters

## Installation

1. Download the DMG for your Mac:
   - **Apple Silicon** (M1/M2/M3): `arm64` version
   - **Intel**: `intel` version
2. Open the DMG and drag to Applications
3. Run AVS_standard from Applications

## Controls

| Key | Action |
|-----|--------|
| Space | Play/pause audio file |
| P | Toggle effect profiling |
| Drag & drop | Load .avs presets or audio files |

## Requirements

- macOS 11.0 or later
