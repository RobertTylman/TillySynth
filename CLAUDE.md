# TillySynth — Robbie Tylman

## What This Is
A polyphonic subtractive synthesizer plugin for macOS and Windows (VST3, AU, Standalone) with a Juno-60-inspired custom UI and a CPU-temperature-driven analogue drift engine.

## Tech Stack
- **Framework:** JUCE
- **Language:** C++17
- **Build system:** CMake with CPM
- **Plugin formats:** VST3, AU, Standalone
- **JUCE modules:** juce_audio_processors, juce_dsp, juce_graphics, juce_gui_basics
- **CPU temperature:** IOKit (macOS), WMI or lightweight wrapper (Windows); graceful fallback to PRNG drift if unavailable

## Architecture Preferences
- `PluginProcessor` owns all DSP objects and audio state; `PluginEditor` owns all drawing and UI state — no cross-coupling
- Each DSP concept is its own class: `Oscillator`, `SubtractivFilter`, `AmpEnvelope`, `FilterEnvelope`, `LFO`, `ChorusEngine`, `AnalogueDriftEngine`, `VoiceManager`
- `VoiceManager` owns a pool of 16 `SynthVoice` objects; each voice composes its own oscillator pair, envelopes, and filter instance
- All user-facing parameters defined in a single `createParameterLayout()` function using APVTS
- `AnalogueDriftEngine` runs on a background timer thread — it polls CPU temperature and pushes drift values to an atomic float array read by the audio thread. Never call OS temperature APIs from `processBlock()`
- `PluginEditor` uses a custom `LookAndFeel` subclass for all component rendering; no JUCE default chrome
- Implement `getStateInformation()` and `setStateInformation()` — session recall is mandatory
- Panel wear texture: generate per-instance randomised seed in constructor; render wear overlay in `PluginEditor::paint()`

## Coding Principles
- **Single responsibility.** Every function does one thing. If you need an "and" to describe what it does, split it.
- **Small functions.** If a function exceeds ~30 lines, it's probably doing too much.
- **No god classes.** Each class represents one clear concept. Decompose if methods accumulate.
- **Composition over deep inheritance.** Prefer combining small focused objects; max two levels of inheritance.
- **Descriptive names over comments.** `calculateDriftOffsetCents()` needs no comment. `calc()` needs a refactor.
- **Comment why, not what.** Explain reasoning behind non-obvious decisions, not what the code does.
- **Fail early and clearly.** Validate inputs at function boundaries; don't propagate bad state silently.

## DSP Constraints
- No memory allocations in `processBlock()` — pre-allocate all buffers in `prepareToPlay()`
- No locks or blocking calls on the audio thread — use atomics for drift values from background thread
- Use `juce::SmoothedValue` for all audio-rate parameter changes (cutoff, level, detune)
- Use `juce::dsp` module processors (filters, oscillators) before writing custom DSP where they fit
- Voice stealing: oldest-note strategy

## What NOT To Do
- Don't add third-party dependencies without asking first
- Don't use `using namespace juce;` — always prefix with `juce::`
- Don't use raw pointers — prefer `std::unique_ptr` or JUCE smart pointer equivalents
- Don't put DSP logic in `PluginEditor` or any UI class
- Don't call OS temperature APIs from the audio thread — use the `AnalogueDriftEngine` background thread only
- Don't add features not in the PRD (no preset browser, no tempo sync, no oversampling)
- Don't use `juce::var` or dynamic typing where static types work
