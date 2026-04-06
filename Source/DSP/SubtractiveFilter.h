#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace tillysynth
{

enum class FilterMode { LowPass = 0, HighPass, BandPass, Notch };
enum class FilterModel { Standard = 0, Ladder, Vintage };
enum class FilterTarget { Osc1 = 0, Osc2, BothOscillators, Noise, All };

// Expose processSample from juce::dsp::LadderFilter (it's protected)
class ExposedLadderFilter : public juce::dsp::LadderFilter<float>
{
public:
    float processOneSample (float input) noexcept
    {
        updateSmoothers();
        return processSample (input, 0);
    }
};

class SubtractiveFilter
{
public:
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    float processSample (float input);

    void setCutoff (float frequencyHz);
    void setResonance (float resonance01);
    void setMode (FilterMode mode);
    void setModel (FilterModel model);
    void setSlope24dB (bool is24dB);

private:
    void updateCoefficients();
    void updateLadderMode();
    float processVintageSample (float input);

    FilterMode mode = FilterMode::LowPass;
    FilterModel model = FilterModel::Standard;
    bool use24dB = true;
    float targetCutoffHz = 8000.0f;
    float smoothedCutoffHz = 8000.0f;
    float resonance = 0.2f;
    double currentSampleRate = 44100.0;
    float smoothingCoeff = 0.995f;
    int updateCounter = 0;

    // Standard: two cascaded biquad stages for 12 or 24 dB/oct
    juce::dsp::IIR::Filter<float> filter1;
    juce::dsp::IIR::Filter<float> filter2;

    // Ladder: Moog-style 4-pole ladder via JUCE
    ExposedLadderFilter ladderFilter;

    // Vintage: 4-stage one-pole cascade with soft saturation
    static constexpr int kVintageStages = 4;
    std::array<float, kVintageStages> vintageState {};
    float vintageFeedback = 0.0f;
};

} // namespace tillysynth
