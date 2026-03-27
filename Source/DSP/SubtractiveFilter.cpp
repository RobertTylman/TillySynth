#include "SubtractiveFilter.h"

namespace tillysynth
{

void SubtractiveFilter::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    filter1.prepare (spec);
    filter2.prepare (spec);

    updateCoefficients();
}

void SubtractiveFilter::reset()
{
    filter1.reset();
    filter2.reset();
}

float SubtractiveFilter::processSample (float input)
{
    float output = filter1.processSample (input);

    if (use24dB)
        output = filter2.processSample (output);

    return output;
}

void SubtractiveFilter::setCutoff (float frequencyHz)
{
    cutoffHz = juce::jlimit (20.0f, 20000.0f, frequencyHz);
    updateCoefficients();
}

void SubtractiveFilter::setResonance (float resonance01)
{
    resonance = juce::jlimit (0.0f, 1.0f, resonance01);
    updateCoefficients();
}

void SubtractiveFilter::setMode (FilterMode m)
{
    mode = m;
    updateCoefficients();
}

void SubtractiveFilter::setSlope24dB (bool is24dB)
{
    use24dB = is24dB;
}

void SubtractiveFilter::updateCoefficients()
{
    // Map resonance 0-1 to Q 0.5-20
    float q = 0.5f + resonance * 19.5f;

    juce::dsp::IIR::Coefficients<float>::Ptr coeffs;

    switch (mode)
    {
        case FilterMode::LowPass:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, cutoffHz, q);
            break;
        case FilterMode::HighPass:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, cutoffHz, q);
            break;
        case FilterMode::BandPass:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (currentSampleRate, cutoffHz, q);
            break;
        case FilterMode::Notch:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeNotch (currentSampleRate, cutoffHz, q);
            break;
    }

    if (coeffs != nullptr)
    {
        *filter1.coefficients = *coeffs;
        *filter2.coefficients = *coeffs;
    }
}

} // namespace tillysynth
