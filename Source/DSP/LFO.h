#pragma once
#include <juce_core/juce_core.h>
#include "Oscillator.h"

namespace tillysynth
{

struct LFOOutput
{
    float cutoffMod  = 0.0f;   // -1 to +1
    float pitchMod   = 0.0f;   // -1 to +1
    float volumeMod  = 0.0f;   // -1 to +1
    float pwMod      = 0.0f;   // -1 to +1
};

class LFO
{
public:
    void prepare (double sampleRate);
    void reset();

    LFOOutput processSample();

    void setWaveform (Waveform wf);
    void setRate (float hz);
    void setDepth (float depth01);
    void setDestinations (bool cutoff, bool pitch, bool volume, bool pw);

    float getPhase() const { return phase; }
    Waveform getWaveform() const { return waveform; }
    float getRate() const { return rate; }
    float getDepth() const { return depth; }

    // Raw -1..1 waveform value (no depth scaling) for mod wheel vibrato
    float getRawSample() const;

private:
    float generateSample();

    Waveform waveform = Waveform::Sine;
    float rate = 1.0f;
    float depth = 0.0f;

    bool destCutoff = false;
    bool destPitch  = false;
    bool destVolume = false;
    bool destPW     = false;

    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    double currentSampleRate = 44100.0;
};

} // namespace tillysynth
