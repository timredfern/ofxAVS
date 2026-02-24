# AVS File Formats

## .avs - Binary Preset (Legacy)

Original Winamp AVS binary format. Contains serialized effect chain with all parameters.

**Structure:**
```
[Header]
  4 bytes: Magic "Nullsoft AVS Preset 0.2\x1a" (25 bytes with null)
  1 byte:  Clear every frame flag

[Effect Chain]
  Recursive structure of effects/containers:
    4 bytes: Effect ID or 0xFFFFFFFF for container
    4 bytes: Data length
    N bytes: Effect-specific parameter data
```

**Usage:** Load existing AVS presets from the original Winamp era.

---

## .avsp - JSON Preset

Modern JSON format for AVS presets. Human-readable and editable.

**Structure:**
```json
{
  "version": 1,
  "root": {
    "type": "EffectList",
    "enabled": true,
    "parameters": {
      "clear_every_frame": true,
      "blend_mode": 0
    },
    "children": [
      {
        "type": "SuperScope",
        "enabled": true,
        "parameters": {
          "draw_mode": 1,
          "color": "#FF00FF",
          "init_script": "n=100",
          "frame_script": "",
          "beat_script": "",
          "point_script": "x=i*2-1; y=v*0.5"
        }
      }
    ]
  }
}
```

**Fields:**
- `version` - Format version (currently 1)
- `root` - Root EffectList container
- `type` - Effect class name
- `enabled` - Whether effect is active
- `parameters` - Effect-specific parameters
- `children` - Child effects (containers only)

---

## .avsc - Catalogue

JSON file linking a preset with audio and MIDI for synchronized playback.

**Structure:**
```json
{
  "preset": "path/to/preset.avs",
  "audio": "path/to/audio.wav",
  "midi": "path/to/sequence.mid"
}
```

**Fields:**
- `preset` - Path to .avs or .avsp preset file (optional)
- `audio` - Path to audio file (wav, mp3, ogg, etc.)
- `midi` - Path to Standard MIDI File (.mid)

**Paths:** Can be absolute or relative to the .avsc file location.

**Usage:** Drag & drop .avsc file onto AVS window to load audio and MIDI together, with MIDI events synchronized to audio playback position.

---

## .mid - Standard MIDI File

Standard MIDI File format (SMF). Parsed for note/CC/pitch bend events.

**Supported Events:**
- Note On/Off (channels 1-16, notes 0-127)
- Control Change (CC 0-127)
- Pitch Bend

**Script Access:**
```
midi_cc[0..127]      - CC values (0.0-1.0)
midi_note[0..127]    - Note velocities (0.0 = off, 0.0-1.0 = on)
midi_note_count      - Number of active notes
midi_note_index[i]   - Note number at index i
midi_pitch_bend      - Pitch bend (-1.0 to 1.0)
midi_any_note        - 1.0 if note triggered this frame
```
