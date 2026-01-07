# Preset Implementation Plan

## Milestone 1: Session Persistence

Automatically save/load the effect chain between application runs using JSON format.

### Phase 1: Core Infrastructure

#### 1.1 Add legacy_index to PluginInfo
**File:** `libs/avs_lib/core/plugin_manager.h`

```cpp
struct PluginInfo {
    std::string name;
    std::string category;
    std::string description;
    std::string author;
    int version;
    int legacy_index = -1;  // ADD: -1 = no legacy support
    std::function<std::unique_ptr<EffectBase>()> factory;
    std::vector<std::vector<ControlDef>> ui_layout;
};
```

#### 1.2 Add legacy indices to existing effects
**Files:** Each effect's `.cpp` file

| Effect | File | legacy_index |
|--------|------|--------------|
| Clear | clear_effect.cpp | 25 |
| OnBeatClear | onbeat_clear_effect.cpp | 5 |
| Brightness | brightness_effect.cpp | 22 |
| SuperScope | superscope_effect.cpp | 36 |
| DotGrid | dot_grid_effect.cpp | 17 |
| DotFountain | dot_fountain_effect.cpp | 19 |
| EffectList | effect_list.cpp | -2 (LIST_ID = 0xFFFFFFFE) |

#### 1.3 Add index lookup to PluginManager
**File:** `libs/avs_lib/core/plugin_manager.h`

```cpp
class PluginManager {
public:
    // Existing...

    // ADD: Lookup by legacy index
    const PluginInfo* get_by_legacy_index(int index) const;

    // ADD: Get legacy index for effect name
    int get_legacy_index(const std::string& name) const;
};
```

### Phase 2: JSON Serialization

#### 2.1 Create preset_json.h/.cpp
**Files:** `libs/avs_lib/core/preset_json.h`, `libs/avs_lib/core/preset_json.cpp`

```cpp
namespace avs {

class PresetJson {
public:
    // Save effect chain to JSON file
    static bool save(const std::string& path, const EffectContainer& root);

    // Load effect chain from JSON file
    static bool load(const std::string& path, EffectContainer& root);

    // String versions for flexibility
    static std::string to_json_string(const EffectContainer& root);
    static bool from_json_string(const std::string& json, EffectContainer& root);

private:
    // Serialize single effect (recursive for containers)
    static std::string effect_to_json(const EffectBase* effect, int indent);

    // Deserialize single effect
    static std::unique_ptr<EffectBase> json_to_effect(/* json node */);

    // Parameter serialization (generic, walks parameter map)
    static std::string params_to_json(const ParameterMap& params);
    static void json_to_params(/* json node */, ParameterMap& params);
};

} // namespace avs
```

#### 2.2 JSON Format Specification

```json
{
  "version": "1.0",
  "format": "avs-json",
  "created": "2025-01-07T12:00:00Z",
  "effects": [
    {
      "type": "Clear",
      "enabled": true,
      "params": {
        "color": 0,
        "blend_mode": 0
      }
    },
    {
      "type": "SuperScope",
      "enabled": true,
      "params": {
        "init_code": "n=800",
        "frame_code": "t=t-0.05",
        "beat_code": "",
        "point_code": "d=i+v*0.2; r=t+i*$PI*4; x=cos(r)*d; y=sin(r)*d",
        "num_colors": 1,
        "color_0": 16777215,
        "draw_mode": 1,
        "audio_channel": 2,
        "audio_source": 0
      }
    },
    {
      "type": "EffectList",
      "enabled": true,
      "params": {
        "blend_mode": 1,
        "clear_fb": false
      },
      "effects": []
    }
  ]
}
```

#### 2.3 Minimal JSON Parser
**Option A:** Header-only library (nlohmann/json, ~500KB single header)
**Option B:** Simple hand-rolled parser (JSON subset we need is small)
**Option C:** Use existing dependency if ofxAVS already has one

Recommend **Option B** for avs_lib to stay dependency-free. Only need:
- Object: `{ "key": value, ... }`
- Array: `[ value, ... ]`
- String: `"text"`
- Number: integers
- Bool: `true`/`false`

**File:** `libs/avs_lib/core/json_utils.h/.cpp`

### Phase 3: Integration

#### 3.1 Add to EffectListRoot
**File:** `libs/avs_lib/effects/effect_list_root.h`

```cpp
class EffectListRoot : public EffectContainer {
public:
    // Existing...

    // ADD: Convenience methods
    bool save_preset(const std::string& path);
    bool load_preset(const std::string& path);
};
```

#### 3.2 Auto-save in ofxAVS
**File:** `src/ofxAVS.cpp`

```cpp
void ofxAVS::setup() {
    // Existing setup...

    // Load last session
    std::string session_path = ofFilePath::getUserHomeDir() + "/.avs_session.json";
    if (ofFile::doesFileExist(session_path)) {
        renderer->root().load_preset(session_path);
    }
}

ofxAVS::~ofxAVS() {
    // Save session on exit
    std::string session_path = ofFilePath::getUserHomeDir() + "/.avs_session.json";
    renderer->root().save_preset(session_path);
}
```

### Phase 4: Testing

#### 4.1 Unit tests
**File:** `libs/avs_lib/tests/preset_json_test.cpp`

- Round-trip: create effects → save JSON → load JSON → verify parameters match
- Empty chain
- Nested EffectList
- All parameter types (int, bool, string, color)

#### 4.2 Manual testing
- Start app with effects, quit, restart → same effects appear
- Modify parameters, quit, restart → parameters preserved

---

## Task Checklist

### Phase 1: Core Infrastructure
- [ ] Add `legacy_index` field to `PluginInfo` struct
- [ ] Add `legacy_index` to each existing effect's `effect_info`
- [ ] Add `get_by_legacy_index()` to `PluginManager`
- [ ] Add `get_legacy_index()` to `PluginManager`

### Phase 2: JSON Serialization
- [ ] Create `json_utils.h/.cpp` (minimal JSON read/write)
- [ ] Create `preset_json.h/.cpp`
- [ ] Implement `params_to_json()` - generic parameter serialization
- [ ] Implement `json_to_params()` - generic parameter deserialization
- [ ] Implement `effect_to_json()` - single effect with recursion
- [ ] Implement `json_to_effect()` - create effect and populate params
- [ ] Implement `save()` - write to file
- [ ] Implement `load()` - read from file

### Phase 3: Integration
- [ ] Add `save_preset()` / `load_preset()` to `EffectListRoot`
- [ ] Add session auto-load in `ofxAVS::setup()`
- [ ] Add session auto-save in `ofxAVS` destructor

### Phase 4: Testing
- [ ] Add preset JSON unit tests
- [ ] Manual testing of session persistence

---

## Milestone 2: Legacy .avs Loading (Future)

After Milestone 1 is complete:

- [ ] Create `preset_legacy.h/.cpp`
- [ ] Create `unknown_effect.h/.cpp` for unrecognized effects
- [ ] Implement binary parsing (header, effect entries)
- [ ] Add `load_legacy_config()` to each effect
- [ ] Test with original AVS preset files

---

## Milestone 3: Full Preset UI (Future)

- [ ] Load/Save file dialogs in UI
- [ ] Preset browser panel
- [ ] Recent presets list
- [ ] Preset metadata (author, description)
