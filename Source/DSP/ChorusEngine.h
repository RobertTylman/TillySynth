#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace tillysynth
{

// The Juno panel exposes four states: Off, I, II, and I+II.
// We preserve that exact shape here.
enum class ChorusMode { Off = 0, I, II, Both };

// ChorusEngine is a behavior-model of the classic Juno chorus topology:
//
// 1) Start from one mono source (historically after the HPF in hardware).
// 2) Feed two short bucket-brigade-style delay paths (left and right).
// 3) Modulate those two delays with one shared LFO, but opposite polarity
//    so channels move in opposite directions (+depth on L, -depth on R).
// 4) Mix dry + wet with the Juno-like wet attenuation.
//
// This is not a transistor-level BBD circuit simulator; instead it targets the
// audible behavior of the hardware (movement, width, softening, slight grit).
class ChorusEngine
{
public:
    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void setMode (ChorusMode mode);
    void setDepth (float mix01);

    void process (juce::AudioBuffer<float>& buffer);

private:
    // DelayLine is a circular buffer with a fractional-delay read.
    //
    // Why this structure?
    // A chorus is fundamentally a variable short delay. In analog BBD chips,
    // delay changes continuously as the clock changes. In digital code, this
    // means the read position is often non-integer (e.g. 162.37 samples), so
    // we need interpolation between stored samples.
    struct DelayLine
    {
        // Contiguous storage for delay samples.
        std::vector<float> buffer;

        // Current write position in the circular buffer.
        int writeIndex = 0;

        // Total allocated length.
        int maxLength = 0;

        void prepare (int length);
        void reset();
        void write (float sample);
        float readCubic (float delaySamples) const;
    };

    // ModeSpec hard-codes the "character contract" for each Juno mode.
    //
    // lfoRateHz:      LFO speed for this mode.
    // min/maxDelaySec: Delay excursion bounds used by that mode.
    // isMonoOut:      I+II is intentionally mono in this model.
    // useSineLfo:     I and II use triangle; I+II uses fast sine-like motion.
    struct ModeSpec
    {
        float lfoRateHz = 0.0f;
        float minDelaySec = 0.0f;
        float maxDelaySec = 0.0f;
        bool isMonoOut = false;
        bool useSineLfo = false;
    };

    ModeSpec getModeSpec() const;
    float getLfoValue (float phase, bool useSine) const;
    float processDelayLine (DelayLine& line, float input, float delaySamples);

    ChorusMode mode = ChorusMode::Off;
    float dryWetMix = 0.5f;
    double currentSampleRate = 44100.0;

    // Two delay paths represent the two BBD lines in a stereo chorus output.
    // The same input is written to both lines; only modulated read position
    // differs (inverted modulation between left/right).
    DelayLine delayLeft;
    DelayLine delayRight;

    // Shared LFO phase for both lines. The right side uses inverted polarity
    // of the same LFO value to achieve 180-degree modulation opposition.
    float lfoPhase = 0.0f;

    // Pre-delay low-pass approximation.
    //
    // Many BBD-style systems filter before/after the delay to control aliasing,
    // clock artifacts, and tone. Here, two cascaded one-pole filters create an
    // approximate 12 dB/oct slope with low cost.
    float preLp1State = 0.0f;
    float preLp2State = 0.0f;
    float preLpCoeff = 0.0f;

    // Post-delay smoothing states per channel. This softens top-end and makes
    // the delayed path feel less "sterile digital."
    float outLpLeft = 0.0f;
    float outLpRight = 0.0f;
    float outLpCoeff = 0.0f;
};

} // namespace tillysynth
