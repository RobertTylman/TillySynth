#include "ChorusEngine.h"
#include <algorithm>
#include <cmath>

namespace tillysynth
{

// --- DelayLine ---

void ChorusEngine::DelayLine::prepare (int length)
{
    // Allocate and clear the circular buffer.
    //
    // `length` is set to the max delay we ever need plus interpolation guard
    // samples. Keeping this fixed avoids reallocations in the audio callback.
    maxLength = length;
    buffer.assign (static_cast<size_t> (length), 0.0f);
    writeIndex = 0;
}

void ChorusEngine::DelayLine::reset()
{
    // Reset to silence between transport stops / sample-rate changes.
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}

void ChorusEngine::DelayLine::write (float sample)
{
    // Write current input at the write head, then advance with wraparound.
    buffer[static_cast<size_t> (writeIndex)] = sample;
    writeIndex = (writeIndex + 1) % maxLength;
}

float ChorusEngine::DelayLine::readCubic (float delaySamples) const
{
    // Read from "now - delaySamples" in the circular buffer.
    //
    // Because modulation makes delay fractional (e.g. 132.42 samples),
    // we cannot just read one integer index without introducing stepping.
    // So we interpolate between nearby samples.
    float readPos = static_cast<float> (writeIndex) - delaySamples;
    while (readPos < 0.0f)
        readPos += static_cast<float> (maxLength);

    // Collect four neighboring samples around the read position.
    // i1 and i2 straddle the fractional location; i0 and i3 provide slope info.
    int i0 = (static_cast<int> (readPos) - 1 + maxLength) % maxLength;
    int i1 = static_cast<int> (readPos) % maxLength;
    int i2 = (i1 + 1) % maxLength;
    int i3 = (i2 + 1) % maxLength;

    // Fractional position between i1 and i2.
    float frac = readPos - std::floor (readPos);

    float y0 = buffer[static_cast<size_t> (i0)];
    float y1 = buffer[static_cast<size_t> (i1)];
    float y2 = buffer[static_cast<size_t> (i2)];
    float y3 = buffer[static_cast<size_t> (i3)];

    // Cubic Hermite interpolation:
    //
    // We fit a cubic polynomial through points using local slope estimates.
    // Intuition:
    // - Linear interpolation only uses two points and creates sharper corners
    //   as delay moves, which can sound rougher.
    // - Cubic Hermite uses four points and inferred tangents, producing
    //   smoother trajectories as read position glides across samples.
    //
    // This is especially useful in chorus/flanger/vibrato where delay time is
    // continuously modulated and read position is always moving.
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    // Evaluate cubic at `frac`.
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

// --- ChorusEngine ---

void ChorusEngine::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    // Juno-like delay window tops out around 5.35 ms (for modes I/II).
    // We allocate to that maximum and add +4 samples as guard points so cubic
    // interpolation can safely read i0..i3 around the read head.
    int maxDelaySamples = static_cast<int> (std::ceil (sampleRate * 0.00535)) + 4;

    delayLeft.prepare (maxDelaySamples);
    delayRight.prepare (maxDelaySamples);

    // Pre-BBD low-pass coefficient.
    //
    // One-pole low-pass in "leaky integrator" form:
    // y += a * (x - y), where a = 1 - exp(-2*pi*fc/fs).
    //
    // We choose ~8 kHz here to remove some top-end before delay modulation,
    // approximating the anti-artifact/voicing role around BBD paths.
    preLpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 8000.0f
                                  / static_cast<float> (sampleRate));

    // Post-BBD smoothing coefficient (~7 kHz) for wet-path softening.
    // This creates a slight rounded/analog contour in delayed signal.
    outLpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 7000.0f
                                  / static_cast<float> (sampleRate));

    reset();
}

void ChorusEngine::reset()
{
    // Clear both delay memories and all filter/LFO states.
    delayLeft.reset();
    delayRight.reset();
    lfoPhase = 0.0f;
    preLp1State = 0.0f;
    preLp2State = 0.0f;
    outLpLeft = 0.0f;
    outLpRight = 0.0f;
}

void ChorusEngine::setMode (ChorusMode m)
{
    // Mode is switched sample-accurately on next process call.
    mode = m;
}

void ChorusEngine::setDepth (float mix01)
{
    // Re-purposed from modulation depth to global dry/wet blend.
    dryWetMix = mix01;
}

ChorusEngine::ModeSpec ChorusEngine::getModeSpec() const
{
    // Canonical mode constants from the requested Juno-style spec.
    //
    // I, II:
    // - triangle LFO
    // - wide delay excursion
    // - stereo (L/R inverse modulation)
    //
    // I+II:
    // - fast LFO (Leslie-like shimmer zone)
    // - narrow delay excursion
    // - mono output
    // - sine shape for smoother high-rate motion
    switch (mode)
    {
        case ChorusMode::I:
            return { 0.513f, 0.00166f, 0.00535f, false, false };
        case ChorusMode::II:
            return { 0.863f, 0.00166f, 0.00535f, false, false };
        case ChorusMode::Both:
            return { 9.75f, 0.00330f, 0.00370f, true, true };
        case ChorusMode::Off:
        default:
            return {};
    }
}

float ChorusEngine::getLfoValue (float phase, bool useSine) const
{
    // For I+II mode we use sine to keep very fast modulation smoother.
    if (useSine)
        return std::sin (phase * juce::MathConstants<float>::twoPi);

    // Symmetric bipolar triangle in [-1, +1].
    // phase in [0, 1):
    // 0.0 -> -1, 0.25 -> 0, 0.5 -> +1, 0.75 -> 0, 1.0 -> -1.
    return (phase < 0.5f)
        ? (4.0f * phase - 1.0f)
        : (3.0f - 4.0f * phase);
}

float ChorusEngine::processDelayLine (DelayLine& line, float input, float delaySamples)
{
    // Write/read order models real-time delay behavior:
    // current sample enters the line, then we read the delayed tap.
    line.write (input);
    float delayed = line.readCubic (delaySamples);

    // Mild transfer non-linearity:
    // BBD paths are not perfectly linear; this gently rounds peaks and can
    // emulate "softened saw" character when driven.
    constexpr float drive = 1.15f;
    delayed = std::tanh (delayed * drive) / std::tanh (drive);

    return delayed;
}

void ChorusEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (mode == ChorusMode::Off)
        return;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    auto spec = getModeSpec();
    if (spec.lfoRateHz <= 0.0f || numChannels <= 0)
        return;

    // The historic architecture conceptually starts from a mono feed and then
    // creates stereo movement in the chorus network. We mirror that by
    // averaging input channels into `monoIn` per sample.
    float* left = buffer.getWritePointer (0);
    float* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    // Juno wet calibration:
    // the wet path is approximately -1.62 dB relative to dry at full wet.
    constexpr float wetCalibration = 0.829f; // -1.62 dB

    // Phase advance per sample.
    float phaseInc = spec.lfoRateHz / static_cast<float> (currentSampleRate);

    // Convert spec delay bounds from seconds to samples.
    float minDelaySamples = spec.minDelaySec * static_cast<float> (currentSampleRate);
    float maxDelaySamples = spec.maxDelaySec * static_cast<float> (currentSampleRate);
    float centerDelay = 0.5f * (minDelaySamples + maxDelaySamples);
    float depthSamples = 0.5f * (maxDelaySamples - minDelaySamples);

    // Dry/wet control:
    // 0.0 -> fully dry
    // 1.0 -> full calibrated wet amount with dry removed
    float wetMix = juce::jlimit (0.0f, 1.0f, dryWetMix);
    float dryGain = 1.0f - wetMix;
    float wetGain = wetMix * wetCalibration;

    for (int i = 0; i < numSamples; ++i)
    {
        float inL = left[i];
        float inR = right != nullptr ? right[i] : inL;
        float monoIn = 0.5f * (inL + inR);

        // Two cascaded one-pole sections approximate ~12 dB/oct low-pass.
        // This acts as pre-conditioning before delay modulation.
        preLp1State += preLpCoeff * (monoIn - preLp1State);
        preLp2State += preLpCoeff * (preLp1State - preLp2State);
        float bbdInput = preLp2State;

        // One shared LFO drives both delay lines.
        // Left sees +LFO, right sees -LFO => 180-degree inversion.
        float lfo = getLfoValue (lfoPhase, spec.useSineLfo);

        // Compute variable delay taps and clamp for interpolation safety:
        // minimum 2 samples, maximum maxLength-4 (needed by cubic neighborhood).
        float delayL = juce::jlimit (2.0f, static_cast<float> (delayLeft.maxLength - 4),
                                     centerDelay + lfo * depthSamples);
        float delayR = juce::jlimit (2.0f, static_cast<float> (delayRight.maxLength - 4),
                                     centerDelay - lfo * depthSamples);

        // Process each BBD path independently with same input.
        float wetL = processDelayLine (delayLeft, bbdInput, delayL);
        float wetR = processDelayLine (delayRight, bbdInput, delayR);

        // Post smoothing on wet outputs.
        outLpLeft += outLpCoeff * (wetL - outLpLeft);
        outLpRight += outLpCoeff * (wetR - outLpRight);

        if (spec.isMonoOut)
        {
            // Mode I+II: mono output behavior (both channels carry same signal).
            float monoWet = 0.5f * (outLpLeft + outLpRight);
            float out = dryGain * monoIn + wetGain * monoWet;
            left[i] = out;
            if (right != nullptr)
                right[i] = out;
        }
        else
        {
            // Modes I/II: stereo output, with channel-specific wet taps.
            left[i] = dryGain * monoIn + wetGain * outLpLeft;
            if (right != nullptr)
                right[i] = dryGain * monoIn + wetGain * outLpRight;
        }

        // Advance and wrap normalized phase [0, 1).
        lfoPhase += phaseInc;
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;
    }
}

} // namespace tillysynth
