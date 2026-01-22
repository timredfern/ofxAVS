## What's New

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

## macOS Security

The app is not signed with an Apple Developer certificate. On first launch:
1. macOS will say it "can't be opened because Apple cannot check it for malicious software"
2. Go to **System Settings → Privacy & Security**
3. Scroll down and click **Open Anyway** next to the AVS_standard message

## Requirements

- macOS 11.0 or later
