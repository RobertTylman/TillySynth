#include "NoiseOscillator.h"

namespace tillysynth
{

void NoiseOscillator::prepare (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    rng.setSeed (static_cast<juce::int64> (juce::Time::currentTimeMillis()
                                            + rng.nextInt64()));
    reset();
}

void NoiseOscillator::reset()
{
    pinkRows.fill (0.0f);
    pinkRunningSum = 0.0f;
    pinkCounter = 0;
    brownState = 0.0f;
    prevWhiteSample = 0.0f;
    shHeldValue = 0.0f;
    shPhase = 0.0f;
}

float NoiseOscillator::processSample()
{
    if (level <= 0.0f)
        return 0.0f;

    float sample = 0.0f;

    switch (noiseType)
    {
        case NoiseType::White:   sample = generateWhite();   break;
        case NoiseType::Pink:    sample = generatePink();     break;
        case NoiseType::Brown:   sample = generateBrown();    break;
        case NoiseType::Blue:    sample = generateBlue();     break;
        case NoiseType::Digital: sample = generateDigital();  break;
    }

    return sample * level;
}

void NoiseOscillator::setSHRate (float rateHz)
{
    if (sampleRate > 0.0)
        shPhaseInc = static_cast<float> (rateHz / sampleRate);
    else
        shPhaseInc = 0.0f;
}

float NoiseOscillator::generateWhite()
{
    return rng.nextFloat() * 2.0f - 1.0f;
}

float NoiseOscillator::generatePink()
{
    // Voss-McCartney algorithm: maintain kPinkRows random values refreshed
    // at binary-subdivided intervals. Each sample, check which rows need
    // updating based on trailing zeros of the counter.
    int changed = pinkCounter == 0 ? 0 : __builtin_ctz (static_cast<unsigned> (pinkCounter));
    if (changed >= kPinkRows)
        changed = kPinkRows - 1;

    pinkRunningSum -= pinkRows[static_cast<size_t> (changed)];
    float newVal = rng.nextFloat() * 2.0f - 1.0f;
    pinkRows[static_cast<size_t> (changed)] = newVal;
    pinkRunningSum += newVal;

    ++pinkCounter;
    if (pinkCounter >= (1 << kPinkRows))
        pinkCounter = 0;

    // Add a white noise component and normalise
    float white = rng.nextFloat() * 2.0f - 1.0f;
    return (pinkRunningSum + white) / static_cast<float> (kPinkRows + 1);
}

float NoiseOscillator::generateBrown()
{
    float white = rng.nextFloat() * 2.0f - 1.0f;
    brownState += (white - brownState) * 0.002f;

    // Normalise (brown noise has low amplitude, scale up)
    return juce::jlimit (-1.0f, 1.0f, brownState * 20.0f);
}

float NoiseOscillator::generateBlue()
{
    float white = rng.nextFloat() * 2.0f - 1.0f;
    float sample = white - prevWhiteSample;
    prevWhiteSample = white;

    // Normalise (differentiator output spans -2..2)
    return sample * 0.5f;
}

float NoiseOscillator::generateDigital()
{
    shPhase += shPhaseInc;
    if (shPhase >= 1.0f)
    {
        shPhase -= 1.0f;
        shHeldValue = rng.nextFloat() * 2.0f - 1.0f;
    }

    return shHeldValue;
}

} // namespace tillysynth
