#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

namespace tillysynth
{

enum class ChorusMode { Off = 0, I, II, Both };

class ChorusEngine
{
public:
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void setMode (ChorusMode mode);
    void setRate (float hz);
    void setDepth (float depth01);

    void process (juce::AudioBuffer<float>& buffer);

private:
    struct DelayLine
    {
        std::vector<float> buffer;
        int writeIndex = 0;
        int maxLength = 0;

        void prepare (int length);
        void reset();
        void write (float sample);
        float readCubic (float delaySamples) const;
    };

    // BBD stage: models one bucket-brigade delay line with LFO, feedback, and tone shaping
    struct BBDStage
    {
        DelayLine delay;
        float lfoPhase = 0.0f;
        float baseDelayMs = 0.0f;
        float modDepthMs = 0.0f;
        float feedback = 0.0f;
        float prevOutput = 0.0f;

        // One-pole low-pass to emulate BBD high-frequency rolloff
        float lpState = 0.0f;
        float lpCoeff = 0.0f;
    };

    void processBBDStage (BBDStage& stage, const float* input, float* output,
                          int numSamples, float lfoRate, float mix);

    ChorusMode mode = ChorusMode::Off;
    float rate = 0.5f;
    float depth = 0.5f;
    double currentSampleRate = 44100.0;

    // Juno-style: two BBD stages with different characteristics
    BBDStage stageI;
    BBDStage stageII;

    // Pre-allocated work buffer
    std::vector<float> workBuffer;
};

} // namespace tillysynth
