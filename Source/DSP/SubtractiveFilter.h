#pragma once
#include <juce_dsp/juce_dsp.h>

namespace tillysynth
{

enum class FilterMode { LowPass = 0, HighPass, BandPass, Notch };
enum class FilterTarget { Osc1 = 0, Osc2, BothOscillators, Noise, All };

class SubtractiveFilter
{
public:
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    float processSample (float input);

    void setCutoff (float frequencyHz);
    void setResonance (float resonance01);
    void setMode (FilterMode mode);
    void setSlope24dB (bool is24dB);

private:
    void updateCoefficients();

    FilterMode mode = FilterMode::LowPass;
    bool use24dB = true;
    float targetCutoffHz = 8000.0f;
    float smoothedCutoffHz = 8000.0f;
    float resonance = 0.2f;
    double currentSampleRate = 44100.0;
    float smoothingCoeff = 0.995f;
    int updateCounter = 0;

    // Two cascaded biquad stages for 12 or 24 dB/oct
    juce::dsp::IIR::Filter<float> filter1;
    juce::dsp::IIR::Filter<float> filter2;
};

} // namespace tillysynth
