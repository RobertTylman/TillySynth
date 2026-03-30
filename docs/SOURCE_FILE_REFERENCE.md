# TillySynth Source File Reference

This document describes every file under `Source/` as of March 29, 2026. It is meant to be a practical map of the codebase: what each file owns, how it fits into the synth, and where to look when you want to change a behavior.

## High-Level Architecture

At a high level, TillySynth is split into four layers:

1. The plugin processor layer owns JUCE integration, parameter state, MIDI handling, and the per-block audio pipeline.
2. The DSP layer implements voices, oscillators, envelopes, filters, LFOs, chorus, noise, and the analogue drift system.
3. The UI layer draws the synth interface and binds widgets to parameters.
4. The preset tooling layer manages factory and user presets, preset mutation, and the external preset review application.

Audio generally flows like this:

`PluginProcessor -> VoiceManager -> SynthVoice -> Oscillators / Noise / Envelopes / Filters -> FX -> Output`

Preset and state data generally flows like this:

`Parameters / APVTS <-> PresetManager <-> Plugin UI / Review App`

## Top-Level Source Files

### `Source/PluginProcessor.h`

Declares `TillySynthProcessor`, the main JUCE `AudioProcessor` for the synth. This file defines the public surface that the rest of the app talks to: parameter access, preset access, MIDI keyboard state, transpose, UI-driven pitch/mod wheel input, and read-only telemetry used by the editor such as LFO phase, output meter values, drift values, and oscilloscope samples.

It is the main ownership hub for runtime systems. The processor owns the `AudioProcessorValueTreeState`, `VoiceManager`, chorus, reverb, preset manager, and smoothed master volume.

### `Source/PluginProcessor.cpp`

Implements the processor lifecycle. This file sets up the processor in the constructor, prepares DSP objects in `prepareToPlay`, clears/reconfigures resources in `releaseResources`, and runs the synth in `processBlock`.

Its most important job is translating parameter values from the APVTS into live DSP state once per block in `updateParametersFromAPVTS()`. It also handles incoming MIDI, UI pitch bend/mod wheel messages, JUCE state serialization, and editor creation.

### `Source/PluginEditor.h`

Declares the synth editor and the custom `WheelComponent` used for the pitch and mod wheels. This header is where the UI is organized conceptually: helper methods for making knobs, combos, and toggles; layout helpers for each section; and drawing helpers for custom visuals like the drift scope, VU meter, LFO display, and envelope preview.

It also defines most of the editor-owned widgets: preset navigation, save/random buttons, transpose controls, master volume, keyboard, author link, tooltip support, and the maps that store control attachments by parameter ID.

### `Source/PluginEditor.cpp`

Implements the entire synth interface. The constructor builds and wires the controls to the APVTS, sets tooltips, applies the custom look-and-feel, and configures header actions like preset navigation and saving.

The rest of the file is split between drawing and layout. `paint()` draws the section backgrounds and custom meters/displays, while `resized()` and the `layout...Section()` helpers position every control. This is also where the editor renders the analogue drift strip, LFO waveform previews, and the filter ADSR preview, and where the wheel component interaction is implemented.

### `Source/Parameters.h`

Central registry of parameter IDs. Every exposed synth control has a stable string constant here, grouped by domain: oscillator 1, oscillator 2, filter, LFOs, modulation envelopes, chorus, reverb, noise, and master.

This file is the safest place to start when you want to know whether a behavior is already parameterized. The rest of the codebase relies on these IDs for UI attachments, APVTS reads, preset storage, and mutation logic.

### `Source/Parameters.cpp`

Builds the full APVTS parameter layout. It defines the human-facing choice lists for combo parameters and creates all float, bool, int, and choice parameters with their ranges and defaults.

This is where the synth's control surface is canonically defined. If a parameter appears in the UI, DSP, and presets, it almost always starts here.

### `Source/PresetManager.h`

Declares the `Preset` data structure and the `PresetManager` class. `Preset` is a lightweight name/category/parameter snapshot, while `PresetManager` owns the combined factory-plus-user preset list and exposes load, apply, capture, save, delete, and refresh operations.

The header makes it clear that preset storage is APVTS-based. Presets are ultimately lists of parameter IDs and values, not bespoke binary patches.

### `Source/PresetManager.cpp`

Implements the full preset system. The top of the file builds the large hardcoded factory preset library, while the lower half manages user preset directories, XML serialization, preset application, migration/backfill for missing parameters, and user preset deletion.

This is one of the most important integration points in the project. It bridges the synth engine, the UI, the review app, and the on-disk preset format. If a new parameter needs to survive load/save correctly, this file almost always needs attention.

### `Source/PresetVariantGenerator.h`

Declares the preset mutation utility used by the review workflow. The class exposes a single public entry point that takes an existing preset and generates a named variant.

The helper methods indicate its strategy: mutate floats and integers differently, and treat some parameters as more stable than others so generated variants stay musically related to the source preset.

### `Source/PresetVariantGenerator.cpp`

Implements variant generation. It walks over the source preset's parameter list, applies selective mutations, and returns a new `Preset` object rather than rewriting the original.

This file exists mainly to support curation workflows. It is responsible for making "new preset from old preset" feel intentional rather than random chaos.

### `Source/PresetReviewApp.cpp`

Implements the separate desktop preset review tool. This file contains everything for that app: the persisted review store for ratings and notes, the fixed audition melody library, the main `PresetReviewComponent`, the document window, and the JUCE application bootstrap.

It reuses the synth core to load and audition presets, then adds review-specific behavior on top: step through presets, hear short melodies, rate sounds, write notes, browse the full list, generate new variants, delete weak user presets, and export review data.

## DSP Source Files

### `Source/DSP/Oscillator.h`

Declares the main oscillator voice building block. It defines the `Waveform` enum and the `Oscillator` class, which supports waveform selection, level control, pulse width, unison voice count, unison detune, and unison blend.

This oscillator is designed to be flexible enough for both primary synth oscillators. It is not just a simple sine source; it carries the core timbral controls used across the instrument.

### `Source/DSP/Oscillator.cpp`

Implements oscillator generation. It prepares phase state, resets voice phases, generates samples for each waveform, applies polyBLEP-style band-limiting around discontinuities, and manages unison frequency offsets.

This file is responsible for a large part of the synth's character. Any change to waveform shape, pulse width handling, or unison sound will usually land here.

### `Source/DSP/NoiseOscillator.h`

Declares the `NoiseType` enum and `NoiseOscillator` class. The class supports multiple noise colors and a sample-and-hold rate parameter for more digital or stepped textures.

It exists as a dedicated sound source rather than being folded into the main oscillators, which keeps the noise path simpler and easier to shape separately.

### `Source/DSP/NoiseOscillator.cpp`

Implements the actual noise generation algorithms. It provides white, pink, brown, blue, and digital-style outputs, plus timing logic for the sample-and-hold behavior.

This file is where the noise source gets its identity. If you want new noise flavors or different coloration behavior, this is the place to work.

### `Source/DSP/Envelope.h`

Declares the synth's reusable ADSR envelope generator. The interface is intentionally small: prepare, reset, set parameters, note on/off, process one sample, and query whether the envelope is active.

Multiple systems use this same class, including amp envelopes, the filter envelope, noise envelope, and the newer modulation envelopes.

### `Source/DSP/Envelope.cpp`

Implements the ADSR state machine. It calculates attack/decay/release coefficients from times, advances per sample, handles sustain state, and returns the current envelope value.

This file is a shared low-level utility. A bug here would affect many parts of the synth at once.

### `Source/DSP/SubtractiveFilter.h`

Declares the multimode subtractive filter and the enums that describe it. `FilterMode` controls the response shape, while `FilterTarget` controls which sound sources the filter should affect: oscillator 1, oscillator 2, both oscillators, noise, or all sources.

The class itself exposes the classic filter controls: cutoff, resonance, mode, and 12 dB versus 24 dB slope behavior.

### `Source/DSP/SubtractiveFilter.cpp`

Implements the filter processing and coefficient updates. It resets state, processes individual samples, updates the active mode, and recalculates coefficients when parameters change.

This file is the main tone-shaping stage after the raw oscillators and noise. Any redesign of the filter sound or slope behavior would happen here.

### `Source/DSP/LFO.h`

Declares the low-frequency oscillator system. `LFOOutput` is a routed modulation payload containing the current sample's contributions for cutoff, pitch, volume, and pulse width.

The `LFO` class owns waveform, rate, depth, routing flags, and raw sample access. The raw sample is used for cases like mod-wheel vibrato, while the routed output is used by the voice manager.

### `Source/DSP/LFO.cpp`

Implements LFO sample generation and routing. It produces a waveform sample, scales it by depth, and writes the result into whichever destinations are enabled.

This file is the modulation source side of the LFO system. Destination application happens later, mainly inside `VoiceManager` and `SynthVoice`.

### `Source/DSP/ChorusEngine.h`

Declares the chorus effect and the `ChorusMode` enum. The class exposes the public chorus controls and hides the internal delay-line and bucket-brigade-style stage details.

This effect is intended to provide the synth's classic widening and motion. It sits after the voice sum rather than inside each voice.

### `Source/DSP/ChorusEngine.cpp`

Implements the chorus effect. It contains the delay-line helper, cubic-interpolated delay reads, mode switching, rate/depth control, and the per-stage processing that gives the chorus its movement.

This file is where the synth's "ensemble" flavor lives. If the chorus feels too subtle, too seasick, or too narrow, this is the file to inspect.

### `Source/DSP/DriftSensorReader.h`

Declares a small platform abstraction for external drift-related signals. `DriftSensorData` packages normalized CPU load, thermal pressure, and battery drain metrics plus availability flags.

The `DriftSensorReader` class uses a PIMPL so the rest of the codebase can ask for sensor data without depending directly on macOS-specific implementation details.

### `Source/DSP/DriftSensorReader.mm`

Implements the macOS-specific side of `DriftSensorReader` in Objective-C++. This file bridges into platform APIs to read system-state signals that can be fed into the analogue drift model.

This file is intentionally isolated. Keeping the platform-specific logic here prevents the rest of the synth DSP from becoming cluttered with Apple-only code.

### `Source/DSP/AnalogueDriftEngine.h`

Declares the analogue drift system. It owns per-voice pitch and cutoff drift values, exposes a user drift amount, and provides lightweight getters that the audio thread can read safely.

It also exposes cached sensor state for the UI, which lets the editor show things like CPU and thermal influence without touching platform APIs directly.

### `Source/DSP/AnalogueDriftEngine.cpp`

Implements the drift timer and update logic. It periodically reads platform sensor data, updates per-voice drift signals, and mixes randomness with system-state influence to create a more alive analogue behavior.

This file is where the synth's "analogue drift" concept becomes concrete. It is both a DSP utility and a small personality engine for the instrument.

### `Source/DSP/SynthVoice.h`

Declares a single playable synth voice. Each voice owns two oscillators, one noise source, multiple envelopes, three per-source filters, and the state needed for glide, note tracking, modulation envelope routing, and voice stealing.

This file is the per-note sound architecture. If you want to understand what one note consists of before voices are mixed together, this is the right place.

### `Source/DSP/SynthVoice.cpp`

Implements per-voice sound generation. It starts and releases notes, calculates glide, computes modulation from LFOs and modulation envelopes, derives oscillator frequencies, generates source samples, applies amp/noise/filter envelopes, routes the filter to the selected target, and returns the mixed sample for one voice.

This file is the heart of the synth engine. It is where raw building blocks become an expressive note with envelopes, modulation, drift, and filtering.

### `Source/DSP/VoiceManager.h`

Declares the polyphony and modulation coordinator. It owns the fixed-size voice array, the two LFOs, the analogue drift engine, note-order bookkeeping, sustain state, mono-legato handling, and all the "update parameter once per block" entry points.

Think of this class as the conductor above the individual voices. It does not draw UI and does not own APVTS state; it translates higher-level synth settings into voice behavior.

### `Source/DSP/VoiceManager.cpp`

Implements voice allocation and note control. It handles note on/off, sustain pedal logic, pitch wheel, all-notes-off, mono/legato note reuse, voice stealing, per-sample LFO processing, and mixing all active voices into a mono signal before effects.

It is also the fan-out point for parameter updates. Oscillator, envelope, filter, noise, LFO, modulation-envelope, polyphony, glide, and drift settings all get pushed into the active voice layer here.

## UI Support Files

### `Source/UI/TillySynthLookAndFeel.h`

Declares the visual theme used by the synth and review app UI. It centralizes the amber-and-charcoal color palette and declares the custom drawing hooks for knobs, sliders, buttons, combo boxes, and labels.

This file is the visual vocabulary of the project. If you want to restyle the interface consistently, start here before touching individual editor drawing code.

### `Source/UI/TillySynthLookAndFeel.cpp`

Implements the custom JUCE look-and-feel. It draws the round knobs, linear sliders, buttons, combo boxes, and labels with the synth's specific visual identity and scaling behavior.

This file is focused on reusable widget rendering, unlike `PluginEditor.cpp`, which is focused on overall layout and section-level visuals.

## File Relationships That Matter Most

If you are onboarding to the codebase, these are the most important relationships to keep in mind:

- `Parameters.*` defines what can be controlled.
- `PluginEditor.*` exposes those controls to the user.
- `PluginProcessor.*` reads those controls and pushes them into DSP objects.
- `VoiceManager.*` distributes settings and MIDI state across voices.
- `SynthVoice.*` turns those settings into actual note audio.
- `PresetManager.*` makes parameter sets portable and persistent.
- `PresetReviewApp.cpp` reuses the synth core for batch preset evaluation outside the plugin.

## Good Starting Points For Common Tasks

For a UI change, start in `Source/PluginEditor.cpp` and `Source/UI/TillySynthLookAndFeel.cpp`.

For a new parameter, start in `Source/Parameters.h`, `Source/Parameters.cpp`, then wire it through `Source/PluginProcessor.cpp`, the relevant DSP file, and `Source/PresetManager.cpp`.

For a sound-engine change, start in `Source/DSP/SynthVoice.cpp`, then follow dependencies outward into `Oscillator`, `Filter`, `Envelope`, `LFO`, or `VoiceManager`.

For preset behavior, start in `Source/PresetManager.cpp` and `Source/PresetVariantGenerator.cpp`.

For the external preset curation workflow, start in `Source/PresetReviewApp.cpp`.
