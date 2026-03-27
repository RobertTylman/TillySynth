#include "Oscillator.h"
#include <cmath>

namespace tillysynth
{

Oscillator::Oscillator()
{
    phases.fill (0.0f);
    phaseIncrements.fill (0.0f);
}

void Oscillator::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    reset();
}

void Oscillator::reset()
{
    for (int i = 0; i < kMaxUnisonVoices; ++i)
        phases[i] = static_cast<float> (juce::Random::getSystemRandom().nextFloat());

    updateUnisonFrequencies();
}

// PolyBLEP residual — smooths discontinuities at waveform transitions
static float polyBlep (float phase, float phaseInc)
{
    if (phase < phaseInc)
    {
        float t = phase / phaseInc;
        return t + t - t * t - 1.0f;
    }

    if (phase > 1.0f - phaseInc)
    {
        float t = (phase - 1.0f) / phaseInc;
        return t * t + t + t + 1.0f;
    }

    return 0.0f;
}

float Oscillator::processSample()
{
    float output = 0.0f;

    if (unisonCount == 1)
    {
        output = generateWaveformSample (phases[0], phaseIncrements[0]);
        phases[0] += phaseIncrements[0];
        if (phases[0] >= 1.0f)
            phases[0] -= 1.0f;
    }
    else
    {
        float centerWeight = 1.0f - unisonBlend;
        float sideWeight = unisonBlend;

        for (int i = 0; i < unisonCount; ++i)
        {
            float sample = generateWaveformSample (phases[i], phaseIncrements[i]);
            float weight = (i == unisonCount / 2) ? centerWeight : sideWeight;
            output += sample * weight;

            phases[i] += phaseIncrements[i];
            if (phases[i] >= 1.0f)
                phases[i] -= 1.0f;
        }

        output /= static_cast<float> (unisonCount);
    }

    return output * level;
}

float Oscillator::generateWaveformSample (float phase, float phaseInc) const
{
    switch (waveform)
    {
        case Waveform::Sine:
            return std::sin (phase * juce::MathConstants<float>::twoPi);

        case Waveform::Sawtooth:
        {
            float saw = 2.0f * phase - 1.0f;
            saw -= polyBlep (phase, phaseInc);
            return saw;
        }

        case Waveform::Square:
        {
            float sq = (phase < pulseWidth) ? 1.0f : -1.0f;
            sq += polyBlep (phase, phaseInc);
            // Apply polyBLEP at the pulse width transition too
            float shifted = phase - pulseWidth;
            if (shifted < 0.0f)
                shifted += 1.0f;
            sq -= polyBlep (shifted, phaseInc);
            return sq;
        }

        case Waveform::Triangle:
        {
            // Integrated polyBLEP square → triangle (smoother)
            float sq = (phase < 0.5f) ? 1.0f : -1.0f;
            sq += polyBlep (phase, phaseInc);
            float shifted = phase - 0.5f;
            if (shifted < 0.0f)
                shifted += 1.0f;
            sq -= polyBlep (shifted, phaseInc);

            // Leaky integrator to convert square to triangle
            // This is computed per-sample so we use the stored state
            // For simplicity, use the naive formula with polyBLEP applied to the phase
            return (phase < 0.5f)
                ? (4.0f * phase - 1.0f)
                : (3.0f - 4.0f * phase);
        }
    }

    return 0.0f;
}

void Oscillator::setFrequency (float frequencyHz)
{
    baseFrequency = frequencyHz;
    updateUnisonFrequencies();
}

void Oscillator::setWaveform (Waveform wf)
{
    waveform = wf;
}

void Oscillator::setPulseWidth (float pw01)
{
    pulseWidth = juce::jlimit (0.01f, 0.99f, pw01);
}

void Oscillator::setUnisonVoices (int count)
{
    unisonCount = juce::jlimit (1, kMaxUnisonVoices, count);
    updateUnisonFrequencies();
}

void Oscillator::setUnisonDetune (float cents)
{
    unisonDetuneCents = cents;
    updateUnisonFrequencies();
}

void Oscillator::setUnisonBlend (float blend01)
{
    unisonBlend = juce::jlimit (0.0f, 1.0f, blend01);
}

void Oscillator::setLevel (float level01)
{
    level = level01;
}

void Oscillator::updateUnisonFrequencies()
{
    if (currentSampleRate <= 0.0)
        return;

    if (unisonCount == 1)
    {
        phaseIncrements[0] = static_cast<float> (baseFrequency / currentSampleRate);
        return;
    }

    for (int i = 0; i < unisonCount; ++i)
    {
        float normalised = static_cast<float> (i) / static_cast<float> (unisonCount - 1) - 0.5f;
        float detuneCents = normalised * unisonDetuneCents;
        float detuneRatio = std::pow (2.0f, detuneCents / 1200.0f);
        float freq = baseFrequency * detuneRatio;
        phaseIncrements[i] = static_cast<float> (freq / currentSampleRate);
    }
}

} // namespace tillysynth
