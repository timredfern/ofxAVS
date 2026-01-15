# Building ofxAVS

## Prerequisites

- OpenFrameworks 0.12+
- C++17 compiler (Xcode Command Line Tools on macOS)
- Required addons:
  - [ofxImGui](https://github.com/jvcleave/ofxImGui) - UI rendering
  - [ofxFft](https://github.com/kylemcdonald/ofxFft) - FFT audio processing
  - [ofxAudioDecoder](https://github.com/kylemcdonald/ofxAudioDecoder) - Audio file playback

## Environment Setup

Set the `OF_ROOT` environment variable to your OpenFrameworks installation:

```bash
export OF_ROOT=/path/to/openFrameworks
```

Add this to your shell profile (`~/.zshrc` or `~/.bashrc`) for persistence.

## Building Examples

### Chain Example (Full UI)

```bash
cd examples/chain
make            # Debug build
make Release    # Release build
make run        # Run after building
```

### Simple Example (Minimal)

```bash
cd examples/simple
make
make run
```

## Make Targets

Run `make help` in any example directory:

```
Available targets:
  make                 - Build debug version
  make Release         - Build release version
  make clean           - Clean build artifacts
  make help            - Show this help

Resources:
  make copy-resources  - Copy AVS resources to bin/data

Packaging:
  make package         - Build release + package + create DMG
  make package-only    - Package existing build (no rebuild)
  make run-release     - Open the packaged app
  make clean-release   - Remove release artifacts

Info:
  make app-info        - Show architectures and min OS version
```

## Building avs_lib Tests

The core library has its own test suite:

```bash
cd libs/avs_lib/tests/build
make && ./avs_tests
```

## Packaging for Distribution

Create a distributable macOS app:

```bash
cd examples/chain
make package
```

This creates:
- `bin/release/chain.app` - Signed application bundle
- `bin/release/chain.dmg` - Disk image with Applications shortcut

## Troubleshooting

**"No such file or directory" errors**
- Ensure `OF_ROOT` is set correctly
- Verify OpenFrameworks is installed at that path

**Missing addons**
- Clone required addons into `$OF_ROOT/addons/`:
  ```bash
  cd $OF_ROOT/addons
  git clone https://github.com/jvcleave/ofxImGui
  git clone https://github.com/kylemcdonald/ofxFft
  git clone https://github.com/kylemcdonald/ofxAudioDecoder
  ```

**Linker errors**
- Run `make clean` and rebuild
- Check that all addons are listed in `addons.make`
