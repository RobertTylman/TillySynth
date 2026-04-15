#include "SubtractiveFilter.h"
#include <cmath>

namespace tillysynth
{

void SubtractiveFilter::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Smoothing coefficient: ~2ms time constant
    float smoothSamples = static_cast<float> (sampleRate * 0.002);
    smoothingCoeff = 1.0f - 1.0f / smoothSamples;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    filter1.prepare (spec);
    filter2.prepare (spec);
    filter3.prepare (spec);
    ladderFilter.prepare (spec);

    vintageState.fill (0.0f);
    vintageFeedback = 0.0f;

    updateCoefficients();
}

void SubtractiveFilter::reset()
{
    filter1.reset();
    filter2.reset();
    filter3.reset();
    ladderFilter.reset();
    smoothedCutoffHz = targetCutoffHz;
    updateCounter = 0;
    vintageState.fill (0.0f);
    vintageFeedback = 0.0f;
}

float SubtractiveFilter::processSample (float input)
{
    // Smooth the cutoff toward target every sample
    smoothedCutoffHz += (targetCutoffHz - smoothedCutoffHz) * (1.0f - smoothingCoeff);

    switch (model)
    {
        case FilterModel::Standard:
        {
            if (++updateCounter >= 4)
            {
                updateCounter = 0;
                updateCoefficients();
            }

            int stages = 1;
            switch (slope)
            {
                case FilterSlope::dB24: stages = 2; break;
                case FilterSlope::dB36: stages = 3; break;
                case FilterSlope::dB12:
                case FilterSlope::dB6:
                default:                stages = 1; break;
            }

            float output = filter1.processSample (input);

            if (stages >= 2)
                output = filter2.processSample (output);
            if (stages >= 3)
                output = filter3.processSample (output);

            if (! std::isfinite (output))
                return 0.0f;

            output = juce::jlimit (-4.0f, 4.0f, output);

            return output;
        }

        case FilterModel::Ladder:
        {
            // Ladder filter has its own built-in smoothing
            ladderFilter.setCutoffFrequencyHz (smoothedCutoffHz);
            ladderFilter.setResonance (resonance);
            return ladderFilter.processOneSample (input);
        }

        case FilterModel::Vintage:
            return processVintageSample (input);
    }

    return input;
}

float SubtractiveFilter::processVintageSample (float input)
{
    // One-pole coefficient from smoothed cutoff
    float wc = juce::MathConstants<float>::twoPi * smoothedCutoffHz
             / static_cast<float> (currentSampleRate);
    float g = wc / (1.0f + wc);

    // Scale resonance into feedback (capped below self-oscillation)
    float k = resonance * 3.6f;

    // Number of active stages depends on slope setting
    int stages = 2;
    switch (slope)
    {
        case FilterSlope::dB6:  stages = 1; break;
        case FilterSlope::dB12: stages = 2; break;
        case FilterSlope::dB24: stages = 4; break;
        case FilterSlope::dB36: stages = 6; break;
        default:                stages = 2; break;
    }

    // Feedback from last active stage — soft-clipped for warmth
    float fb = std::tanh (vintageFeedback * k);
    float x = input - fb;

    // Cascade of one-pole low-pass stages with interleaved tanh saturation
    for (int i = 0; i < stages; ++i)
    {
        float v = (x - vintageState[static_cast<size_t> (i)]) * g;
        float lp = vintageState[static_cast<size_t> (i)] + v;
        vintageState[static_cast<size_t> (i)] = lp + v;
        x = std::tanh (lp);
    }

    vintageFeedback = x;

    // For high-pass: subtract low-pass output from input
    if (mode == FilterMode::HighPass)
        return input - x;

    // For band-pass: difference of two low-pass outputs (approximate)
    if (mode == FilterMode::BandPass)
        return (input - x) * x * 4.0f;

    // For notch: input minus twice the band-pass
    if (mode == FilterMode::Notch)
        return input - (input - x) * x * 8.0f;

    return x;
}

void SubtractiveFilter::setCutoff (float frequencyHz)
{
    targetCutoffHz = juce::jlimit (20.0f, 20000.0f, frequencyHz);
}

void SubtractiveFilter::setResonance (float resonance01)
{
    resonance = juce::jlimit (0.0f, 1.0f, resonance01);
}

void SubtractiveFilter::setMode (FilterMode m)
{
    mode = m;
    updateCoefficients();
    updateLadderMode();
}

void SubtractiveFilter::setModel (FilterModel m)
{
    model = m;
}

void SubtractiveFilter::setSlope (FilterSlope s)
{
    slope = s;
    updateCoefficients();
    updateLadderMode();
}

void SubtractiveFilter::updateLadderMode()
{
    using LM = juce::dsp::LadderFilterMode;

    bool use24 = (slope == FilterSlope::dB24 || slope == FilterSlope::dB36);

    if (use24)
    {
        switch (mode)
        {
            case FilterMode::LowPass:  ladderFilter.setMode (LM::LPF24); break;
            case FilterMode::HighPass:  ladderFilter.setMode (LM::HPF24); break;
            case FilterMode::BandPass:  ladderFilter.setMode (LM::BPF24); break;
            case FilterMode::Notch:     ladderFilter.setMode (LM::LPF24); break;
        }
    }
    else
    {
        switch (mode)
        {
            case FilterMode::LowPass:  ladderFilter.setMode (LM::LPF12); break;
            case FilterMode::HighPass:  ladderFilter.setMode (LM::HPF12); break;
            case FilterMode::BandPass:  ladderFilter.setMode (LM::BPF12); break;
            case FilterMode::Notch:     ladderFilter.setMode (LM::LPF12); break;
        }
    }
}

void SubtractiveFilter::updateCoefficients()
{
    // Map resonance 0-1 to Q with a gentler curve to avoid unstable
    // self-oscillation when multiple stages are cascaded.
    float q = 0.5f + (resonance * resonance) * 9.5f;
    if (slope == FilterSlope::dB24)
        q = juce::jmin (q, 4.0f);
    else if (slope == FilterSlope::dB36)
        q = juce::jmin (q, 2.5f);

    float cutoff = juce::jlimit (20.0f, 20000.0f, smoothedCutoffHz);

    auto makeCoeffs = [this, cutoff] (float stageQ) -> juce::dsp::IIR::Coefficients<float>::Ptr
    {
        switch (mode)
        {
            case FilterMode::LowPass:
                return juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, cutoff, stageQ);
            case FilterMode::HighPass:
                return juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, cutoff, stageQ);
            case FilterMode::BandPass:
                return juce::dsp::IIR::Coefficients<float>::makeBandPass (currentSampleRate, cutoff, stageQ);
            case FilterMode::Notch:
                return juce::dsp::IIR::Coefficients<float>::makeNotch (currentSampleRate, cutoff, stageQ);
        }

        return {};
    };

    juce::dsp::IIR::Coefficients<float>::Ptr coeffs;
    bool firstOrder = (slope == FilterSlope::dB6)
                   && (mode == FilterMode::LowPass || mode == FilterMode::HighPass);

    if (firstOrder)
    {
        if (mode == FilterMode::LowPass)
            coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (currentSampleRate, cutoff);
        else
            coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (currentSampleRate, cutoff);
    }
    else
    {
        coeffs = makeCoeffs (q);
    }

    if (coeffs != nullptr)
    {
        *filter1.coefficients = *coeffs;

        // Keep additional stages near Butterworth to prevent runaway peaks.
        auto shapingCoeffs = firstOrder ? coeffs : makeCoeffs (0.7071f);
        if (shapingCoeffs != nullptr)
        {
            *filter2.coefficients = *shapingCoeffs;
            *filter3.coefficients = *shapingCoeffs;
        }
        else
        {
            *filter2.coefficients = *coeffs;
            *filter3.coefficients = *coeffs;
        }
    }
}

} // namespace tillysynth
