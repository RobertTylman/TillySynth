#include "Parameters.h"

namespace tillysynth
{

static juce::StringArray waveformChoices { "Sine", "Sawtooth", "Square", "Triangle" };
static juce::StringArray filterModeChoices { "Low-pass", "High-pass", "Band-pass", "Notch" };
static juce::StringArray filterSlopeChoices { "12 dB/oct", "24 dB/oct" };
static juce::StringArray filterTargetChoices { "Osc 1", "Osc 2", "Osc 1+2", "Noise", "All" };
static juce::StringArray chorusModeChoices { "Off", "I", "II", "I+II" };
static juce::StringArray noiseTypeChoices { "White", "Pink", "Brown", "Blue", "Digital" };

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
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

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

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "filter_target", 1 }, "Filter Target", filterTargetChoices, 4));

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

static void addModEnvParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                             const juce::String& prefix, const juce::String& name)
{
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_attack", 1 }, name + " Attack",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_decay", 1 }, name + " Decay",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 200.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_sustain", 1 }, name + " Sustain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_release", 1 }, name + " Release",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 300.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { prefix + "_amount", 1 }, name + " Amount",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_dest_cutoff", 1 }, name + " -> Cutoff", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_dest_resonance", 1 }, name + " -> Resonance", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_dest_pitch", 1 }, name + " -> Pitch", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { prefix + "_dest_volume", 1 }, name + " -> Volume", false));
}

static void addNoiseParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "noise_type", 1 }, "Noise Type", noiseTypeChoices, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise_level", 1 }, "Noise Level",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise_attack", 1 }, "Noise Attack",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise_decay", 1 }, "Noise Decay",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 200.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise_sustain", 1 }, "Noise Sustain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise_release", 1 }, "Noise Release",
        juce::NormalisableRange<float> (0.0f, 10000.0f, 1.0f, 0.3f), 300.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise_sh_rate", 1 }, "Noise S&H Rate",
        juce::NormalisableRange<float> (1.0f, 20000.0f, 0.1f, 0.25f), 1000.0f));
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

static void addReverbParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverb_size", 1 }, "Reverb Size",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverb_damping", 1 }, "Reverb Damping",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverb_mix", 1 }, "Reverb Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "reverb_width", 1 }, "Reverb Width",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
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

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "master_unison", 1 }, "Master Unison", 1, 7, 1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master_unison_detune", 1 }, "Master Unison Detune",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sidechain_amount", 1 }, "Sidechain Amount",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
}

static juce::StringArray modMatrixSourceChoices {
    "None", "LFO 1", "LFO 2", "Mod Env 1", "Mod Env 2",
    "Velocity", "Aftertouch", "Mod Wheel"
};

static juce::StringArray modMatrixDestChoices {
    "None", "Filter Cutoff", "Filter Resonance", "Pitch", "Volume",
    "Pulse Width", "Osc 1 Level", "Osc 2 Level", "Noise Level",
    "LFO 1 Rate", "LFO 2 Rate"
};

static void addModMatrixParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for (int i = 0; i < 8; ++i)
    {
        auto slotNum = juce::String (i + 1);
        auto prefix = "modmatrix_" + slotNum;

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { prefix + "_source", 1 },
            "Mod " + slotNum + " Source", modMatrixSourceChoices, 0));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { prefix + "_dest", 1 },
            "Mod " + slotNum + " Dest", modMatrixDestChoices, 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { prefix + "_amount", 1 },
            "Mod " + slotNum + " Amount",
            juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));
    }
}

static void addModRangeParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    // Filter cutoff range: percentage of base cutoff (0–400%, default 75%)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_cutoff", 1 }, "Mod Range Cutoff",
        juce::NormalisableRange<float> (0.0f, 400.0f, 1.0f), 75.0f));

    // Filter resonance range: percentage of full range (0–100%, default 75%)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_resonance", 1 }, "Mod Range Resonance",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 75.0f));

    // Pitch range in semitones (0–48, default 2)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_pitch", 1 }, "Mod Range Pitch",
        juce::NormalisableRange<float> (0.0f, 48.0f, 0.1f), 2.0f));

    // Volume range: percentage (0–200%, default 50%)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_volume", 1 }, "Mod Range Volume",
        juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f), 50.0f));

    // Pulse width range: percentage of PW travel (0–100%, default 45%)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_pw", 1 }, "Mod Range PW",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 45.0f));

    // Osc level ranges: percentage (0–200%, default 100%)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_osc1_level", 1 }, "Mod Range Osc1 Level",
        juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_osc2_level", 1 }, "Mod Range Osc2 Level",
        juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f), 100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_noise_level", 1 }, "Mod Range Noise Level",
        juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f), 100.0f));

    // LFO rate ranges in Hz (0–50, default 10)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_lfo1_rate", 1 }, "Mod Range LFO1 Rate",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 10.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modrange_lfo2_rate", 1 }, "Mod Range LFO2 Rate",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 10.0f));
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    addOscillatorParams (layout, "osc1", "Osc 1");
    addOscillatorParams (layout, "osc2", "Osc 2");
    addNoiseParams (layout);
    addFilterParams (layout);
    addLFOParams (layout, "lfo1", "LFO 1");
    addLFOParams (layout, "lfo2", "LFO 2");
    addModEnvParams (layout, "modenv1", "Mod Env 1");
    addModEnvParams (layout, "modenv2", "Mod Env 2");
    addModMatrixParams (layout);
    addModRangeParams (layout);
    addChorusParams (layout);
    addReverbParams (layout);
    addMasterParams (layout);

    return layout;
}

} // namespace tillysynth
