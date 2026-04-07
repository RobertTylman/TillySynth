#pragma once
#include <juce_dsp/juce_dsp.h>

namespace tillysynth
{

enum class OutputMode { Off = 0, SoftClip, Console, Tape };

class OutputStage
{
public:
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void process (juce::AudioBuffer<float>& buffer);

    void setMode (OutputMode newMode);
    void setDrive (float drive01);
    void setMix (float mix01);

private:
    float processSampleSoftClip (float input) const;
    float processSampleConsole (float input, int channel);
    float processSampleTape (float input, int channel);

    OutputMode mode = OutputMode::Off;
    float drive = 0.0f;
    float mix = 1.0f;
    double currentSampleRate = 44100.0;

    // Per-channel filter state
    static constexpr int kMaxChannels = 2;
    float consoleLP[kMaxChannels] {};
    float tapeBias[kMaxChannels] {};
    float tapeLP[kMaxChannels] {};
};

} // namespace tillysynth
