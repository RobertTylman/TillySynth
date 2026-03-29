#pragma once
#include <juce_core/juce_core.h>
#include <array>

namespace tillysynth
{

enum class NoiseType { White = 0, Pink, Brown, Blue, Digital };

class NoiseOscillator
{
public:
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    float processSample();

    void setNoiseType (NoiseType type) { noiseType = type; }
    void setLevel (float level01) { level = level01; }
    void setSHRate (float rateHz);

private:
    float generateWhite();
    float generatePink();
    float generateBrown();
    float generateBlue();
    float generateDigital();

    NoiseType noiseType = NoiseType::White;
    float level = 0.0f;
    double sampleRate = 44100.0;

    juce::Random rng;

    // Pink noise (Voss-McCartney)
    static constexpr int kPinkRows = 16;
    std::array<float, kPinkRows> pinkRows {};
    float pinkRunningSum = 0.0f;
    int pinkCounter = 0;

    // Brown noise (leaky integrator)
    float brownState = 0.0f;

    // Blue noise (differentiator)
    float prevWhiteSample = 0.0f;

    // Digital S&H
    float shHeldValue = 0.0f;
    float shPhase = 0.0f;
    float shPhaseInc = 0.0f;
};

} // namespace tillysynth
