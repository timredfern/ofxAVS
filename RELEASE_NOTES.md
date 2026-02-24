## Cross-Platform Audio

- Replaced macOS-only audio decoder with miniaudio library
- Audio file loading now works on macOS, Linux, and Windows
- Supports MP3, WAV, FLAC, OGG, and other common formats

## Linux Support

- Fixed C++20 designated initializer ordering for GCC 13+
- Tested on Debian Trixie (Raspberry Pi 4)

## Bug Fixes

- Loading an audio file now clears any previously loaded MIDI file

---

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
| Cmd+S | Save preset |
| Cmd+L | Load preset |
| Space | Play/pause audio (standard) / Toggle fullscreen (multiwindow output) |
| P | Toggle effect profiling |
| ` | Toggle MIDI debug output |
| Drag & drop | Load .avs/.avsp presets, audio files, .mid MIDI files, or .avsc catalogues |

## macOS Security

The apps are not signed with an Apple Developer certificate. On first launch:
1. macOS will say it "can't be opened because Apple cannot check it for malicious software"
2. Go to **System Settings → Privacy & Security**
3. Scroll down and click **Open Anyway** next to the app name

## Requirements

- macOS 11.0 or later
- Linux support via source build
