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
            currentValue += attackCoeff;
            if (currentValue >= 1.0f)
            {
                currentValue = 1.0f;
                stage = Stage::Decay;
            }
            break;

        case Stage::Decay:
            currentValue -= decayCoeff;
            if (currentValue <= sustainLevel)
            {
                currentValue = sustainLevel;
                stage = Stage::Sustain;
            }
            break;

        case Stage::Sustain:
            currentValue = sustainLevel;
            break;

        case Stage::Release:
            currentValue -= releaseCoeff;
            if (currentValue <= 0.0f)
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
    auto samplesToCoeff = [] (float ms, double sr) -> float
    {
        if (ms <= 0.0f)
            return 1.0f;

        float samples = static_cast<float> (ms * 0.001f * sr);
        return 1.0f / samples;
    };

    attackCoeff = samplesToCoeff (attackMs, currentSampleRate);
    decayCoeff = (1.0f - sustainLevel) * samplesToCoeff (decayMs, currentSampleRate);
    releaseCoeff = samplesToCoeff (releaseMs, currentSampleRate);
}

} // namespace tillysynth
