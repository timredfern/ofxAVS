# Plugin System Design

## Current Status
Built-in effects use simple string names (e.g., "brightness", "clear", "oscilloscope") registered directly in the PluginManager. Each effect is self-contained with no dependencies.

## Future Plugin Architecture

### Directory-Based Namespacing
Plugins will be organized by directory structure to prevent naming conflicts:

```
effects/
├── builtin/
│   ├── brightness
│   ├── clear
│   └── oscilloscope
├── plugins/
│   ├── pack1/
│   │   ├── brightness      # Different "brightness" effect
│   │   └── wobble
│   └── company/
│       ├── premium/
│       │   └── blur
│       └── free/
│           └── distort
```

Registration uses full paths as effect IDs:
- `builtin/brightness`
- `plugins/pack1/brightness`  
- `plugins/company/premium/blur`

### Plugin Reference Problem
When plugins reference other plugins (for multi-input effects, dependencies, etc.), directory names can change but references must remain stable.

**Problem:** If user renames `plugins/mypack/` to `plugins/my_pack/`, references break:
```cpp
dependencies = {"mypack/brightness", "otherpack/blur"};  // Broken!
```

### Proposed Solution: Package Manifests
Each plugin directory contains a manifest file with stable naming:

```json
// plugins/some-folder/package.json
{
  "name": "mypack",           // Stable name for references
  "version": "1.0.0",
  "author": "Developer Name",
  "description": "Collection of visual effects",
  "effects": [
    {
      "name": "brightness",
      "class": "MyBrightnessEffect",
      "dependencies": ["com.company.basepack/utils"]
    }
  ]
}
```

**Benefits:**
- Directory structure prevents file conflicts
- Manifest provides stable names for inter-plugin references
- Allows versioning and metadata
- Enables dependency management

### Alternative Approaches Considered

1. **Namespace per file** - Can still have clashes within same namespace
2. **Hardcoded qualified names** - Not dynamic at runtime  
3. **Plugin metadata with namespace field** - Less intuitive than directories

### Implementation Notes
- Plugin loading will scan directories recursively
- Manifest parsing determines stable plugin names
- Effect registration uses `stable_name/effect_name` format
- Dependency resolution happens at plugin load time

### Multi-Input/Output Effects
Future effects with multiple inputs/outputs will benefit from this system:
- Masking effects: `inputs: ["source", "mask"]`
- Channel splitters: `outputs: ["red", "green", "blue"]`  
- Plugin references: `dependencies: ["basepack/utilities"]`

The stable naming system ensures these complex effect networks remain functional regardless of directory organization.