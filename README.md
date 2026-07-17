# PitchFollow EQ — Logic Pro X Plugin

An intelligent equalizer Audio Unit (AU) plugin for Logic Pro X that detects the fundamental frequency of a singer's voice and automatically adjusts the EQ curve to follow the performance.

## Features

- **8-Band Parametric Equalizer** with draggable handles on a graphical display
- **Real-Time Frequency Response Curve** — see the EQ shape at a glance
- **Fundamental Frequency Detection** using autocorrelation + FFT
- **Pitch Tracking Mode** — when enabled, the EQ automatically adjusts:
  - Boosts presence in the frequency range near the detected pitch
  - Adds warmth to lower harmonics
  - Follows high/low vocal bumps in real time
  - Enhances harmonic content up to the 4th harmonic
- **Pitch Display** — shows the detected note name and frequency
- **Spectrum Analyzer** — real-time FFT visualization below the EQ graph
- **Fully automatable parameters** — all EQ bands, tracking, sensitivity, speed, and intensity

## Requirements

- macOS 11.0+
- Xcode 14+ (for building)
- CMake 3.22+
- Internet connection (for first build — JUCE is fetched via CMake)

## Building

```bash
cd PitchFollowEQ
cmake -B build -G Xcode
cmake --build build --config Release
```

The AU plugin will be copied to:
`~/Library/Audio/Plug-Ins/Components/PitchFollowEQ.component`

### Manual build without CMake GUI

```bash
cd PitchFollowEQ
mkdir build && cd build
cmake .. -G Xcode
open PitchFollowEQ.xcodeproj
```

Then build the **PitchFollowEQ AU** scheme in Xcode.

## Using in Logic Pro X

1. Build and install the plugin (see above)
2. Open Logic Pro X
3. Create an Audio Track
4. Add the plugin: Audio FX > AU Instruments > PitchFollowAudio > PitchFollow EQ
   (or search "PitchFollow" in the plugin browser)
5. Enable **Pitch Tracking** toggle to activate the auto-following mode
6. Adjust **Sensitivity** — how strongly the EQ reacts to pitch changes
7. Adjust **Speed** — how fast the EQ follows pitch changes
8. Adjust **Intensity** — overall depth of the auto-EQ effect

## Architecture

```
PitchFollowEQ/
├── CMakeLists.txt              # JUCE CMake build (fetches JUCE 7)
├── Source/
│   ├── PluginProcessor.h/cpp   # Audio processor, APVTS, state save/load
│   ├── PluginEditor.h/cpp      # Main plugin UI
│   ├── DSP/
│   │   ├── Equalizer.h/cpp     # 8-band parametric EQ (biquad peaking filters)
│   │   ├── PitchDetector.h/cpp # Autocorrelation-based f0 detection
│   │   └── PitchFollowEngine.h/cpp # Maps pitch → EQ adjustments
│   └── UI/
│       ├── EQGraphComponent.h/cpp  # Interactive EQ graph with response curve
│       └── SpectrumAnalyzer.h/cpp  # FFT spectrum display
```

### DSP Pipeline

1. **PitchDetector** analyses the incoming audio via autocorrelation to find the fundamental frequency (f0)
2. **PitchFollowEngine** maps f0 to 8-band gain adjustments:
   - Band closest to f0 gets a presence boost
   - Adjacent bands get a gentle cut for clarity
   - Harmonics (2nd–4th) of f0 get subtle boosts
   - High/low range compensation based on note register
3. **Equalizer** applies the resulting gain curve using biquad peaking filters
4. Smoothing (adjustable tracking speed) ensures artifact-free transitions

## License

MIT
