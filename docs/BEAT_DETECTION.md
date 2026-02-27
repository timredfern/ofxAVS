# Beat Detection in AVS

## Architecture

Beat detection lives in **ofxAVS** (the host layer), not avs_lib (the rendering library).

```
┌─────────────────────────────────────────────────────────────┐
│ ofxAVS (host)                                               │
│                                                             │
│   Audio Input ──► FFT ──► BeatDetector ──► isBeat          │
│                    │           │                            │
│                    │      ┌────┴────┐                       │
│                    │      │ Classic │ (AudioData energy)    │
│                    │      │ Modern  │ (raw FFT flux)        │
│                    │      └────┬────┘                       │
│                    │           │                            │
│                    ▼           ▼                            │
│              AudioData      isBeat                          │
│                    │           │                            │
└────────────────────┼───────────┼────────────────────────────┘
                     │           │
                     ▼           ▼
┌─────────────────────────────────────────────────────────────┐
│ avs_lib (renderer)                                          │
│                                                             │
│   renderer->render(audioData, isBeat, framebuffer)          │
│                                                             │
│   Effects receive isBeat, react accordingly                 │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Why this design?

1. **Raw FFT access**: Modern onset detection needs unprocessed FFT magnitudes.
   By the time audio reaches avs_lib as `AudioData`, it's been log-compressed
   or dB-scaled, losing the magnitude information needed for spectral flux.

2. **Host flexibility**: Different hosts (ofxAVS, webAVS, etc.) may want
   different beat detection algorithms. The renderer just needs a boolean.

3. **Clean separation**: avs_lib is pure rendering with no audio analysis
   dependencies. It receives `isBeat` from the host.

## API

```cpp
class BeatDetector {
    // Classic mode: uses AudioData (energy-based, original AVS algorithm)
    bool process(const avs::AudioData& visdata);

    // Modern mode: uses raw FFT magnitudes (spectral flux)
    bool processModern(const float* amplitudeLeft,
                       const float* amplitudeRight,
                       int binSize);

    // State
    int getBpm() const;        // Current detected BPM (0 if unknown)
    int getConfidence() const; // 0-100%
    bool isSticky() const;     // Locked to tempo?

    // Manual adjustment
    void doubleBeat();  // Double the BPM
    void halfBeat();    // Halve the BPM
    void reset();       // Clear all state
};
```

## Classic Mode (Original AVS)

The original AVS algorithm from `bpm.cpp`, preserved for compatibility.

### Stage 1: Energy Detection

```cpp
// Sum absolute waveform values per channel
for (int x = 0; x < 576; x++) {
    energy += abs(waveform[channel][x]);
}

// Beat if energy exceeds decaying threshold
threshold = (peak * 34) / 32;  // 106.25% of peak
if (energy >= threshold && energy > minimum) {
    beat = true;
    peak = (energy + peak) / 2;  // Update peak
}
```

**Characteristics:**
- Simple energy-based detection
- Works on time-domain waveform
- Fast decay adapts to dynamics
- Can miss quiet beats, trigger on sustained notes

### Stage 2: BPM Tracking

```cpp
// Maintain history of beat times
// Calculate average interval
// Predict next beat based on tempo
// Suppress beats that don't align with prediction
```

**Parameters:**
- `MIN_BPM = 30`
- `MAX_BPM = 300`
- `HIST_SIZE = 8` beats for tempo calculation

## Modern Mode (Spectral Flux)

Improved detection using FFT magnitudes directly.

### Key Insight

A beat is an **onset** - a sudden increase in energy. The classic algorithm
measures absolute energy, but:

- A sustained note has high energy but no onset
- A quiet kick drum has low energy but clear onset
- We want to detect *changes*, not levels

### Algorithm

```cpp
// 1. Calculate spectral flux (sum of positive magnitude changes)
for (int bin = 1; bin < bassEnd; bin++) {
    float diff = amplitude[bin] - prevAmplitude[bin];
    if (diff > 0) {  // Half-wave rectification: only increases
        flux += diff;
    }
    prevAmplitude[bin] = amplitude[bin];
}

// 2. Adaptive threshold (mean + k * stddev over ~0.7 seconds)
threshold = mean(fluxHistory) + 1.5 * stddev(fluxHistory);

// 3. Onset if flux exceeds threshold
if (flux > threshold) {
    rawBeat = true;
}

// 4. Feed into BPM tracking (same as classic)
refinedBeat = refineBeat(rawBeat);
```

### Why Spectral Flux Works Better

| Scenario | Classic (Energy) | Modern (Flux) |
|----------|------------------|---------------|
| Quiet kick drum | Might miss (low energy) | Detects (energy spike) |
| Sustained chord | False positive (high energy) | No trigger (no change) |
| Hi-hat vs kick | Can't distinguish | Bass bins weighted |
| Dynamic range | Fixed decay struggles | Adaptive threshold |

### Frequency Bands

```cpp
// FFT bin to frequency: freq = bin * sampleRate / fftSize
// At 44100Hz with 512-sample FFT:
//   Bin 1  ≈ 86 Hz
//   Bin 4  ≈ 344 Hz
//   Bin 16 ≈ 1.4 kHz

BASS_END = 16;  // Focus on 0-1.4kHz for kick drums
```

### Half-Wave Rectification

Only positive changes (increases) count as potential onsets:

```cpp
if (diff > 0) {
    flux += diff;  // Energy increase = potential onset
}
// Negative diff ignored (energy decrease = note release, not onset)
```

### Adaptive Threshold

Instead of a fixed decay, we calculate threshold from recent history:

```cpp
threshold = mean + k * stddev;  // k = 1.5 typical
```

This adapts to both quiet and loud passages automatically.

## Usage in ofxAVS

Currently wired up in `ofxAVS::update()`:

```cpp
void ofxAVS::update() {
    // Classic mode uses AudioData
    bool isBeat = beat_detector_->process(current_audio_data);

    renderer->render(current_audio_data, isBeat, pixels);
}
```

To use modern mode, call from `audioIn()` where FFT is available:

```cpp
void ofxAVS::audioIn(ofSoundBuffer& buffer) {
    // ... FFT processing ...

    if (!audio_classic_mode_) {
        // Modern: use raw FFT magnitudes
        is_beat_ = beat_detector_->processModern(
            amplitudeLeft, amplitudeRight, binSize);
    }
}
```

## UI Controls

The beat detector has a configuration UI matching original AVS:

| Control | Description |
|---------|-------------|
| Standard/Advanced | Standard = raw detection, Advanced = BPM tracking |
| Input slider | Animates on raw beat detection |
| Output slider | Animates on refined beat output |
| Current BPM | Detected tempo |
| Confidence | How stable the detection is (0-100%) |
| Auto-keep | Lock to tempo when confident |
| 2x / /2 | Manual tempo adjustment |
| Reset | Clear all state |
| Keep/Readapt | Manual sticky toggle |

## Tuning

### Spectral Flux Parameters

```cpp
// In BeatDetector.cpp

// History size for adaptive threshold (~0.7s at 60fps)
static constexpr int FLUX_HISTORY_SIZE = 43;

// Threshold multiplier (higher = fewer false positives, may miss quiet beats)
float k = 1.5f;  // in calcAdaptiveThreshold()

// Bass frequency range (bins)
int bassEnd = 16;  // in calcSpectralFlux()
```

### BPM Tracking Parameters

```cpp
static constexpr int HIST_SIZE = 8;   // Beats for tempo calculation
static constexpr int MIN_BPM = 30;    // Minimum detectable BPM
static constexpr int MAX_BPM = 300;   // Maximum detectable BPM
```

## Future Improvements

1. **Multi-band detection**: Separate bass/mid/high onsets for different effects
2. **Phase-locked loop**: Smoother tempo tracking with prediction
3. **Transient detection**: Time-domain onset detection for sharper attacks
4. **Machine learning**: Train on labeled beat data (requires dependencies)
