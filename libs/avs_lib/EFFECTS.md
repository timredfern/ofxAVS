# AVS Effects Catalogue

A comprehensive catalogue of all AVS effects with their inputs, outputs, blend modes, and controls.

## Render Effects
Effects that generate new visual content from audio data or clear the screen.

### Clear
- **Purpose**: Clears or blends framebuffer with a color to create trails/feedback
- **Inputs**: 
  - Framebuffer
  - Beat signal
- **Outputs**: Framebuffer (modified in-place)
- **Blend Modes**:
  - Replace: Fill with solid color (no trails)
  - Additive: Add color values (brightening effect)
  - Maximum: Take maximum of each channel
  - Average: Average with color (creates fadeout trails)
  - Subtractive: Subtract color values (darkening effect)
- **Controls**:
  - Color: The color to blend with
  - Blend mode: Selection of blend operation
  - Only first: Apply only on first frame

### Oscilloscope
- **Purpose**: Renders audio waveform visualization
- **Inputs**:
  - Audio waveform (576 samples per channel)
  - Framebuffer
- **Outputs**: Framebuffer (modified in-place)
- **Blend Modes**: None (direct draw)
- **Controls**:
  - Color: Waveform drawing color
  - Channel: Left/Right/Both audio channels
  - Draw mode: Dots or lines
  - Line width: Thickness of lines

### SuperScope
- **Purpose**: Advanced oscilloscope with multi-phase scripting
- **Inputs**:
  - Audio waveform
  - Audio spectrum
  - Beat signal
  - Frame counter
- **Outputs**: Framebuffer (points/lines drawn)
- **Blend Modes**: Additive or replace
- **Controls**:
  - Init script: Run once on initialization
  - Frame script: Run once per frame
  - Beat script: Run on beat detection
  - Point script: Run for each audio sample point
  - Draw mode: Dots/lines/wireframe
  - Colors: Multiple color options
  - N: Number of points to process

### Dot Grid
- **Purpose**: Renders a grid of dots modulated by audio
- **Inputs**:
  - Audio spectrum
  - Beat signal
- **Outputs**: Framebuffer (dots rendered)
- **Blend Modes**: Additive or replace
- **Controls**:
  - Grid size: X and Y dimensions
  - Color: Dot colors
  - Size modulation: Audio reactivity amount

### Dot Plane
- **Purpose**: 3D plane of dots with perspective
- **Inputs**:
  - Audio input
  - Beat signal
- **Outputs**: Perspective-projected dots
- **Blend Modes**: Additive or replace
- **Controls**:
  - Rotation: X/Y/Z angles
  - Distance: Z depth
  - Grid density: Point count

### Moving Particle
- **Purpose**: Particle system driven by audio
- **Inputs**:
  - Audio spectrum/waveform
  - Beat signal
- **Outputs**: Particle sprites to Framebuffer
- **Blend Modes**: Additive
- **Controls**:
  - Particle count
  - Size range
  - Speed/direction
  - Color mode
  - Gravity

### OnBeat Clear
- **Purpose**: Clears screen on beat detection
- **Inputs**:
  - Beat signal signal
  - Framebuffer
- **Outputs**: Cleared framebuffer on beats
- **Blend Modes**: Replace or blend
- **Controls**:
  - Color: Clear color
  - Blend amount
  - Beat sensitivity

### Picture/Picture II
- **Purpose**: Renders bitmap images
- **Inputs**:
  - Image file
  - Beat signal
- **Outputs**: Image rendered to Framebuffer
- **Blend Modes**: Replace/Add/50-50/Every other pixel
- **Controls**:
  - Image selection
  - Blend mode
  - On beat toggle
  - Persistent display

### Ring
- **Purpose**: Renders expanding rings from audio
- **Inputs**:
  - Audio waveform
  - Beat signal
- **Outputs**: Concentric rings
- **Blend Modes**: Additive
- **Controls**:
  - Ring count
  - Expansion speed
  - Color source
  - Line width

### Rotating Stars
- **Purpose**: Star field effect
- **Inputs**:
  - Audio input
  - Beat signal
- **Outputs**: Rotating star points
- **Blend Modes**: Additive
- **Controls**:
  - Star count
  - Rotation speed
  - Depth layers
  - Color mode

### Simple Spectrum
- **Purpose**: Basic spectrum analyzer display
- **Inputs**:
  - Audio spectrum
- **Outputs**: Spectrum bars
- **Blend Modes**: Replace or additive
- **Controls**:
  - Bar count
  - Height scale
  - Color mode
  - Draw style

### Spectrum Analyzer
- **Purpose**: Advanced spectrum analyzer
- **Inputs**:
  - Audio spectrum (FFT)
- **Outputs**: Spectrum visualization
- **Blend Modes**: Replace or additive
- **Controls**:
  - Bar count
  - Logarithmic/linear scale
  - Peak hold
  - Color gradient
  - Mirror mode

### SVP (Simple Visualization Plugin)
- **Purpose**: Oscilloscope with beat-reactive features
- **Inputs**:
  - Audio waveform
  - Beat signal
- **Outputs**: Waveform visualization
- **Blend Modes**: Additive
- **Controls**:
  - Draw mode
  - Beat flash
  - Color cycling

### Text
- **Purpose**: Renders text messages
- **Inputs**:
  - Text string
  - Beat signal
- **Outputs**: Framebuffer (text rendered)
- **Blend Modes**: Replace or blend
- **Controls**:
  - Text content
  - Font/size
  - Position
  - Color
  - On beat effects

### Timescope
- **Purpose**: Scrolling waveform history
- **Inputs**:
  - Audio waveform over time
- **Outputs**: Scrolling waveform display
- **Blend Modes**: Replace
- **Controls**:
  - Scroll speed
  - Color mode
  - Band count

## Trans Effects (Transform)
Effects that move or distort existing pixels.

### Movement
- **Purpose**: Simple preset-based transformations
- **Inputs**:
  - Framebuffer
- **Outputs**: Framebuffer (transformed)
- **Blend Modes**: Optional source blend
- **Controls**:
  - Preset selection (23 built-in effects)
  - Custom mode with r,d or x,y scripts
  - Source mapping
  - Wrap mode
  - Bilinear filtering

### Dynamic Movement
- **Purpose**: Grid-based coordinate transformation with scripting
- **Inputs**:
  - Framebuffer
  - Audio input for script variables
  - Beat signal
- **Outputs**: Framebuffer (transformed) via coordinate remapping
- **Blend Modes**: Optional max blend
- **Controls**:
  - Init/frame/beat/pixel scripts
  - Grid resolution (2x2 to 256x256)
  - Coordinate system (rectangular/polar)
  - Interpolation mode
  - Wrap edges
  - Bilinear filtering

### Dynamic Distance Modifier
- **Purpose**: Distance-based distortion
- **Inputs**:
  - Framebuffer
  - Audio input
- **Outputs**: Distance-modulated transformation
- **Blend Modes**: None
- **Controls**:
  - Distance script
  - Polar mode
  - Scale factor

### Dynamic Shift
- **Purpose**: Pixel shifting based on scripts
- **Inputs**:
  - Framebuffer
  - Audio input
- **Outputs**: Shifted pixels
- **Blend Modes**: Replace or blend
- **Controls**:
  - X shift script
  - Y shift script
  - Wrap mode

### Roto Blitter
- **Purpose**: Rotating bitmap blitter
- **Inputs**:
  - Framebuffer
  - Bitmap image
- **Outputs**: Rotated/scaled bitmap
- **Blend Modes**: Multiple blend modes
- **Controls**:
  - Rotation angle/speed
  - Scale
  - Zoom on beat
  - Blend mode

### Scatter
- **Purpose**: Scatters pixels randomly
- **Inputs**:
  - Framebuffer
- **Outputs**: Scattered pixels
- **Blend Modes**: None
- **Controls**:
  - Scatter amount
  - Direction

### Texer/Texer II
- **Purpose**: Particle-based texture mapper
- **Inputs**:
  - Framebuffer
  - Texture image
  - Audio input
- **Outputs**: Textured particles
- **Blend Modes**: Additive or replace
- **Controls**:
  - Particle count
  - Texture selection
  - Size modulation
  - Position scripts

## Trans Effects (Color/Filter)
Effects that modify pixel colors or apply filters.

### Blur
- **Purpose**: Smoothing/blur filter
- **Inputs**:
  - Framebuffer
- **Outputs**: Blurred framebuffer
- **Blend Modes**: None (direct filter)
- **Controls**:
  - Blur radius
  - Algorithm (box/gaussian)

### Brightness
- **Purpose**: Adjust image brightness
- **Inputs**:
  - Framebuffer
- **Outputs**: Brightness-adjusted pixels
- **Blend Modes**: None
- **Controls**:
  - Brightness level (-255 to +255)

### Channel Shift
- **Purpose**: Shift color channels independently
- **Inputs**:
  - Framebuffer
- **Outputs**: Channel-shifted pixels
- **Blend Modes**: None
- **Controls**:
  - Red shift X/Y
  - Green shift X/Y
  - Blue shift X/Y
  - Wrap mode

### Color Clip
- **Purpose**: Clip colors to range
- **Inputs**:
  - Framebuffer
- **Outputs**: Clipped colors
- **Blend Modes**: None
- **Controls**:
  - Min/max values per channel
  - Clip mode

### Color Map
- **Purpose**: Map colors through lookup table
- **Inputs**:
  - Framebuffer
- **Outputs**: Remapped colors
- **Blend Modes**: None
- **Controls**:
  - Color map selection
  - Cycle on beat
  - Map rotation

### Color Modifier
- **Purpose**: Modify colors with scripts
- **Inputs**:
  - Framebuffer
  - Audio input
- **Outputs**: Modified colors
- **Blend Modes**: None
- **Controls**:
  - Red/green/blue scripts
  - Beat reactivity

### Color Reduction
- **Purpose**: Reduce color bit depth
- **Inputs**:
  - Framebuffer
- **Outputs**: Quantized colors
- **Blend Modes**: None
- **Controls**:
  - Bits per channel

### Contrast
- **Purpose**: Adjust image contrast
- **Inputs**:
  - Framebuffer
- **Outputs**: Contrast-adjusted pixels
- **Blend Modes**: None
- **Controls**:
  - Contrast level

### Convolution Filter
- **Purpose**: Apply convolution kernels
- **Inputs**:
  - Framebuffer
- **Outputs**: Filtered pixels
- **Blend Modes**: None
- **Controls**:
  - Kernel selection
  - Scale factor
  - Bias
  - Wrap edges

### Fast Brightness
- **Purpose**: Optimized brightness adjustment
- **Inputs**:
  - Framebuffer
- **Outputs**: Brightness-adjusted pixels
- **Blend Modes**: None
- **Controls**:
  - Brightness multiplier

### Grain
- **Purpose**: Add film grain effect
- **Inputs**:
  - Framebuffer
- **Outputs**: Grainy pixels
- **Blend Modes**: Blend amount
- **Controls**:
  - Grain amount
  - Grain size
  - Static/animated

### Interferences
- **Purpose**: Wave interference patterns
- **Inputs**:
  - Framebuffer
- **Outputs**: Interference pattern overlay
- **Blend Modes**: Additive or multiply
- **Controls**:
  - Wave count
  - Speed
  - Alpha

### Interleave
- **Purpose**: Interlace/scanline effects
- **Inputs**:
  - Framebuffer
- **Outputs**: Interlaced output
- **Blend Modes**: None
- **Controls**:
  - Line thickness
  - Offset
  - Blend amount

### Invert
- **Purpose**: Invert colors
- **Inputs**:
  - Framebuffer
- **Outputs**: Inverted colors
- **Blend Modes**: None
- **Controls**:
  - Invert toggle

### Mirror
- **Purpose**: Mirror/flip transformations
- **Inputs**:
  - Framebuffer
- **Outputs**: Mirrored pixels
- **Blend Modes**: None
- **Controls**:
  - Mirror mode (horizontal/vertical/quad)
  - Smooth transition
  - On beat toggle

### Mosaic
- **Purpose**: Pixelate/mosaic effect
- **Inputs**:
  - Framebuffer
- **Outputs**: Pixelated output
- **Blend Modes**: None
- **Controls**:
  - Block size
  - Blend amount

### Multi Delay
- **Purpose**: Echo/delay effect
- **Inputs**:
  - Framebuffer
  - History buffer
- **Outputs**: Delayed/echoed pixels
- **Blend Modes**: Multiple blend modes
- **Controls**:
  - Delay frames
  - Feedback amount
  - Blend mode

### Multi Filter
- **Purpose**: Chain multiple filters
- **Inputs**:
  - Framebuffer
- **Outputs**: Multi-filtered output
- **Blend Modes**: Per-filter blend
- **Controls**:
  - Filter selection
  - Parameters per filter

### Normalize
- **Purpose**: Normalize brightness range
- **Inputs**:
  - Framebuffer
- **Outputs**: Normalized pixels
- **Blend Modes**: None
- **Controls**:
  - Auto/manual mode

### Unique Tone
- **Purpose**: Posterize with unique colors
- **Inputs**:
  - Framebuffer
- **Outputs**: Posterized output
- **Blend Modes**: None
- **Controls**:
  - Color count
  - Invert

### Video Delay
- **Purpose**: Frame delay/echo
- **Inputs**:
  - Framebuffer
  - Frame history
- **Outputs**: Delayed frames
- **Blend Modes**: Blend with current
- **Controls**:
  - Delay frames
  - Feedback

### Water
- **Purpose**: Water ripple effect
- **Inputs**:
  - Framebuffer
- **Outputs**: Rippled output
- **Blend Modes**: None
- **Controls**:
  - Wave speed
  - Amplitude
  - Frequency

### Water Bump
- **Purpose**: Water surface simulation
- **Inputs**:
  - Framebuffer
  - Height map
- **Outputs**: Refracted pixels
- **Blend Modes**: None
- **Controls**:
  - Light direction
  - Bump height
  - Refraction amount

## Misc Effects
Utility and control effects.

### AVS Trans Automation
- **Purpose**: Automate effect parameters
- **Inputs**:
  - Time/beat counters
- **Outputs**: Parameter changes
- **Blend Modes**: N/A
- **Controls**:
  - Automation curves
  - Speed
  - Beat sync

### Buffer Save
- **Purpose**: Save/restore framebuffer
- **Inputs**:
  - Framebuffer to save
- **Outputs**: Saved buffer on restore
- **Blend Modes**: Blend on restore
- **Controls**:
  - Buffer slot
  - Action (save/restore)
  - Blend mode

### Comment
- **Purpose**: Add comments to preset
- **Inputs**: None
- **Outputs**: None
- **Blend Modes**: N/A
- **Controls**:
  - Comment text

### Custom BPM
- **Purpose**: Override automatic BPM detection
- **Inputs**:
  - Manual BPM value
- **Outputs**: Beat signals
- **Blend Modes**: N/A
- **Controls**:
  - BPM value
  - Confidence
  - Skip beats

### Effect List
- **Purpose**: Container for effect chain
- **Inputs**:
  - Input from previous effect
- **Outputs**: Output of contained effects
- **Blend Modes**: Per-effect
- **Controls**:
  - Effect order
  - Enable/disable effects
  - Input/output modes

### Framerate Limiter
- **Purpose**: Limit rendering framerate
- **Inputs**:
  - Frame timing
- **Outputs**: Throttled frames
- **Blend Modes**: N/A
- **Controls**:
  - Target FPS
  - Limit mode

### Global Variables
- **Purpose**: Share variables between effects
- **Inputs**:
  - Variable assignments
- **Outputs**: Variable values
- **Blend Modes**: N/A
- **Controls**:
  - Variable definitions
  - Init script
  - Frame script

### MIDI Trace
- **Purpose**: Visualize MIDI input
- **Inputs**:
  - MIDI messages
- **Outputs**: MIDI visualization
- **Blend Modes**: Additive
- **Controls**:
  - Channel selection
  - Display mode
  - Color mapping

### Set Render Mode
- **Purpose**: Control rendering pipeline
- **Inputs**:
  - Mode selection
- **Outputs**: Rendering configuration
- **Blend Modes**: N/A
- **Controls**:
  - Blend mode
  - Line width
  - Clear mode