## v0.1.8 - Color Accuracy

**Major Color System Overhaul:**
- Fixed 8 critical R↔B channel swap bugs affecting starfield, ddm, ring, oscstar, rotstar, dot_grid, picture effects
- Converted all 27 color-handling effects to use explicit color.h utilities
- Eliminated error-prone magic number bit manipulation throughout codebase
- Colors now render correctly across all effects

**Bug Fixes:**
- Fix keyboard navigation losing focus after arrow key/delete in effect list
- Fix audio data indexing to match original AVS convention
- Fix UI color picker display for ARGB format
- Fix blitter_feedback UI: add missing labels and groupboxes

**Effect List:**
- Complete Effect List implementation with all blend modes
- Input/output blending with 14 modes (ignore, replace, 50/50, max, additive, etc.)
- Evaluation override scripting support
- Buffer-based alpha blending

**Developer Tools:**
- Add avs_test script for cross-implementation comparison with grandchild
- Add WORKLOG.md for session state persistence

## Installation

1. Choose your variant:
   - **ofxAVS-standard**: Single window with effect chain, parameters, and visualization
   - **ofxAVS-multiwindow**: Separate windows for chain, output, and parameters
2. Download the DMG for your Mac architecture:
   - **Apple Silicon** (M1/M2/M3): `arm64` version
   - **Intel**: `intel` version
3. Open the DMG and drag to Applications

## Controls

| Key | Action |
|-----|--------|
| Space | Play/pause audio (standard) / Toggle fullscreen (multiwindow output) |
| P | Toggle effect profiling |
| Drag & drop | Load .avs presets or audio files |

## macOS Security

The apps are not signed with an Apple Developer certificate. On first launch:
1. macOS will say it "can't be opened because Apple cannot check it for malicious software"
2. Go to **System Settings → Privacy & Security**
3. Scroll down and click **Open Anyway** next to the app name

## Requirements

- macOS 11.0 or later
