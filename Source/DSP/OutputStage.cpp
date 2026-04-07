#include "OutputStage.h"
#include <cmath>

namespace tillysynth
{

void OutputStage::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    reset();
}

void OutputStage::reset()
{
    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        consoleLP[ch] = 0.0f;
        tapeBias[ch] = 0.0f;
        tapeLP[ch] = 0.0f;
    }
}

void OutputStage::setMode (OutputMode newMode)
{
    mode = newMode;
}

void OutputStage::setDrive (float drive01)
{
    drive = juce::jlimit (0.0f, 1.0f, drive01);
}

void OutputStage::setMix (float mix01)
{
    mix = juce::jlimit (0.0f, 1.0f, mix01);
}

void OutputStage::process (juce::AudioBuffer<float>& buffer)
{
    if (mode == OutputMode::Off || drive <= 0.0001f)
        return;

    int numChannels = juce::jmin (buffer.getNumChannels(), kMaxChannels);
    int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float dry = data[i];
            float wet = dry;

            switch (mode)
            {
                case OutputMode::SoftClip:
                    wet = processSampleSoftClip (dry);
                    break;
                case OutputMode::Console:
                    wet = processSampleConsole (dry, ch);
                    break;
                case OutputMode::Tape:
                    wet = processSampleTape (dry, ch);
                    break;
                case OutputMode::Off:
                    break;
            }

            data[i] = dry + (wet - dry) * mix;
        }
    }
}

float OutputStage::processSampleSoftClip (float input) const
{
    // Drive scales input into tanh saturation range (1x–5x gain)
    float gained = input * (1.0f + drive * 4.0f);
    return std::tanh (gained);
}

float OutputStage::processSampleConsole (float input, int channel)
{
    // Gentle mix-bus saturation: soft compression + HF rolloff
    float gained = input * (1.0f + drive * 2.0f);

    // Soft saturation curve — adds subtle even harmonics
    float saturated = gained / (1.0f + std::abs (gained));

    // One-pole low-pass for HF rolloff (console warmth)
    // Corner: 18 kHz at drive=0 → 8 kHz at drive=1
    float cornerHz = 18000.0f - drive * 10000.0f;
    float wc = juce::MathConstants<float>::twoPi * cornerHz
             / static_cast<float> (currentSampleRate);
    float g = wc / (1.0f + wc);

    consoleLP[channel] += (saturated - consoleLP[channel]) * g;
    return consoleLP[channel];
}

float OutputStage::processSampleTape (float input, int channel)
{
    // Tape-style saturation with hysteresis-like asymmetry
    float gained = input * (1.0f + drive * 3.0f);

    // Slow-moving bias that tracks signal — creates asymmetric
    // clipping characteristic of magnetic tape hysteresis
    float biasAmount = drive * 0.15f;
    tapeBias[channel] += (gained * biasAmount - tapeBias[channel]) * 0.001f;
    float biased = gained + tapeBias[channel];

    // Tape saturation: tanh with asymmetry from bias
    float saturated = std::tanh (biased);

    // Gentle HF loss (tape head losses), stronger with more drive
    float hfCoeff = 0.05f * drive;
    tapeLP[channel] += (saturated - tapeLP[channel]) * (1.0f - hfCoeff);
    return tapeLP[channel];
}

} // namespace tillysynth
