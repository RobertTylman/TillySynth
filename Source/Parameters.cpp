#include "Parameters.h"

namespace tillysynth
{

static juce::StringArray waveformChoices { "Sine", "Sawtooth", "Square", "Triangle" };
static juce::StringArray filterModeChoices { "Low-pass", "High-pass", "Band-pass", "Notch" };
static juce::StringArray filterSlopeChoices { "12 dB/oct", "24 dB/oct" };
static juce::StringArray chorusModeChoices { "Off", "I", "II", "I+II" };

static void addOscillatorParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                                 const juce::String& prefix, const juce::String& name)
{
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_waveform", 1 }, name + " Waveform", waveformChoices, 1));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "_octave", 1 }, name + " Octave", -2, 2, 0));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "_semitone", 1 }, name + " Semitone", -12, 12, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_fine_tune", 1 }, name + " Fine Tune",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 1.0f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_level", 1 }, name + " Level",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_pulse_width", 1 }, name + " Pulse Width",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { prefix + "_unison_voices", 1 }, name + " Unison Voices", 1, 7, 1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_unison_detune", 1 }, name + " Unison Detune",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_unison_blend", 1 }, name + " Unison Blend",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    // Amp envelope
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_attack", 1 }, name + " Attack",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_decay", 1 }, name + " Decay",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 200.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_sustain", 1 }, name + " Sustain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_release", 1 }, name + " Release",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 300.0f));
}

static void addFilterParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filter_mode", 1 }, "Filter Mode", filterModeChoices, 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filter_slope", 1 }, "Filter Slope", filterSlopeChoices, 1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_cutoff", 1 }, "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.25f), 8000.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_resonance", 1 }, "Filter Resonance",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_env_amount", 1 }, "Filter Env Amount",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_key_tracking", 1 }, "Filter Key Tracking",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_velocity", 1 }, "Filter Velocity",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    // Filter envelope
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_attack", 1 }, "Filter Attack",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 10.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_decay", 1 }, "Filter Decay",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 400.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_sustain", 1 }, "Filter Sustain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 30.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "filter_release", 1 }, "Filter Release",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 500.0f));
}

static void addLFOParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                           const juce::String& prefix, const juce::String& name)
{
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { prefix + "_waveform", 1 }, name + " Waveform", waveformChoices, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_rate", 1 }, name + " Rate",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.4f), 1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_depth", 1 }, name + " Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_dest_cutoff", 1 }, name + " -> Cutoff", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_dest_pitch", 1 }, name + " -> Pitch", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_dest_volume", 1 }, name + " -> Volume", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_dest_pw", 1 }, name + " -> PW", false));
}

static void addChorusParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "chorus_mode", 1 }, "Chorus Mode", chorusModeChoices, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorus_rate", 1 }, "Chorus Rate",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 0.5f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chorus_depth", 1 }, "Chorus Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
}

static void addMasterParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master_volume", 1 }, "Master Volume",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 80.0f));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "master_polyphony", 1 }, "Polyphony", 1, 16, 16));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master_glide", 1 }, "Glide",
        juce::NormalisableRange<float> (0.0f, 1000.0f, 1.0f, 0.4f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "master_pitch_bend", 1 }, "Pitch Bend Range", 1, 24, 2));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "master_mono_legato", 1 }, "Mono Legato", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master_analog_drift", 1 }, "Analogue Drift",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    addOscillatorParams (layout, "osc1", "Osc 1");
    addOscillatorParams (layout, "osc2", "Osc 2");
    addFilterParams (layout);
    addLFOParams (layout, "lfo1", "LFO 1");
    addLFOParams (layout, "lfo2", "LFO 2");
    addChorusParams (layout);
    addMasterParams (layout);

    return layout;
}

} // namespace tillysynth
