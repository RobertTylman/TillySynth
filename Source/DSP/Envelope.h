#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace tillysynth
{

class Envelope
{
public:
    void prepare (double sampleRate);
    void reset();

    void setParameters (float attackMs, float decayMs, float sustain01, float releaseMs);

    void noteOn();
    void noteOff();

    float processSample();

    bool isActive() const { return stage != Stage::Idle; }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void calculateCoefficients();

    Stage stage = Stage::Idle;
    double currentSampleRate = 44100.0;

    float attackMs  = 5.0f;
    float decayMs   = 200.0f;
    float sustainLevel = 0.7f;
    float releaseMs = 300.0f;

    float attackCoeff  = 0.0f;
    float attackBase   = 0.0f;
    float decayCoeff   = 0.0f;
    float decayBase    = 0.0f;
    float releaseCoeff = 0.0f;
    float releaseBase  = 0.0f;

    float currentValue = 0.0f;
};

} // namespace tillysynth
