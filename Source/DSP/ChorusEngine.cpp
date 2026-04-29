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
    // We allocate up to 6.0 ms so the per-channel center-delay mismatch in
    // process() can push the longer-side read position past the nominal
    // 5.35 ms peak without clamping the LFO excursion. +4 samples are guard
    // points so cubic interpolation can safely read i0..i3 around the head.
    int maxDelaySamples = static_cast<int> (std::ceil (sampleRate * 0.00600)) + 4;

    delayLeft.prepare (maxDelaySamples);
    delayRight.prepare (maxDelaySamples);

    if (sampleRate <= 0.0)
        return;

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
    lfoPhaseLeft = 0.0f;
    lfoPhaseRight = 0.0f;
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
    // Authentic Juno-60 chorus spec.
    //
    // Hardware topology: one triangle LFO modulates two 256-step BBD lines.
    // Left sees +LFO, right sees -LFO (180-degree inversion). Modes I and II
    // output stereo; mode I+II is mono and acts like a fast Leslie-type
    // shimmer. Rates and delay bounds are taken from the reverse-engineered
    // Juno-60 values (service notes quote rounded figures: 0.5 / 0.83 / 1 Hz,
    // the last almost certainly a typo for 10 Hz).
    switch (mode)
    {
        case ChorusMode::I:
            return { 0.513f, 0.0f, 0.00166f, 0.00535f, false };
        case ChorusMode::II:
            return { 0.863f, 0.0f, 0.00166f, 0.00535f, false };
        case ChorusMode::Both:
            return { 9.75f,  0.0f, 0.00330f, 0.00370f, true  };
        case ChorusMode::Off:
        default:
            return {};
    }
}

float ChorusEngine::getTriangleLfo (float phase) const
{
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
    if (mode == ChorusMode::Off || currentSampleRate <= 0.0)
        return;

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    auto spec = getModeSpec();
    if (spec.lfoRateLeftHz <= 0.0f || numChannels <= 0)
        return;

    // The historic architecture conceptually starts from a mono feed and then
    // creates stereo movement in the chorus network. We mirror that by
    // averaging input channels into `monoIn` per sample.
    float* left = buffer.getWritePointer (0);
    float* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    // Wet-path gain at full mix.
    //
    // The hardware attenuates the wet bus by ~1.62 dB, but in practice that
    // made "max chorus" feel underpowered against the dry path. Running the
    // wet at unity gives more presence at high mix settings while staying
    // well inside sane headroom (the BBD tanh non-linearity caps peaks).
    constexpr float wetCalibration = 1.0f;

    // Phase advances per sample for each channel's LFO.
    // lfoRateRightHz == 0 signals "mirror left with inverted polarity"
    // (modes I, II). Otherwise the right channel runs independently (I+II).
    float phaseIncLeft = spec.lfoRateLeftHz / static_cast<float> (currentSampleRate);
    float phaseIncRight = spec.lfoRateRightHz / static_cast<float> (currentSampleRate);
    bool independentRight = spec.lfoRateRightHz > 0.0f;

    // Convert spec delay bounds from seconds to samples.
    float minDelaySamples = spec.minDelaySec * static_cast<float> (currentSampleRate);
    float maxDelaySamples = spec.maxDelaySec * static_cast<float> (currentSampleRate);
    float centerDelay = 0.5f * (minDelaySamples + maxDelaySamples);
    float depthSamples = 0.5f * (maxDelaySamples - minDelaySamples);

    // Per-channel center-delay mismatch.
    //
    // On real Juno hardware the two BBD lines are clocked from the same LFO
    // but their absolute delay-per-stage isn't perfectly matched — small
    // component tolerances make the two paths settle at slightly different
    // center delays. That mismatch decorrelates the two wet taps beyond what
    // pure +/- LFO polarity achieves and is a large part of why the hardware
    // chorus feels wide rather than merely "out of phase."
    //
    // We only apply it in the stereo modes (I, II). Mode I+II is mono-sum by
    // spec, so any L/R delay offset collapses in the sum and adds nothing —
    // keeping centers matched there stays closer to the hardware tap values.
    constexpr float centerMismatch = 0.06f;
    float centerL = spec.isMonoOut ? centerDelay : centerDelay * (1.0f - centerMismatch);
    float centerR = spec.isMonoOut ? centerDelay : centerDelay * (1.0f + centerMismatch);

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

        // Resolve each channel's modulation value.
        // Modes I, II: right mirrors left with inverted polarity (cheap, classic).
        // Mode I+II:   right runs its own LFO at the Mode-II rate, so the two
        //              channels beat against each other continuously.
        float lfoL = getTriangleLfo (lfoPhaseLeft);
        float lfoR = independentRight ? getTriangleLfo (lfoPhaseRight) : -lfoL;

        // Compute variable delay taps and clamp for interpolation safety:
        // minimum 2 samples, maximum maxLength-4 (needed by cubic neighborhood).
        // Each side uses its own center to introduce the BBD clock mismatch.
        float delayL = juce::jlimit (2.0f, static_cast<float> (delayLeft.maxLength - 4),
                                     centerL + lfoL * depthSamples);
        float delayR = juce::jlimit (2.0f, static_cast<float> (delayRight.maxLength - 4),
                                     centerR + lfoR * depthSamples);

        // Process each BBD path independently with same input.
        float wetL = processDelayLine (delayLeft, bbdInput, delayL);
        float wetR = processDelayLine (delayRight, bbdInput, delayR);

        // Post smoothing on wet outputs.
        outLpLeft += outLpCoeff * (wetL - outLpLeft);
        outLpRight += outLpCoeff * (wetR - outLpRight);

        if (spec.isMonoOut)
        {
            // Mode I+II: hardware sums both wet taps and drives both output
            // channels identically. Dry is summed to mono here too so the
            // whole mode's output is genuinely mono, matching the spec.
            float monoWet = 0.5f * (outLpLeft + outLpRight);
            float out = dryGain * monoIn + wetGain * monoWet;
            left[i] = out;
            if (right != nullptr)
                right[i] = out;
        }
        else
        {
            // Modes I, II: stereo output. BBD lines still see the mono sum
            // (authentic "one HPF feeds both BBDs" topology), but the dry
            // path passes through in native L/R so any upstream stereo
            // content (drift, pan, etc.) survives to the output.
            left[i] = dryGain * inL + wetGain * outLpLeft;
            if (right != nullptr)
                right[i] = dryGain * inR + wetGain * outLpRight;
        }

        // Advance both phases; right only moves when it has an independent rate.
        lfoPhaseLeft += phaseIncLeft;
        if (lfoPhaseLeft >= 1.0f)
            lfoPhaseLeft -= 1.0f;

        if (independentRight)
        {
            lfoPhaseRight += phaseIncRight;
            if (lfoPhaseRight >= 1.0f)
                lfoPhaseRight -= 1.0f;
        }
    }
}

} // namespace tillysynth
