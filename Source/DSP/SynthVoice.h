#pragma once
#include "Oscillator.h"
#include "NoiseOscillator.h"
#include "Envelope.h"
#include "SubtractiveFilter.h"

namespace tillysynth
{

class SynthVoice
{
public:
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void noteOn (int midiNote, float velocity, bool legatoRetrigger = false);
    void noteOff();

    float processSample (float lfoFilterMod, float lfoPitchMod,
                         float lfoVolumeMod, float lfoPWMod,
                         float driftPitchCents, float driftCutoffHz);

    bool isActive() const;
    bool isNoteHeld() const { return noteHeld; }

    int getCurrentNote() const { return currentNote; }
    int getNoteStartOrder() const { return noteStartOrder; }

    void setNoteStartOrder (int order) { noteStartOrder = order; }

    // Oscillator params
    void setOsc1Params (Waveform wf, int octave, int semitone, float fineTuneCents,
                        float level01, float pw01, int unisonVoices,
                        float unisonDetune, float unisonBlend);
    void setOsc2Params (Waveform wf, int octave, int semitone, float fineTuneCents,
                        float level01, float pw01, int unisonVoices,
                        float unisonDetune, float unisonBlend);

    // Envelope params
    void setAmpEnv1Params (float attackMs, float decayMs, float sustain01, float releaseMs);
    void setAmpEnv2Params (float attackMs, float decayMs, float sustain01, float releaseMs);
    void setFilterEnvParams (float attackMs, float decayMs, float sustain01, float releaseMs);

    // Noise params
    void setNoiseParams (NoiseType type, float level01, float shRateHz);
    void setNoiseEnvParams (float attackMs, float decayMs, float sustain01, float releaseMs);

    // Filter params
    void setFilterParams (FilterMode mode, bool is24dB, float cutoffHz, float resonance01,
                          float envAmount, float keyTracking01, float velocity01);

    void setGlideTime (float glideMs);

private:
    float calculateFrequency (float note, int octave, int semitone, float fineCents,
                              float pitchMod, float driftCents) const;

    Oscillator osc1;
    Oscillator osc2;
    NoiseOscillator noiseOsc;

    Envelope ampEnv1;
    Envelope ampEnv2;
    Envelope noiseEnv;
    Envelope filterEnv;

    SubtractiveFilter filter;

    int currentNote = -1;
    float currentVelocity = 0.0f;
    bool noteHeld = false;
    int noteStartOrder = 0;

    // Oscillator tuning offsets
    int osc1Octave = 0, osc1Semitone = 0;
    float osc1FineTune = 0.0f;
    int osc2Octave = 0, osc2Semitone = 0;
    float osc2FineTune = 0.0f;

    // Filter state
    float baseCutoff = 8000.0f;
    float filterEnvAmount = 0.5f;
    float filterKeyTracking = 0.0f;
    float filterVelocitySens = 0.0f;

    double sampleRate = 44100.0;

    // Glide / portamento
    float glideCoeff = 1.0f;
    float currentGlideNote = -1.0f;

    // Voice stealing fade-out
    bool stealing = false;
    float stealFadeLevel = 1.0f;
    float stealFadeDecrement = 0.0f;

    // Pending note-on for after fade completes
    int pendingNote = -1;
    float pendingVelocity = 0.0f;
};

} // namespace tillysynth
