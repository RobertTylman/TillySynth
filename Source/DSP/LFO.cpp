#include "LFO.h"
#include <cmath>

namespace tillysynth
{

void LFO::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    phase = 0.0f;
    phaseIncrement = static_cast<float> (rate / sampleRate);
}

void LFO::reset()
{
    phase = 0.0f;
}

LFOOutput LFO::processSample()
{
    float sample = generateSample() * depth;

    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;

    LFOOutput output;
    if (destCutoff) output.cutoffMod = sample;
    if (destPitch)  output.pitchMod  = sample;
    if (destVolume) output.volumeMod = sample;
    if (destPW)     output.pwMod     = sample;

    return output;
}

void LFO::setWaveform (Waveform wf)
{
    waveform = wf;
}

void LFO::setRate (float hz)
{
    rate = hz;
    if (currentSampleRate > 0.0)
        phaseIncrement = static_cast<float> (rate / currentSampleRate);
}

void LFO::setDepth (float depth01)
{
    depth = depth01;
}

void LFO::setDestinations (bool cutoff, bool pitch, bool volume, bool pw)
{
    destCutoff = cutoff;
    destPitch  = pitch;
    destVolume = volume;
    destPW     = pw;
}

float LFO::generateSample()
{
    switch (waveform)
    {
        case Waveform::Sine:
            return std::sin (phase * juce::MathConstants<float>::twoPi);

        case Waveform::Sawtooth:
            return 2.0f * phase - 1.0f;

        case Waveform::Square:
            return (phase < 0.5f) ? 1.0f : -1.0f;

        case Waveform::Triangle:
            return (phase < 0.5f)
                ? (4.0f * phase - 1.0f)
                : (3.0f - 4.0f * phase);
    }

    return 0.0f;
}

} // namespace tillysynth
