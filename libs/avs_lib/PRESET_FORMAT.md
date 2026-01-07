# AVS Preset File Format

This document describes the binary format of AVS preset files (`.avs`), based on analysis of the original Winamp AVS source code.

## Overview

AVS presets store a complete visualization configuration including:
- Effect chain (list of effects in render order)
- Per-effect parameters and scripts
- Nested Effect Lists (containers that group effects)

## File Structure

```
[Header]
[Root Mode Byte]
[Effect 0]
[Effect 1]
...
[Effect N]
```

## Header

The file begins with a 25-byte signature:

```
"Nullsoft AVS Preset 0.2\x1a"
```

- Bytes 0-21: `"Nullsoft AVS Preset 0."`
- Byte 22: Version character (`'1'` or `'2'`)
- Byte 23: `'.'` (this is part of "0.2" but the actual check is on byte 22)
- Byte 24: `0x1a` (EOF marker, standard for DOS-era files)

The loader accepts version '1' or '2' at position 22.

## Data Types

All multi-byte integers are **little-endian**.

### Integer (4 bytes)
```
byte[0] | (byte[1] << 8) | (byte[2] << 16) | (byte[3] << 24)
```

### Length-Prefixed String
```
[4 bytes] length (includes null terminator, 0 if empty)
[N bytes] string data with null terminator (if length > 0)
```

## Root Mode Byte

After the header, a single byte indicates the root list's mode:
- This controls overall rendering behavior
- Typically 0 for normal operation

## Effect Entry Format

Each effect in the chain is serialized as:

```
[4 bytes] effect_index
[32 bytes] string_id (only if effect_index >= 16384)
[4 bytes] config_length
[config_length bytes] effect-specific config data
```

### Effect Index Values

Built-in effects have integer indices 0-45, assigned by registration order in `rlib.cpp:initfx()`:

| Index | Function | Effect Name |
|-------|----------|-------------|
| 0 | R_SimpleSpectrum | Simple Spectrum |
| 1 | R_DotPlane | Dot Plane |
| 2 | R_OscStars | Oscilloscope Stars |
| 3 | R_FadeOut | Fade Out |
| 4 | R_BlitterFB | Blitter Feedback |
| 5 | R_NFClear | On Beat Clear |
| 6 | R_Blur | Blur |
| 7 | R_BSpin | Beat Spin |
| 8 | R_Parts | Particles |
| 9 | R_RotBlit | Rotation Blitter |
| 10 | R_SVP | Simple VU Meter |
| 11 | R_ColorFade | Color Fade |
| 12 | R_ContrastEnhance | Contrast Enhance |
| 13 | R_RotStar | Rotating Stars |
| 14 | R_OscRings | Oscilloscope Rings |
| 15 | R_Trans | Movement (Transform) |
| 16 | R_Scat | Scatter |
| 17 | R_DotGrid | Dot Grid |
| 18 | R_Stack | Buffer Save/Restore |
| 19 | R_DotFountain | Dot Fountain |
| 20 | R_Water | Water |
| 21 | R_Comment | Comment |
| 22 | R_Brightness | Brightness |
| 23 | R_Interleave | Interleave |
| 24 | R_Grain | Grain |
| 25 | R_Clear | Clear Screen |
| 26 | R_Mirror | Mirror |
| 27 | R_StarField | Starfield |
| 28 | R_Text | Text |
| 29 | R_Bump | Bump Mapping |
| 30 | R_Mosaic | Mosaic |
| 31 | R_WaterBump | Water Bump |
| 32 | R_AVI | AVI Player |
| 33 | R_Bpm | BPM Detector |
| 34 | R_Picture | Picture |
| 35 | R_DDM | Dynamic Distance Modifier |
| 36 | R_SScope | SuperScope |
| 37 | R_Invert | Invert |
| 38 | R_Onetone | Unique Tone |
| 39 | R_Timescope | Timescope |
| 40 | R_LineMode | Line Mode |
| 41 | R_Interferences | Interferences |
| 42 | R_Shift | Channel Shift |
| 43 | R_DMove | Dynamic Movement |
| 44 | R_FastBright | Fast Brightness |
| 45 | R_DColorMod | Color Modifier |

### Special Index Values

| Value | Meaning |
|-------|---------|
| 0xFFFFFFFE | Effect List (container) |
| >= 16384 | Plugin effect (uses string identifier) |

### Plugin Effects

When `effect_index >= 16384` (DLLRENDERBASE), the next 32 bytes contain a null-terminated string identifier for the plugin. This allows third-party APE (AVS Plugin Effect) files to register custom effects.

Some plugins were later converted to built-in effects:

| Plugin ID | Built-in Index |
|-----------|----------------|
| "Winamp Brightness APE v1" | 22 |
| "Winamp Interleave APE v1" | 23 |
| "Winamp Grain APE v1" | 24 |
| "Winamp ClearScreen APE v1" | 25 |
| "Nullsoft MIRROR v1" | 26 |
| "Winamp Starfield v1" | 27 |
| "Winamp Text v1" | 28 |
| "Winamp Bump v1" | 29 |
| "Winamp Mosaic v1" | 30 |
| "Winamp AVIAPE v1" | 32 |
| "Nullsoft Picture Rendering v1" | 34 |
| "Winamp Interf APE v1" | 41 |

## Effect List (Container) Format

Effect Lists are special - they contain child effects and have extended configuration.

### Effect List Config Structure

```
[1 byte] mode (OR'd with 0x80 if extended data follows)

If (mode & 0x80):
    [4 bytes] full mode value
    [4 bytes] inblendval (0-255)
    [4 bytes] outblendval (0-255)
    [4 bytes] bufferin (0-7)
    [4 bytes] bufferout (0-7)
    [4 bytes] ininvert (0 or 1)
    [4 bytes] outinvert (0 or 1)
    [4 bytes] beat_render (0 or 1)
    [4 bytes] beat_render_frames

If non-root Effect List:
    [4 bytes] DLLRENDERBASE (16384)
    [32 bytes] "AVS 2.8+ Effect List Config"
    [4 bytes] code_config_length
    [code_config_length bytes] code config (use_code + init/frame expressions)

[Child effects...]
```

### Effect List Blend Modes

The `blendin` and `blendout` values select from these modes:

| Value | Mode |
|-------|------|
| 0 | Ignore |
| 1 | Replace |
| 2 | 50/50 |
| 3 | Maximum |
| 4 | Additive |
| 5 | Subtractive 1 |
| 6 | Subtractive 2 |
| 7 | Every other line |
| 8 | Every other pixel |
| 9 | XOR |
| 10 | Adjustable |
| 11 | Multiply |
| 12 | Buffer |
| 13 | Minimum |

## Per-Effect Config Formats

Each effect type has its own serialization format. Examples below.

### SuperScope (Index 36)

```
[1 byte] version (1 = new string format, 0 = legacy)

If version == 1:
    [string] point code (per-point expression)
    [string] frame code (per-frame expression)
    [string] beat code (on-beat expression)
    [string] init code (initialization expression)
Else (legacy format):
    [1024 bytes] fixed-size code block (256 bytes per expression)

[4 bytes] which_ch (audio channel: 0=left, 1=right, 2=center, +4=spectrum)
[4 bytes] num_colors (1-16)
[4 bytes × num_colors] colors (0x00BBGGRR format)
[4 bytes] mode (0=dots, 1=lines)
```

### Clear Screen (Index 25)

```
[4 bytes] enabled (0 or 1)
[4 bytes] onbeat (0=every frame, 1=on beat only)
[4 bytes] color (0x00BBGGRR)
[4 bytes] blend_mode (0=replace, 1=additive, 2=50/50, etc.)
[4 bytes] onbeat_count (frames to clear after beat, if onbeat > 1)
```

### Brightness (Index 22)

```
[4 bytes] enabled
[4 bytes] blend_mode (0=replace, 1=additive, 2=50/50)
[4 bytes] red_adjust (0-8192, center=4096)
[4 bytes] green_adjust
[4 bytes] blue_adjust
[4 bytes] dissociate (0 or 1)
[4 bytes] exclude (0 or 1)
[4 bytes] exclude_color (0x00BBGGRR)
[4 bytes] distance (0-255)
```

### Dot Grid (Index 17)

```
[4 bytes] num_colors (1-16)
[4 bytes × num_colors] colors
[4 bytes] spacing (pixels between dots)
[4 bytes] x_move (sub-pixel movement per frame)
[4 bytes] y_move
[4 bytes] blend_mode
```

### Dynamic Movement (Index 43)

```
[string] init code
[string] frame code
[string] beat code
[string] point code (x/y transformation)
[4 bytes] grid_w
[4 bytes] grid_h
[4 bytes] blend (0 or 1)
[4 bytes] wrap (0 or 1)
[4 bytes] rectangular_coords (0=polar, 1=rectangular)
```

## Color Format

Colors are stored as 32-bit integers in **BGR** format (Windows COLORREF):
```
0x00BBGGRR
```

- Bits 0-7: Red
- Bits 8-15: Green
- Bits 16-23: Blue
- Bits 24-31: Usually 0 (not alpha)

## Loading Algorithm

```
1. Read and validate 25-byte header
2. Read root mode byte
3. While not EOF:
   a. Read effect_index (4 bytes)
   b. If effect_index >= 16384: read string_id (32 bytes)
   c. Read config_length (4 bytes)
   d. Read config_data (config_length bytes)
   e. Look up effect factory by index or string_id
   f. Create effect instance
   g. Call effect.load_config(config_data)
   h. If Effect List: recursively load child effects
   i. Add effect to chain
```

## Example Hex Dump

A minimal preset with just a Clear effect:

```
00000000: 4e75 6c6c 736f 6674 2041 5653 2050 7265  Nullsoft AVS Pre
00000010: 7365 7420 302e 321a 00                   set 0.2..
          ^^                                       root mode
00000019: 1900 0000                                effect_index = 25 (Clear)
0000001d: 1000 0000                                config_length = 16
00000021: 0100 0000                                enabled = 1
00000025: 0000 0000                                onbeat = 0
00000029: 0000 0000                                color = black
0000002d: 0000 0000                                blend_mode = replace
```

## References

- `r_list.cpp` - Effect list load/save (lines 64-207)
- `rlib.cpp` - Effect registration and factory (lines 109-157)
- `r_sscope.cpp` - SuperScope serialization example (lines 79-130)
- `r_defs.h` - PUT_INT/GET_INT macros
