# PRD: TillySynth
**Studio:** Robbie Tylman

## Goal
TillySynth is a polyphonic subtractive synthesizer plugin inspired by the vintage aesthetic and warmth of the Roland Juno-60, with a signature CPU-temperature-driven analogue drift engine. Users can craft lush, organically imperfect pads, leads, and textures that feel alive rather than sterile.

## Features

### Oscillators (×2, independent)
- Waveform selector: Sine / Sawtooth / Square / Triangle
- Octave: -2 to +2 (integer steps)
- Semitone: -12 to +12 (integer steps)
- Fine tune: -100 to +100 cents
- Level: 0–100%, default 100%
- Pulse width: 0–100%, default 50% (active only when Square selected)
- Unison voices: 1–7, default 1
- Unison detune spread: 0–100 cents, default 20 cents
- Unison blend: 0–100%, default 50%
- Amp envelope per oscillator (ADSR):
  - Attack: 0–10,000 ms, default 5 ms
  - Decay: 0–10,000 ms, default 200 ms
  - Sustain: 0–100%, default 70%
  - Release: 0–10,000 ms, default 300 ms

### Filter
- Mode: Low-pass / High-pass / Band-pass / Notch
- Slope: 12 dB/oct or 24 dB/oct (selectable)
- Cutoff: 20–20,000 Hz, default 8,000 Hz
- Resonance: 0–100%, default 20%
- Envelope amount: -100 to +100%, default 50%
- Key tracking: 0–100%, default 0%
- Velocity-to-filter: 0–100%, default 0% (harder velocity opens cutoff)
- Filter envelope (ADSR):
  - Attack: 0–10,000 ms, default 10 ms
  - Decay: 0–10,000 ms, default 400 ms
  - Sustain: 0–100%, default 30%
  - Release: 0–10,000 ms, default 500 ms

### LFOs (×2, independent)
- Waveform: Sine / Saw / Square / Triangle, default Sine
- Rate: 0.01–20 Hz, default 1 Hz
- Depth: 0–100%, default 0%
- Destinations (multi-assign): Filter cutoff / Oscillator pitch / Oscillator volume / Pulse width

### Chorus
- Mode: Off / I / II / I+II, default Off
- Rate: 0.1–10 Hz, default 0.5 Hz
- Depth: 0–100%, default 50%
- Character modelled after BBD-style vintage chorus (subtle wow/flutter)

### Master
- Master volume: 0–100%, default 80%
- Polyphony: 1–16 voices, default 16
- Glide / portamento: 0–1,000 ms, default 0 ms
- Pitch bend range: 1–24 semitones, default 2
- Mono legato mode: toggle — when on, overlapping notes retrigger pitch only; envelopes continue uninterrupted
- Analogue Detune: 0–100%, default 0%
  - Reads CPU temperature via OS APIs, maps fluctuations to random per-voice pitch offset
  - Max drift at 100%: ±8 cents per voice
  - Also drives per-voice filter cutoff drift: ±4 Hz per voice at 100%
  - Update rate tied to CPU polling interval (unpredictable / organic — not a fixed LFO)
  - On plugin load: drift seed values are randomised so no two instances sound identical

## Behaviour
- 16-voice polyphony; voice stealing uses oldest-note strategy
- Mono legato mode: detect overlapping MIDI notes; retrigger pitch without retriggering envelopes
- Velocity-to-filter scales linearly from 0% (no effect) to 100% (full velocity range opens cutoff to maximum)
- CPU temperature polling: use platform APIs (macOS: `IOKit`, Windows: `WMI` or third-party wrapper); graceful fallback to random drift if temperature unavailable
- Pulse width control disabled/greyed in UI when oscillator waveform is not Square
- Both LFOs can target the same destination simultaneously (additive)
- Plugin state (all parameters) saved and restored via DAW session (`getStateInformation` / `setStateInformation`)
- Startup: randomise analogue drift seeds on every plugin instantiation

## UI
- Fully custom drawn UI; Juno-60 inspired but distinctly original — not a clone
- Vintage horizontal panel layout with dedicated sections per module
- Colour palette: warm ambers, muted creams, dark charcoal panel background
- Panel wear: per-instance randomised subtle knob scuffs and surface texture variation rendered at load time
- VU-style output meter with warm amber/green colour and analogue lag ballistics
- Branding: "TillySynth" wordmark + "Robbie Tylman" studio name in UI header
- Target plugin window size: [NEED: confirm preferred width × height]

## Plugin Formats & Platforms
- Formats: VST3, AU, Standalone
- Platforms: macOS, Windows

## Out of Scope (v1)
- Preset browser or preset management system
- Tempo sync for LFOs
- Oversampling / anti-aliasing
- Sidechain input
- MIDI learn / parameter mapping
- Undo/redo for parameter changes
- Multi-output routing
- AAX / CLAP formats
- Linux support
