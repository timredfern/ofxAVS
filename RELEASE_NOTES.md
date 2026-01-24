## What's New

**Audio Management:**
- Responsive audio UI adapts to panel width

**Rendering:**
- Auto-resizing output renders at native resolution
- Fullscreen mode in multiwindow renders at screen resolution (not stretched)

**Architecture:**
- Multiwindow example uses independent GL contexts (no context sharing)
- CPU rendering (pixels) separated from GPU operations (texture) for cleaner multi-window support
- Vsync disabled in multiwindow for consistent frame rates

**UI Improvements:**
- Parameters panel shows effect name in window title (saves vertical space)
- Dynamic panel sizing adapts to window dimensions
- Context menus have subtle background contrast for visibility

**Documentation:**
- Updated README and BUILD with correct example names
- Added example descriptions (AVS_standard, AVS_multiwindow, AVS_simple)

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
