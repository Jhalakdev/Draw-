# Curvex — Draw EQ

Brand: **Curvex**
Product: **Draw EQ**
Phase modes: Minimum / Linear / Natural — zero pre-ringing cepstrum FIR
M/S processing: Stereo / Mid / Side / Left / Right
6 character emulations: Mister Passive, Krane Mybiz, West Nugget, Pool Dake, Never 80-8, Liquid State Solid
Auto Gain toggle: maintains constant perceived level

## Build

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

Install paths: `~/Library/Audio/Plug-Ins/Components/Draw EQ.component`, `~/Library/Audio/Plug-Ins/VST3/Draw EQ.vst3`

## Future Product Roadmap

1. **Draw EQ Lite** — draw EQ without analog character emulations
2. **Draw EQ Pro** — current full state with WDF analog emulations
3. **Matching EQ** — learn EQ curve from reference audio
4. **Brickwall** — high-end brickwall limiter (details TBD)
5. **Character Gain** — gain fader + saturation fader; as gain increases, analog character increases. Phase invert / mono knob
6. **AutoTune** — AI-powered pitch correction via LLM API (SUNO-level)
7. **One-Knob Compressor** — ratio + threshold only, with character
