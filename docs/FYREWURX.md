# FunkyFX FyrewurX v1 - Research Notes

Third-party AVS APE (AVS Plugin Effect) for Winamp's Advanced Visualization Studio.

## Overview

| Field | Value |
|-------|-------|
| Name | FunkyFX FyrewurX v1 |
| Category | Render APE |
| Author | Paul Holden |
| Date | November 28, 1999 |
| Binary | 45KB, 32-bit Windows DLL (PE32) |
| Source Code | **Not available** |

## Effect Description

FyrewurX is a beat-reactive fireworks particle effect. It renders animated firework bursts that react to music beats, with customizable color settings.

From AVS tutorials:
- Access via: `+` → `Render APE` → `FyrewurX`
- Works well combined with water & blur effects
- Has "Enable on beat speed changes" with speed sliders
- Multiple FyrewurX effects can be stacked

## Binary Analysis

```
File: fyrewurx.ape
Type: PE32 executable (DLL) (GUI) Intel 80386, for MS Windows
Size: 45,056 bytes
Export: _AVS_APE_RetrFunc (standard APE interface)
```

### Imported Functions

**Drawing (GDI32.dll):**
- CreatePen, CreateBrushIndirect
- Rectangle, SelectObject, DeleteObject

**UI (USER32.dll):**
- CreateDialogParamA, CheckDlgButton, IsDlgButtonChecked
- GetDlgItem, GetWindowRect, GetCursorPos, InvalidateRect

**Color Picker (comdlg32.dll):**
- ChooseColorA

**Timing (WINMM.dll):**
- timeGetTime

## Configuration Format

FyrewurX uses a simple 8-byte configuration:

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0 | 4 | int32 | enabled (0 = off, 1 = on) |
| 4 | 4 | int32 | color (Windows BGR format: 0x00BBGGRR) |

### Example Config Values (from presets)

| Preset | Enabled | Color (BGR) | Color (RGB) |
|--------|---------|-------------|-------------|
| steve likes it #1 | 1 | 0x005086c9 | #c98650 (orange-brown) |
| steve likes it #2 | 1 | 0x00ff8000 | #0080ff (blue) |
| kaleidoscope #1 | 1 | 0x00a65300 | #0053a6 (dark blue) |
| kaleidoscope #2 | 1 | 0x00530053 | #530053 (purple) |
| kaleidoscope #3 | 1 | 0x00000080 | #800000 (dark red) |

## AVS Preset Binary Format

In .avs preset files, FyrewurX appears as:

```
[4 bytes]  effect_index (>= 0x4000, indicates plugin)
[32 bytes] plugin_id "FunkyFX FyrewurX v1\0..." (null-padded)
[4 bytes]  config_length (always 8 for FyrewurX)
[8 bytes]  config_data (enabled + color)
```

## APE Interface

FyrewurX implements the standard AVS APE interface (from `avs_ape.h`):

```cpp
class C_RBASE {
    virtual int render(char visdata[2][2][576], int is_beat,
                      int *framebuffer, int *fbout, int w, int h) = 0;
    virtual char *get_desc() = 0;
    virtual void load_config(unsigned char *data, int len);
    virtual int save_config(unsigned char *data);
};

// Export function
extern "C" int _AVS_APE_RetrFunc(HINSTANCE hDllInstance,
                                  char **info, int *create);
```

## Download

Binary available from visbot.net:
- URL: https://files.visbot.net/avs/apes/fyrewurx.7z
- MD5: 742e61dcc7041c2e97b396cc81cda91e (reported)

## Implementation Status

Currently marked as `Unsupported: Plugin: FunkyFX FyrewurX v1` in avs_lib.

To implement without source code would require:
1. Reverse engineering the render() function from x86 assembly
2. Understanding the particle system (spawn, trajectory, fade logic)
3. Recreating the beat detection response behavior

## References

- [visbot.net/avs](https://visbot.net/avs) - APE binary downloads
- [AVS Forums Guide](https://visbot.github.io/AVS-Forums/html/t-66576.html) - Usage guide
- [visbot/awesome-avs](https://github.com/visbot/awesome-avs) - Curated AVS resources
- [grandchild/vis_avs](https://github.com/grandchild/vis_avs) - Modern AVS port

## Credits

Original plugin by Paul Holden, credited in AVS documentation:
> "Thanks to Paul Holden for the FunkyFX Firewurx APE"
