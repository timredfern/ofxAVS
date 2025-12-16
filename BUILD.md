# Building ofxAVS

## Environment Setup

**CRITICAL**: Always set the OF_ROOT environment variable before building:

```bash
export OF_ROOT=/path/to/your/openFrameworks
```

### Why Environment Variables?

- Keeps the repository clean of filesystem-specific paths
- Allows different developers to use different openFrameworks locations
- Prevents accidental commits of hardcoded paths

### Building Examples

```bash
export OF_ROOT=/path/to/your/openFrameworks
cd example
make
```

### Building Tests

```bash
cd libs/avs_lib/tests
cmake -B build
cmake --build build
./build/avs_tests
```

## Common Issues

- **"No such file or directory" errors**: Set OF_ROOT environment variable
- **Build failures**: Ensure openFrameworks is properly installed and OF_ROOT points to the correct directory