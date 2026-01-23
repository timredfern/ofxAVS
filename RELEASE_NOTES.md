## What's New

**New: ofxAVS-multiwindow**
- Separate chain and output windows
- Output window is resizable with FPS in title bar
- Press Space for fullscreen output
- Right-click "Params" opens effect parameters in separate windows
- Multiple parameter windows can be open simultaneously

**Bug Fixes:**
- Fix color display from binary presets (colors were showing with R and B swapped - e.g., orange appeared blue)
- Fix crash when deleting non-empty effect lists
- Fix Bump effect to match original AVS behavior

**New Effects:**
- Multi Delay (frame buffer delay with multiple taps)
- Video Delay (single-tap frame buffer delay)
- 49 of 51 portable effects now implemented (96%)

**Script Engine:**
- Added 27 EEL functions: `if`, `above`, `below`, `equal`, `sign`, `sqr`, `invsqrt`, `sigmoid`, `band`, `bor`, `bnot`, `gettime`, `getosc`, `getspec`, `getkbmouse`, `megabuf`, `gmegabuf`, `assign`, `exec2`, `exec3`, `loop`, `while`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`
- Added modulo (`%`) and bitwise (`&`, `|`) operators

**Documentation:**
- Added macOS security instructions for unsigned app

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
| Space | Play/pause audio file |
| P | Toggle effect profiling |
| Drag & drop | Load .avs presets or audio files |

## macOS Security

The apps are not signed with an Apple Developer certificate. On first launch:
1. macOS will say it "can't be opened because Apple cannot check it for malicious software"
2. Go to **System Settings → Privacy & Security**
3. Scroll down and click **Open Anyway** next to the app name

## Requirements

- macOS 11.0 or later
