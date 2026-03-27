#include "ChorusEngine.h"
#include <cmath>

namespace tillysynth
{

// --- DelayLine ---

void ChorusEngine::DelayLine::prepare (int length)
{
    maxLength = length;
    buffer.assign (static_cast<size_t> (length), 0.0f);
    writeIndex = 0;
}

void ChorusEngine::DelayLine::reset()
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}

void ChorusEngine::DelayLine::write (float sample)
{
    buffer[static_cast<size_t> (writeIndex)] = sample;
    writeIndex = (writeIndex + 1) % maxLength;
}

float ChorusEngine::DelayLine::readCubic (float delaySamples) const
{
    // Cubic Hermite interpolation for smoother delay reading
    float readPos = static_cast<float> (writeIndex) - delaySamples;
    while (readPos < 0.0f)
        readPos += static_cast<float> (maxLength);

    int i0 = (static_cast<int> (readPos) - 1 + maxLength) % maxLength;
    int i1 = static_cast<int> (readPos) % maxLength;
    int i2 = (i1 + 1) % maxLength;
    int i3 = (i2 + 1) % maxLength;

    float frac = readPos - std::floor (readPos);

    float y0 = buffer[static_cast<size_t> (i0)];
    float y1 = buffer[static_cast<size_t> (i1)];
    float y2 = buffer[static_cast<size_t> (i2)];
    float y3 = buffer[static_cast<size_t> (i3)];

    // Hermite interpolation
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

// --- ChorusEngine ---

void ChorusEngine::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // 50ms max delay covers the full BBD range
    int maxDelaySamples = static_cast<int> (sampleRate * 0.05) + 4;

    stageI.delay.prepare (maxDelaySamples);
    stageII.delay.prepare (maxDelaySamples);

    // Juno-style chorus parameters:
    // Stage I: shorter delay, moderate rate — adds thickness
    stageI.baseDelayMs = 4.6f;
    stageI.modDepthMs = 1.5f;
    stageI.feedback = 0.15f;
    stageI.lfoPhase = 0.0f;
    stageI.lpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 8000.0f
                                        / static_cast<float> (sampleRate));

    // Stage II: longer delay, slower rate — adds width and movement
    stageII.baseDelayMs = 6.8f;
    stageII.modDepthMs = 2.2f;
    stageII.feedback = 0.12f;
    stageII.lfoPhase = 0.0f;
    stageII.lpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 6500.0f
                                         / static_cast<float> (sampleRate));

    workBuffer.resize (static_cast<size_t> (samplesPerBlock), 0.0f);

    reset();
}

void ChorusEngine::reset()
{
    stageI.delay.reset();
    stageII.delay.reset();
    stageI.lfoPhase = 0.0f;
    stageII.lfoPhase = 0.25f;  // quarter-phase offset for stereo image
    stageI.prevOutput = 0.0f;
    stageII.prevOutput = 0.0f;
    stageI.lpState = 0.0f;
    stageII.lpState = 0.0f;
}

void ChorusEngine::setMode (ChorusMode m)
{
    mode = m;
}

void ChorusEngine::setRate (float hz)
{
    rate = hz;
}

void ChorusEngine::setDepth (float depth01)
{
    depth = depth01;
}

void ChorusEngine::processBBDStage (BBDStage& stage, const float* input, float* output,
                                     int numSamples, float lfoRate, float mix)
{
    float phaseInc = static_cast<float> (lfoRate / currentSampleRate);
    float baseDelaySamples = stage.baseDelayMs * 0.001f * static_cast<float> (currentSampleRate);
    float modDepthSamples = stage.modDepthMs * depth * 0.001f * static_cast<float> (currentSampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        // Tri-shaped LFO for smoother modulation (less zipper than sine at extremes)
        float triLfo = (stage.lfoPhase < 0.5f)
            ? (4.0f * stage.lfoPhase - 1.0f)
            : (3.0f - 4.0f * stage.lfoPhase);

        // Subtle secondary modulation for wow/flutter character
        float flutter = std::sin (stage.lfoPhase * juce::MathConstants<float>::twoPi * 5.7f) * 0.08f;
        float wow = std::sin (stage.lfoPhase * juce::MathConstants<float>::twoPi * 0.3f) * 0.05f;

        float totalMod = triLfo + flutter + wow;

        float delaySamples = baseDelaySamples + totalMod * modDepthSamples;
        delaySamples = juce::jlimit (2.0f, static_cast<float> (stage.delay.maxLength - 4), delaySamples);

        // Write input + feedback into delay
        float feedbackSample = input[i] + stage.prevOutput * stage.feedback;
        stage.delay.write (feedbackSample);

        // Read with cubic interpolation
        float delayed = stage.delay.readCubic (delaySamples);

        // BBD low-pass: bucket-brigade chips naturally roll off highs
        stage.lpState += stage.lpCoeff * (delayed - stage.lpState);
        delayed = stage.lpState;

        stage.prevOutput = delayed;

        // Mix: dry stays centred, wet is the chorus voice
        output[i] = input[i] * (1.0f - mix * 0.5f) + delayed * mix;

        stage.lfoPhase += phaseInc;
        if (stage.lfoPhase >= 1.0f)
            stage.lfoPhase -= 1.0f;
    }
}

void ChorusEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (mode == ChorusMode::Off)
        return;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Ensure work buffer is large enough
    if (static_cast<int> (workBuffer.size()) < numSamples)
        workBuffer.resize (static_cast<size_t> (numSamples), 0.0f);

    // Juno chorus rates: I is faster, II is slower
    float rateI = rate * 1.2f;
    float rateII = rate * 0.8f;

    if (numChannels == 1)
    {
        float* data = buffer.getWritePointer (0);

        if (mode == ChorusMode::I || mode == ChorusMode::Both)
            processBBDStage (stageI, data, data, numSamples, rateI, 0.5f);

        if (mode == ChorusMode::II || mode == ChorusMode::Both)
            processBBDStage (stageII, data, data, numSamples, rateII, 0.5f);
    }
    else
    {
        // Stereo: stage I favours left, stage II favours right for width
        float* left = buffer.getWritePointer (0);
        float* right = buffer.getWritePointer (1);

        // Create mono input for chorus processing
        std::copy (left, left + numSamples, workBuffer.begin());

        if (mode == ChorusMode::I)
        {
            processBBDStage (stageI, workBuffer.data(), left, numSamples, rateI, 0.5f);
            std::copy (left, left + numSamples, right);
        }
        else if (mode == ChorusMode::II)
        {
            processBBDStage (stageII, workBuffer.data(), left, numSamples, rateII, 0.5f);
            std::copy (left, left + numSamples, right);
        }
        else if (mode == ChorusMode::Both)
        {
            // True stereo: each stage to a different channel
            processBBDStage (stageI, workBuffer.data(), left, numSamples, rateI, 0.55f);
            processBBDStage (stageII, workBuffer.data(), right, numSamples, rateII, 0.55f);
        }
    }
}

} // namespace tillysynth
