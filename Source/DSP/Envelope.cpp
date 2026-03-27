#include "Envelope.h"
#include <cmath>

namespace tillysynth
{

void Envelope::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    reset();
}

void Envelope::reset()
{
    stage = Stage::Idle;
    currentValue = 0.0f;
}

void Envelope::setParameters (float attack, float decay, float sustain, float release)
{
    attackMs = attack;
    decayMs = decay;
    sustainLevel = sustain;
    releaseMs = release;
    calculateCoefficients();
}

void Envelope::noteOn()
{
    stage = Stage::Attack;
}

void Envelope::noteOff()
{
    if (stage != Stage::Idle)
        stage = Stage::Release;
}

float Envelope::processSample()
{
    switch (stage)
    {
        case Stage::Idle:
            return 0.0f;

        case Stage::Attack:
            // Exponential attack: multiply toward overshoot target
            currentValue = attackBase + currentValue * attackCoeff;
            if (currentValue >= 1.0f)
            {
                currentValue = 1.0f;
                stage = Stage::Decay;
            }
            break;

        case Stage::Decay:
            // Exponential decay toward sustain level
            currentValue = decayBase + currentValue * decayCoeff;
            if (currentValue <= sustainLevel + 0.0001f)
            {
                currentValue = sustainLevel;
                stage = Stage::Sustain;
            }
            break;

        case Stage::Sustain:
            currentValue = sustainLevel;
            break;

        case Stage::Release:
            // Exponential release toward zero
            currentValue = releaseBase + currentValue * releaseCoeff;
            if (currentValue <= 0.0001f)
            {
                currentValue = 0.0f;
                stage = Stage::Idle;
            }
            break;
    }

    return currentValue;
}

void Envelope::calculateCoefficients()
{
    // Attempt to create smooth exponential curves.
    // For each stage, we compute coeff = exp(-1 / (time_in_samples * rate_constant))
    // The "base" offsets the curve so it converges to the target level.

    auto calcCoeff = [] (float timeMs, double sr) -> float
    {
        if (timeMs <= 0.0f)
            return 0.0f; // instant

        float samples = static_cast<float> (timeMs * 0.001f * sr);
        // Use a time constant that reaches ~99.3% in the given time
        return std::exp (-std::log (1000.0f) / samples);
    };

    attackCoeff = calcCoeff (attackMs, currentSampleRate);
    // Attack goes from current toward 1.0, but we overshoot slightly
    // to avoid the exponential curve flattening near the target.
    // target = 1.0, so base = (1 - coeff) * target
    attackBase = (1.0f - attackCoeff);

    decayCoeff = calcCoeff (decayMs, currentSampleRate);
    // Decay goes from 1.0 toward sustainLevel
    decayBase = (1.0f - decayCoeff) * sustainLevel;

    releaseCoeff = calcCoeff (releaseMs, currentSampleRate);
    // Release goes from sustainLevel toward 0
    releaseBase = 0.0f; // target is 0, so base = (1 - coeff) * 0 = 0
}

} // namespace tillysynth
