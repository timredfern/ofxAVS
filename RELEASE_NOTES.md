## Inital MIDI Support

**Preset Save/Load:**
- Right-click menu: Save Effect / Load Effect (single effect or entire EffectList)
- Right-click on root: Save Preset / Load Preset (entire chain)
- Cmd+S / Cmd+L for global preset save/load
- Load Effect adds to existing chain (doesn't replace)

**avs_lib Events:**
- Agnostic typed event bus
- Scripts can now access MIDI data via `midi_cc[0..127]`, `midi_note[0..127]`, `midi_pitch_bend`

**MIDI file support in UI examples:**
- MIDI playback synced with audio position
- Catalogue files (.avsc) link audio + MIDI for synchronized playback
- Press **`** (backtick) to toggle MIDI debug output
- MIDI file path saved/restored with audio settings

**Documentation:**
- New FILE_FORMATS.md documenting .avs, .avsp, .avsc, and .mid formats
- MIDI tab in expression help with array/variable reference

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
