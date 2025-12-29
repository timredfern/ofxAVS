# AVS Global Parameters and UI

This documents the non-effect UI elements from original AVS - global settings, preset management, beat detection, etc.

## Overview

The original AVS had several configuration dialogs accessible from the main editor menu:
- **Display** (IDM_DISPLAY) - Windowed and overlay settings
- **Fullscreen** (IDM_FULLSCREEN submenu) - Fullscreen mode configuration
- **Presets** (IDM_PRESETS) - Preset loading/saving and random rotation
- **BPM** (IDM_BPM) - Beat detection settings
- **Transitions** (IDM_TRANSITIONS) - Preset transition animations
- **Debug** (IDM_HELP_DEBUGWND) - Expression debugging console

---

## 1. Main Editor Window (IDD_DIALOG1)

**Source:** `cfgwin.cpp` → `dlgProc()` (line 993)

**Resource ID:** 101

The main AVS editor containing the effect tree and effect-specific parameter panels.

### Controls

| ID | Control | Type | Purpose |
|----|---------|------|---------|
| IDC_TREE1 (1079) | Effect Tree | TreeView | Hierarchical effect list |
| IDC_EFFECTRECT (1006) | Effect Panel | Static | Container for effect-specific UI |
| IDC_EFNAME (1085) | Effect Name | GroupBox | Shows selected effect name |
| IDC_ADD (1015) | Add | Button | Add new effect |
| IDC_REMSEL (1010) | Remove | Button | Remove selected effect |
| IDC_CLONESEL (1046) | Clone | Button | Duplicate selected effect |
| IDC_FPS (1078) | FPS | Static | Framerate display |

### Menu (IDR_MENU1 = 140)

| ID | Menu Item |
|----|-----------|
| IDM_DISPLAY (40001) | Display settings |
| IDM_PRESETS (40002) | Preset management |
| IDM_BPM (40005) | Beat detection |
| IDM_TRANSITIONS (40006) | Transitions |
| IDM_FULLSCREEN (40007) | Fullscreen toggle |
| IDM_HELP_DEBUGWND (40008) | Debug window |
| IDM_UNDO (40009) | Undo |
| IDM_REDO (40010) | Redo |
| IDM_ABOUT (40004) | About dialog |

---

## 2. Display Settings (IDD_GCFG_DISP)

**Source:** `cfgwin.cpp` → `DlgProc_Disp()` (line 338)

**Resource ID:** 112

### Windowed Mode

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_CHECK6 (1083) | Wait for retrace | Checkbox | Off | `cfg_fs_d` bit |
| IDC_CHECK1 (1072) | Pixel doubling | Checkbox | Off | `cfg_fs_d` bit |
| IDC_SLIDER1 (1055) | Performance | Slider | - | `cfg_speed` (low byte) |
| IDC_CHECK3 (1122) | Suppress status | Checkbox | Off | `cfg_fs_fps` bit |
| IDC_CHECK5 (1125) | Suppress title | Checkbox | Off | `cfg_fs_fps` bit |
| IDC_CHECK2 (1117) | Reuse on resize | Checkbox | Off | `config_reuseonresize` |

### Transparency (Win2k+)

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_TRANS_CHECK (1144) | Enable transparency | Checkbox | Off | `cfg_trans` |
| IDC_TRANS_SLIDER (1142) | Transparency amount | Slider | 255 | `cfg_trans_amount` (16-255) |

### Overlay Mode

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_BKGND_RENDER (1092) | Enable overlay | Checkbox | Off | `cfg_bkgnd_render` |
| IDC_OVERLAYCOLOR (1094) | Overlay color | Button | - | `cfg_bkgnd_render_color` |
| IDC_DEFOVERLAYCOLOR (1095) | Default color | Button | - | - |
| IDC_SETDESKTOPCOLOR (1093) | Set desktop | Button | - | - |

### Thread Priority

| ID | Control | Type | Options | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_THREAD_PRIORITY (1100) | Priority | Combo | Same/Idle/Lowest/Normal/Highest | `cfg_render_prio` |

### SMP (Multi-processor)

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_CHECK4 (1126) | Enable SMP | Checkbox | Off | `g_config_smp` |
| IDC_EDIT1 (1089) | Thread count | Edit | - | `g_config_smp_mt` |

---

## 3. Fullscreen Settings (IDD_GCFG_FS)

**Source:** `cfgwin.cpp` → `DlgProc_FS()` (line 593)

**Resource ID:** 159

### Display Mode

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_COMBO1 (1093) | Mode | Combo | Auto-populated | `cfg_fs_w`, `cfg_fs_h`, `cfg_fs_bpp` |
| IDC_USE_OVERLAY (1099) | Use overlay | Checkbox | Off | `cfg_fs_use_overlay` |

### Rendering Options

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_CHECK1 (1072) | Double-buffer | Checkbox | On | `cfg_fs_d` bit |
| IDC_CHECK2 (1117) | VSync | Checkbox | Off | `cfg_fs_d` bit |
| IDC_CHECK4 (1126) | Flip display | Checkbox | Off | `cfg_fs_flip` bit |
| IDC_CHECK7 (1084) | Normal order | Checkbox | On | `cfg_fs_flip` bit (inverted) |
| IDC_BPP_CONV (1086) | Color convert | Checkbox | Off | `cfg_fs_flip` bit |

### Display

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_CHECK3 (1122) | Show FPS | Checkbox | Off | `cfg_fs_fps` bit |
| IDC_CHECK5 (1125) | Show title | Checkbox | Off | `cfg_fs_fps` bit |
| IDC_EDIT1 (1089) | Height % | Edit | 100 | `cfg_fs_height` |
| IDC_SLIDER1 (1055) | Performance | Slider | - | `cfg_speed` (high byte) |

### Behavior

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_CHECK8 (1073) | Render when inactive | Checkbox | Off | `cfg_cancelfs_on_deactivate` |
| IDC_CHECK6 (1083) | Double-click toggle | Checkbox | On | `cfg_fs_dblclk` |
| IDC_BUTTON1 (1063) | Enter fullscreen | Button | - | - |

---

## 4. Preset Management (IDD_GCFG_PRESET)

**Source:** `cfgwin.cpp` → `DlgProc_Preset()` (line 213)

**Resource ID:** 123

### Hotkey Presets

| ID | Control | Type | Purpose |
|----|---------|------|---------|
| IDC_COMBO1 (1093) | Slot | Combo | F1-F12, 0-9, Shift+0-9 |
| IDC_BUTTON1 (1063) | Save | Button | Save to selected slot |
| IDC_BUTTON2 (1129) | Load | Button | Load from selected slot |

### Behavior

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_CHECK1 (1072) | Prompt save on load | Checkbox | Off | `config_prompt_save_preset` |

### Random Presets

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_CHECK3 (1122) | Enable random | Checkbox | Off | `cfg_fs_rnd` |
| IDC_EDIT1 (1089) | Rotation time (sec) | Edit | 30 | `cfg_fs_rnd_time` |
| IDC_BUTTON3 (1073) | Subdirectory | Button | Shows current | `config_pres_subdir` |

### Associated Functions

| Function | File | Purpose |
|----------|------|---------|
| `dosavePreset()` | cfgwin.cpp:856 | File save dialog |
| `LoadPreset()` | wnd.cpp | Load preset from file |
| `WritePreset()` | wnd.cpp | Save preset to file |
| `next_preset()` | wnd.cpp | Cycle to next |
| `previous_preset()` | wnd.cpp | Cycle to previous |
| `random_preset()` | wnd.cpp | Load random |

---

## 5. Beat Detection / BPM (IDD_GCFG_BPM)

**Source:** `bpm.cpp` → `DlgProc_Bpm()` (line 106)

**Resource ID:** 145

### Mode Selection

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_BPMSTD (1150) | Standard | Radio | Off | `cfg_smartbeat` = 0 |
| IDC_BPMADV (1152) | Advanced | Radio | On | `cfg_smartbeat` = 1 |

### Behavior

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_STICKY (1154) | Sticky beats | Checkbox | Off | `cfg_smartbeatsticky` |
| IDC_ONLYSTICKY (1155) | Only sticky | Checkbox | Off | `cfg_smartbeatonlysticky` |
| IDC_NEWRESET (1112) | Reset on new song | Radio | - | `cfg_smartbeatresetnewsong` |
| IDC_NEWADAPT (1074) | Adapt on new song | Radio | - | `cfg_smartbeatresetnewsong` |

### Beat Adjustment

| ID | Control | Type | Purpose |
|----|---------|------|---------|
| IDC_2X (1163) | 2x | Button | Double beat frequency |
| IDC_DIV2 (1164) | /2 | Button | Half beat frequency |
| IDC_RESET (1166) | Reset | Button | Reset detection |

### Sensitivity

| ID | Control | Type | Range | Variable |
|----|---------|------|-------|----------|
| IDC_IN (1175) | Input | Slider | 0-8 | `inSlide` |
| IDC_OUT (1176) | Output | Slider | 0-8 | `outSlide` |

### Status Display (Read-only)

| ID | Control | Purpose | Variable |
|----|---------|---------|----------|
| IDC_BPM (1161) | BPM value | `predictionBpm` |
| IDC_CONFIDENCE (1162) | Confidence % | `Confidence` |
| IDC_STICK (1167) | Stick button | When BPM available |
| IDC_UNSTICK (1168) | Unstick button | When locked |

---

## 6. Transitions (IDD_GCFG_TRANSITIONS)

**Source:** `cfgwin.cpp` (transitions dialog)

**Resource ID:** 155

### Pre-initialization

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_CHECK3 (1122) | On random preset | Checkbox | - | `cfg_transitions2` bit |
| IDC_CHECK11 (1124) | On next/prev | Checkbox | - | `cfg_transitions2` bit |
| IDC_CHECK10 (1123) | On load preset | Checkbox | - | `cfg_transitions2` bit |
| IDC_CHECK4 (1126) | Low priority | Checkbox | - | `cfg_transitions2` bit |
| IDC_CHECK5 (1125) | Only in fullscreen | Checkbox | - | `cfg_transitions2` bit |

### Transition Animation

| ID | Control | Type | Default | Config Variable |
|----|---------|------|---------|-----------------|
| IDC_CHECK8 (1073) | On random preset | Checkbox | - | `cfg_transitions` bit |
| IDC_CHECK1 (1072) | On next/prev | Checkbox | - | `cfg_transitions` bit |
| IDC_CHECK2 (1117) | On load preset | Checkbox | - | `cfg_transitions` bit |
| IDC_CHECK9 (1118) | Keep old rendering | Checkbox | - | `cfg_transitions` bit |

### Transition Control

| ID | Control | Type | Range | Config Variable |
|----|---------|------|-------|-----------------|
| IDC_TRANSITION (1087) | Type | Combo | Various | `cfg_transition_mode` |
| IDC_SPEED (1048) | Speed | Slider | 250ms - 8s | `cfg_transitions_speed` |

---

## 7. Debug Window (IDD_DEBUG)

**Source:** `cfgwin.cpp` → `debugProc()` (line 893)

**Resource ID:** 167

### Debug Registers

8 register pairs for monitoring NSEEL global variables:

| Input ID | Display ID | Purpose |
|----------|------------|---------|
| IDC_DEBUGREG_1 (1186) | IDC_DEBUGREG_2 (1187) | Register 1 |
| IDC_DEBUGREG_3 (1188) | IDC_DEBUGREG_4 (1189) | Register 2 |
| IDC_DEBUGREG_5 (1190) | IDC_DEBUGREG_6 (1191) | Register 3 |
| IDC_DEBUGREG_7 (1192) | IDC_DEBUGREG_8 (1193) | Register 4 |
| IDC_DEBUGREG_9 (1194) | IDC_DEBUGREG_10 (1195) | Register 5 |
| IDC_DEBUGREG_11 (1196) | IDC_DEBUGREG_12 (1197) | Register 6 |
| IDC_DEBUGREG_13 (1198) | IDC_DEBUGREG_14 (1199) | Register 7 |
| IDC_DEBUGREG_15 (1200) | IDC_DEBUGREG_16 (1201) | Register 8 |

### Settings

| ID | Control | Type | Purpose |
|----|---------|------|---------|
| IDC_CHECK1 (1072) | Log errors | Checkbox | Log eval code errors |
| IDC_CHECK2 (1117) | Reset vars | Checkbox | Reset variables on recompile |
| IDC_CHECK3 (1122) | Enable SEH | Checkbox | Structured exception handling |

### Output

| ID | Control | Type | Purpose |
|----|---------|------|---------|
| IDC_EDIT1 (1089) | Last error | Edit | Error string display |
| IDC_EDIT2 (1090) | Statistics | Edit | Code stats (segments, bytes) |
| IDC_BUTTON1 (1063) | Clear | Button | Clear errors |

---

## 8. Main Visualization Window

**Source:** `wnd.cpp` → `WndProc()` (line 1108)

**Window Class:** "avswnd"

### Mouse Actions

| Action | Result |
|--------|--------|
| Right-click | Popup preset menu |
| Left-click | Toggle editor or docking |
| Double-click | Toggle fullscreen (if enabled) |
| Drag-drop | Load .avs preset file |

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Space | Random preset |
| U | Next preset |
| Y | Previous preset |
| Escape | Exit fullscreen |
| Alt+Enter | Toggle fullscreen |
| F | Toggle FPS counter |
| R | Toggle random presets |
| F1-F12 | Load preset (Ctrl+: save) |
| 0-9 | Load preset (Ctrl+: save) |
| Shift+0-9 | Load preset (Ctrl+Shift+: save) |

### Window State Variables

| Variable | Purpose |
|----------|---------|
| `cfg_x`, `cfg_y` | Window position |
| `cfg_w`, `cfg_h` | Window size |
| `cfg_cfgwnd_open` | Editor visibility |
| `cfg_cfgwnd_x`, `cfg_cfgwnd_y` | Editor position |
| `inWharf` | Docked in Winamp |

---

## 9. About Dialog (IDD_DIALOG2)

**Source:** `main.cpp` → `aboutProc()` (line 123)

**Resource ID:** 127

| ID | Control | Purpose |
|----|---------|---------|
| IDC_VERSTR (1130) | Version | Version string display |

Shown when clicking "Config" in Winamp without main window open.

---

## Dialog Resource ID Summary

| ID | Macro | Purpose |
|----|-------|---------|
| 101 | IDD_DIALOG1 | Main editor |
| 112 | IDD_GCFG_DISP | Display settings |
| 123 | IDD_GCFG_PRESET | Preset management |
| 127 | IDD_DIALOG2 | About dialog |
| 145 | IDD_GCFG_BPM | Beat detection |
| 155 | IDD_GCFG_TRANSITIONS | Transitions |
| 159 | IDD_GCFG_FS | Fullscreen settings |
| 167 | IDD_DEBUG | Debug console |
| 140 | IDR_MENU1 | Main menu |

---

## Configuration Variable Summary

### Windowed Display
- `cfg_speed` (low byte) - Windowed performance
- `cfg_fs_d` - Pixel doubling, retrace wait
- `cfg_fs_fps` - Status/title suppression
- `cfg_trans` - Transparency enabled
- `cfg_trans_amount` - Transparency value (16-255)
- `config_reuseonresize` - Reuse image on resize

### Fullscreen
- `cfg_fs_w`, `cfg_fs_h`, `cfg_fs_bpp` - Display mode
- `cfg_fs_use_overlay` - Overlay mode
- `cfg_fs_d` - Double-buffer, VSync
- `cfg_fs_flip` - Flip, color conversion
- `cfg_fs_fps` - FPS/title display
- `cfg_fs_height` - Height percentage
- `cfg_speed` (high byte) - Fullscreen performance
- `cfg_cancelfs_on_deactivate` - Deactivation behavior
- `cfg_fs_dblclk` - Double-click toggle

### Overlay
- `cfg_bkgnd_render` - Overlay enabled
- `cfg_bkgnd_render_color` - Overlay color

### Threading
- `cfg_render_prio` - Thread priority
- `g_config_smp` - SMP enabled
- `g_config_smp_mt` - Thread count

### Presets
- `config_prompt_save_preset` - Save prompt
- `cfg_fs_rnd` - Random presets enabled
- `cfg_fs_rnd_time` - Rotation time (seconds)
- `config_pres_subdir` - Subdirectory
- `last_preset` - Last loaded path

### Beat Detection
- `cfg_smartbeat` - Advanced mode enabled
- `cfg_smartbeatsticky` - Sticky beats
- `cfg_smartbeatresetnewsong` - Reset on new song
- `cfg_smartbeatonlysticky` - Only sticky beats

### Transitions
- `cfg_transitions` - Transition enable flags
- `cfg_transitions2` - Pre-init flags
- `cfg_transitions_speed` - Duration
- `cfg_transition_mode` - Type

---

## Audio Data Format

**Source:** `main.cpp` lines 79-80, 266-331 and `VIS.H`

### Winamp VIS Module Structure

Winamp passes audio data to visualization plugins via the `winampVisModule` structure:

```cpp
typedef struct winampVisModule {
    // ... other fields ...
    int sRate;              // Sample rate (e.g., 44100)
    int nCh;                // Number of audio channels
    int spectrumNch;        // Number of spectrum channels (typically 2)
    int waveformNch;        // Number of waveform channels (typically 2)

    unsigned char spectrumData[2][576];   // FFT spectrum data
    unsigned char waveformData[2][576];   // Raw audio waveform
} winampVisModule;
```

### AVS Internal Audio Buffer

AVS maintains its own processed audio buffer:

```cpp
static unsigned char g_visdata[2][2][576];
// [0] = spectrum data, [1] = waveform data
// [0] = left channel,  [1] = right channel
// [576] = samples per channel

static int g_visdata_pstat;  // Peak status flag
```

### Audio Processing Pipeline

**Source:** `main.cpp` → `render()` (line 266)

1. **Spectrum Processing** (with log compression):
```cpp
// Log table for dynamic range compression (initialized in init())
for (x = 0; x < 256; x++) {
    double a = log(x * 60.0 / 255.0 + 1.0) / log(60.0);
    int t = (int)(a * 255.0);
    g_logtab[x] = (unsigned char)t;
}

// Apply log compression to spectrum data
if (g_visdata_pstat)
    // Normal mode: direct compression
    g_visdata[0][0][x] = g_logtab[(unsigned char)this_mod->spectrumData[0][x]];
else {
    // Peak hold mode: only increase values
    int t = g_logtab[(unsigned char)this_mod->spectrumData[0][x]];
    if (g_visdata[0][0][x] < t)
        g_visdata[0][0][x] = t;
}
```

2. **Waveform Processing** (direct copy):
```cpp
memcpy(&g_visdata[1][0][0], this_mod->waveformData, 576 * 2);
```

3. **Beat Detection** (from waveform energy):
```cpp
// Calculate energy per channel
for (ch = 0; ch < 2; ch++) {
    unsigned char *f = (unsigned char*)&this_mod->waveformData[ch][0];
    for (x = 0; x < 576; x++) {
        int r = *f++ ^ 128;  // Convert unsigned to signed
        r -= 128;
        if (r < 0) r = -r;
        lt[ch] += r;
    }
}

// Beat detection threshold
if (lt[0] >= (beat_peak1 * 34) / 32 && lt[0] > (576 * 16)) {
    avs_beat = 1;  // Beat detected
}
```

### Data Format Details

| Field | Format | Range | Notes |
|-------|--------|-------|-------|
| `spectrumData[ch][i]` | unsigned char | 0-255 | FFT magnitude, 576 bins per channel |
| `waveformData[ch][i]` | unsigned char | 0-255 | Raw PCM, XOR with 128 for signed |
| `g_logtab[x]` | unsigned char | 0-255 | Log compression: `log(x*60/255+1)/log(60)*255` |

### Effect Access

Effects receive audio data through the `AudioData` typedef:

```cpp
typedef char AudioData[2][2][576];
// AudioData[0][ch][i] = spectrum (unsigned 0-255)
// AudioData[1][ch][i] = waveform (signed, XOR 128 for unsigned)
```

---

## Preset File Format

**Source:** `r_list.cpp` → `__SavePreset()`, `__LoadPreset()` (lines 1218-1283)

### File Signature

```cpp
char sig_str[] = "Nullsoft AVS Preset 0.2\x1a";
```

- Files must start with this 25-byte signature
- Version byte (position 22) can be '1' or '2'
- Ends with `0x1a` (Ctrl+Z EOF marker)

### File Structure

```
[Signature: 25 bytes] "Nullsoft AVS Preset 0.2\x1a"
[Effect List Config: variable length]
```

### Effect List Serialization

**Source:** `r_list.cpp` → `save_config_ex()` (line 136)

#### Root List Header (1 byte)
```cpp
data[pos++] = mode;  // Blend mode byte
```

#### Nested List Header (extended format)
```cpp
data[pos++] = (mode & 0xff) | 0x80;  // Mode with extended flag
PUT_INT(mode);           // Full mode value (4 bytes)
PUT_INT(inblendval);     // Input blend (4 bytes)
PUT_INT(outblendval);    // Output blend (4 bytes)
PUT_INT(bufferin);       // Input buffer (4 bytes)
PUT_INT(bufferout);      // Output buffer (4 bytes)
PUT_INT(ininvert);       // Input invert (4 bytes)
PUT_INT(outinvert);      // Output invert (4 bytes)
PUT_INT(beat_render);    // Beat render flag (4 bytes)
PUT_INT(beat_render_frames); // Beat frames (4 bytes)
```

#### Extended Data Block (for effect list code)
```cpp
PUT_INT(DLLRENDERBASE);  // Marker for extended data
[32 bytes: "AVS 2.8+ Effect List Config\0..."]
PUT_INT(code_length);    // Length of code block
[code_length bytes: init/frame expressions]
```

#### Effect Entries
For each effect in the list:

```cpp
// Effect index (4 bytes)
PUT_INT(effect_index);

// If DLL/APE effect (index >= DLLRENDERBASE):
[32 bytes: effect identifier string]

// Effect config length (4 bytes)
PUT_INT(config_length);

// Effect-specific config data
[config_length bytes: effect parameters]
```

### Effect Index Values

| Range | Type |
|-------|------|
| 0-255 | Built-in effects (see r_defs.h) |
| >= DLLRENDERBASE | DLL/APE third-party effects |
| UNKN_ID | Unknown/unsupported effect |

### Byte Order

All integers are little-endian (x86):
```cpp
#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; \
                   data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
```

### Loading Process

1. Verify signature (first 25 bytes)
2. Check version byte ('1' or '2')
3. Call `load_config()` on remaining data
4. Recursively load nested effect lists

### Saving Process

1. Allocate 1MB buffer
2. Write signature
3. Call `save_config_ex()` to serialize effect tree
4. Write buffer to file

### Undo System

**Source:** `undo.cpp`, `undo.h`

The undo system uses the same serialization format:

```cpp
class C_UndoItem {
    void *data;      // Serialized preset data
    int length;      // Data length
    bool isdirty;    // Modified since save
};

class C_UndoStack {
    static C_UndoItem *list[256];  // Circular buffer
    static int list_pos;           // Current position
};
```

Functions:
- `__SavePresetToUndo()` - Serialize current state to undo item
- `__LoadPresetFromUndo()` - Restore state from undo item

---

## Built-in Effect IDs

**Source:** `render.cpp` (effect registration)

Effects are registered by index. The index in the preset file maps to a specific effect type. See `r_defs.h` for the complete list of effect IDs.

Common effect indices:
- 0 = Simple oscilloscope
- 1 = Ring oscilloscope
- 2 = ...etc

APE (AVS Plugin Effect) third-party effects use string identifiers stored as 32-byte null-padded strings after the DLLRENDERBASE marker.
