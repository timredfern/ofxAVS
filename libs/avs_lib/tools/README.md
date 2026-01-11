# avs_lib Tools

Command-line utilities for working with AVS presets.

## avs2json

Converts binary AVS presets (.avs) to JSON format.

### Building

```bash
cd libs/avs_lib/tools
mkdir build && cd build
cmake ..
make
```

### Usage

```bash
# Convert to file
./avs2json input.avs output.json

# Print to stdout
./avs2json input.avs
```

### Output Format

```json
{
  "version": "1.0",
  "format": "avs-json",
  "clear_each_frame": false,
  "effects": [
    {
      "type": "SuperScope",
      "enabled": true,
      "params": {
        "init_script": "n=100",
        "frame_script": "t=t+0.1",
        "point_script": "x=i*2-1; y=v",
        "color_0": "#FFFFFFFF"
      }
    }
  ]
}
```

### Notes

- Unsupported effects appear as `"type": "Unsupported: Effect Name"`
- Colors are stored as hex strings: `"#AARRGGBB"`
- Effect Lists with children are nested in the `"effects"` array
