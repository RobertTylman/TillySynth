# TillySynth

**TillySynth** is a polyphonic subtractive synthesizer plugin designed to capture the warmth, character, and "organic imperfection" of vintage hardware, specifically inspired by the legendary Roland Juno-60.

At its heart is a signature **CPU-temperature-driven analogue drift engine**, which uses real-world hardware fluctuations to drive subtle per-voice pitch and filter variations. This ensures that the synthesizer feels alive and evolving, rather than sterile and predictable.

## Key Features

### 🎹 Living Oscillators & Polyphony
- **Polyphonic Subtractive Engine**: Up to 16 voices with an oldest-note-first stealing strategy.
- **Dual Independent Oscillators**: Choose between Sine, Sawtooth, Square (with PWM), and Triangle waves.
- **Unison Mode**: Up to 7 voices per oscillator with adjustable detune spread and blend.
- **Analogue Drift**: A unique engine that polls CPU temperature to drive randomized, organic drift in pitch (±8 cents) and filter cutoff (±4 Hz), making every instance unique.

### 🎛️ Sculpting & Modulation
- **Multi-Mode Filter**: Low-pass, High-pass, Band-pass, and Notch modes with selectable 12dB or 24dB slopes.
- **Key & Velocity Tracking**: Harder playing can open the filter cutoff for dynamic performances.
- **Twin LFOs**: Independent LFOs targeting pitch, filter, volume, or pulse width.
- **Dual Envelopes**: Dedicated ADSR envelopes for both Amplitude and Filter sections.

### ✨ Vintage Effects
- **BBD-Style Chorus**: A characterful modeled chorus with modes I, II, and I+II, providing the classic Juno-60 "lushness" with subtle wow and flutter.
- **Portamento / Glide**: Smooth pitch transitions for monophonic or polyphonic lines.

## Technical Stack

- **Framework**: [JUCE](https://juce.com/) (Audio Processors, DSP, Graphics, GUI)
- **Language**: C++17
- **Build System**: CMake with [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)
- **Platforms**: macOS (AU, VST3, Standalone), Windows (VST3, Standalone)

## Design Philosophy

TillySynth prioritizes **visual and sonic character**. The UI is custom-drawn with a vintage horizontal layout, featuring warm ambers, muted creams, and high-fidelity "panel wear" textures that are randomized per instance. 

Every technical decision, from the lack of a generic preset browser to the inclusion of hardware-influenced drift, is aimed at encouraging users to craft their own unique, "living" sounds.

---

*Designed and Developed by [Robbie Tylman](https://github.com/RobertTylman)*
