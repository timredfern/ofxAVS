# ofxAVS v1.0.1

Audio and beat detection improvements.

## New Features

- **Stereo spectrum analysis** - separate FFT for left and right channels instead of mono mix
- **Modern audio mode** - optional processing with spectral flux beat detection, more accurate than energy-based Classic mode
- **Runtime audio mode switching** - toggle between Classic (original Winamp) and Modern audio processing
- **Improved progress bar** - song title overlaid on seekable progress bar with drag support

## Changes

- Beat detector auto-keep now defaults to OFF for more reliable BPM lock-in

## UI Improvements

- Multiwindow layout: wider chain window (420px), taller input panel
- Narrower audio device dropdowns for better fit in compact layouts

---

# ofxAVS v1.0.0

Initial public release of ofxAVS - bringing Nullsoft's legendary Winamp visualizer to creative coding.

## Features

- **49 effects** ported from original AVS (96% coverage)
- **Full preset compatibility** - load original .avs preset files
- **Built-in audio management** - microphone input or audio file playback
- **Live MIDI input** - connect hardware controllers with channel filtering and debug window
- **ImGui-based UI** - effect chain editing with parameter controls
- **Beat detection** - automatic BPM detection and beat-triggered effects
- **Session persistence** - effect chains, audio and MIDI settings saved between sessions
- **Cross-platform** - macOS, Linux (Raspberry Pi tested)

## Installation

1. Download the DMG for your Mac architecture:
   - **Apple Silicon** (M1/M2/M3): `arm64` version
   - **Intel**: `intel` version
2. Open the DMG and drag to Applications

## Controls

| Key | Action |
|-----|--------|
| Cmd+S | Save preset |
| Cmd+L | Load preset |
| Space | Play/pause audio |
| P | Toggle effect profiling |
| ` | Toggle MIDI debug output |
| Drag & drop | Load .avs presets, audio files, .mid MIDI files |

## macOS Security

The app is not signed with an Apple Developer certificate. On first launch:
1. macOS will say it "can't be opened because Apple cannot check it for malicious software"
2. Go to **System Settings → Privacy & Security**
3. Scroll down and click **Open Anyway** next to the app name

## Requirements

- macOS 11.0 or later
- Linux support via source build (see docs/BUILD.md)

## Attribution

Based on original AVS source code by Nullsoft, Inc.
AVS Copyright (C) 2005 Nullsoft, Inc.
