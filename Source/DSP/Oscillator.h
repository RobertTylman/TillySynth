#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace tillysynth
{

enum class Waveform { Sine = 0, Sawtooth, Square, Triangle };

class Oscillator
{
public:
    Oscillator();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    float processSample();

    void setFrequency (float frequencyHz);
    void setWaveform (Waveform wf);
    void setPulseWidth (float pw01);

    void setUnisonVoices (int count);
    void setUnisonDetune (float cents);
    void setUnisonBlend (float blend01);

    void setLevel (float level01);

    float getFrequency() const { return baseFrequency; }

private:
    static constexpr int kMaxUnisonVoices = 7;

    float generateWaveformSample (float phase, float phaseInc) const;
    void updateUnisonFrequencies();

    Waveform waveform = Waveform::Sawtooth;
    float baseFrequency = 440.0f;
    float pulseWidth = 0.5f;
    float level = 1.0f;

    int unisonCount = 1;
    float unisonDetuneCents = 20.0f;
    float unisonBlend = 0.5f;

    double currentSampleRate = 44100.0;

    std::array<float, kMaxUnisonVoices> phases {};
    std::array<float, kMaxUnisonVoices> phaseIncrements {};
};

} // namespace tillysynth
