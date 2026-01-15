# Chain Example

Full-featured AVS visualization with effect chain editing and audio controls.

## Building

```bash
export OF_ROOT=/path/to/openFrameworks
make
make run
```

## Features

- **Effect Chain Panel** - View and edit the effect tree
- **Parameter Panel** - Adjust settings for selected effect
- **Beat Detector** - Automatic BPM detection with manual override
- **Audio Controls** - Input/output device selection, mic gain, file playback
- **Preset Loading** - Drag and drop .avs preset files
- **Session Persistence** - Effect chain saved between runs

## Controls

- **Space** - Play/pause audio file
- **Drag & Drop** - Load .avs presets or audio files

## Audio

Select input and output devices from the Audio panel. Supports:
- Microphone input with adjustable gain
- Audio file playback (drag and drop)
- Device selection saved between sessions

## UI Layout

```
┌──────────────────────────────────────────────────────────────────┐
│  Chain Panel    │  Parameters Panel         │  Visualization     │
│  - Beat Detect  │  - Selected effect        │  600x600 output    │
│  - Effect List  │    controls               │                    │
│    └─ Effects   │                           │                    │
│                 │                           │                    │
├─────────────────┴───────────────────────────┴────────────────────┤
│  Audio: Input [▼]  Output [▼]   ○ Mic ○ File   Gain [====]       │
└──────────────────────────────────────────────────────────────────┘
```
