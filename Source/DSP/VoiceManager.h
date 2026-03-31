#pragma once
#include "SynthVoice.h"
#include "NoiseOscillator.h"
#include "LFO.h"
#include "AnalogueDriftEngine.h"
#include "ModulationMatrix.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace tillysynth
{

class VoiceManager
{
public:
    static constexpr int kMaxVoices = 16;

    VoiceManager();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void handleNoteOn (int midiNote, float velocity);
    void handleNoteOff (int midiNote);
    void handlePitchWheel (int pitchWheelValue);
    void handleSustainPedal (bool isDown);
    void handleAllNotesOff();

    void renderNextSample (float& leftOut, float& rightOut);

    // Parameter updates (call once per block, not per sample)
    void updateOsc1Params (Waveform wf, int octave, int semitone, float fineTune,
                           float level, float pw, int unisonVoices,
                           float unisonDetune, float unisonBlend);
    void updateOsc2Params (Waveform wf, int octave, int semitone, float fineTune,
                           float level, float pw, int unisonVoices,
                           float unisonDetune, float unisonBlend);

    void updateAmpEnv1 (float attack, float decay, float sustain, float release);
    void updateAmpEnv2 (float attack, float decay, float sustain, float release);
    void updateFilterEnv (float attack, float decay, float sustain, float release);

    void updateNoiseParams (NoiseType type, float level, float shRate);
    void updateNoiseEnv (float attack, float decay, float sustain, float release);

    void updateFilterParams (FilterMode mode, bool is24dB, float cutoff, float resonance,
                             float envAmount, float keyTracking, float velocity,
                             FilterTarget target);

    void updateLFO1 (Waveform wf, float rate, float depth,
                     bool destCutoff, bool destPitch, bool destVolume, bool destPW);
    void updateLFO2 (Waveform wf, float rate, float depth,
                     bool destCutoff, bool destPitch, bool destVolume, bool destPW);

    void updateModEnv1 (float attack, float decay, float sustain, float release, float amount,
                        bool destCutoff, bool destResonance, bool destPitch, bool destVolume);
    void updateModEnv2 (float attack, float decay, float sustain, float release, float amount,
                        bool destCutoff, bool destResonance, bool destPitch, bool destVolume);

    void setMaxPolyphony (int voices);
    void setMonoLegato (bool enabled);
    void setGlideTime (float ms);
    void setPitchBendRange (int semitones);
    void setModWheelValue (float value01);
    void setAftertouchValue (float value01);

    void updateModMatrix (int slotIndex, ModSource source, ModDest dest, float amount);

    AnalogueDriftEngine& getDriftEngine() { return driftEngine; }
    const LFO& getLFO1() const { return lfo1; }
    const LFO& getLFO2() const { return lfo2; }

private:
    int findFreeVoice() const;
    int findOldestVoice() const;
    int findVoicePlayingNote (int midiNote) const;

    std::array<SynthVoice, kMaxVoices> voices;

    LFO lfo1;
    LFO lfo2;
    AnalogueDriftEngine driftEngine;
    ModulationMatrix modMatrix;

    int maxPolyphony = 16;
    bool monoLegato = false;
    float glideTimeMs = 0.0f;
    int pitchBendRange = 2;
    float currentPitchBend = 0.0f;
    float modWheelValue = 0.0f;
    float aftertouchValue = 0.0f;
    float lfo1BaseRate = 1.0f;
    float lfo2BaseRate = 1.0f;

    int noteOrderCounter = 0;
    bool sustainPedalDown = false;
    std::array<bool, 128> sustainedNotes {};

    // Track held notes for mono legato
    std::array<int, 128> heldNotes {};
    int heldNoteCount = 0;
};

} // namespace tillysynth
