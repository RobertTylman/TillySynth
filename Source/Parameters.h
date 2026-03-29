#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace tillysynth
{

namespace ParamIDs
{
    // Oscillator 1
    inline constexpr const char* osc1Waveform     = "osc1_waveform";
    inline constexpr const char* osc1Octave        = "osc1_octave";
    inline constexpr const char* osc1Semitone      = "osc1_semitone";
    inline constexpr const char* osc1FineTune      = "osc1_fine_tune";
    inline constexpr const char* osc1Level         = "osc1_level";
    inline constexpr const char* osc1PulseWidth    = "osc1_pulse_width";
    inline constexpr const char* osc1UnisonVoices  = "osc1_unison_voices";
    inline constexpr const char* osc1UnisonDetune  = "osc1_unison_detune";
    inline constexpr const char* osc1UnisonBlend   = "osc1_unison_blend";
    inline constexpr const char* osc1Attack        = "osc1_attack";
    inline constexpr const char* osc1Decay         = "osc1_decay";
    inline constexpr const char* osc1Sustain       = "osc1_sustain";
    inline constexpr const char* osc1Release       = "osc1_release";

    // Oscillator 2
    inline constexpr const char* osc2Waveform     = "osc2_waveform";
    inline constexpr const char* osc2Octave        = "osc2_octave";
    inline constexpr const char* osc2Semitone      = "osc2_semitone";
    inline constexpr const char* osc2FineTune      = "osc2_fine_tune";
    inline constexpr const char* osc2Level         = "osc2_level";
    inline constexpr const char* osc2PulseWidth    = "osc2_pulse_width";
    inline constexpr const char* osc2UnisonVoices  = "osc2_unison_voices";
    inline constexpr const char* osc2UnisonDetune  = "osc2_unison_detune";
    inline constexpr const char* osc2UnisonBlend   = "osc2_unison_blend";
    inline constexpr const char* osc2Attack        = "osc2_attack";
    inline constexpr const char* osc2Decay         = "osc2_decay";
    inline constexpr const char* osc2Sustain       = "osc2_sustain";
    inline constexpr const char* osc2Release       = "osc2_release";

    // Filter
    inline constexpr const char* filterMode        = "filter_mode";
    inline constexpr const char* filterSlope       = "filter_slope";
    inline constexpr const char* filterCutoff      = "filter_cutoff";
    inline constexpr const char* filterResonance   = "filter_resonance";
    inline constexpr const char* filterEnvAmount   = "filter_env_amount";
    inline constexpr const char* filterKeyTracking = "filter_key_tracking";
    inline constexpr const char* filterVelocity    = "filter_velocity";
    inline constexpr const char* filterAttack      = "filter_attack";
    inline constexpr const char* filterDecay       = "filter_decay";
    inline constexpr const char* filterSustain     = "filter_sustain";
    inline constexpr const char* filterRelease     = "filter_release";

    // LFO 1
    inline constexpr const char* lfo1Waveform      = "lfo1_waveform";
    inline constexpr const char* lfo1Rate          = "lfo1_rate";
    inline constexpr const char* lfo1Depth         = "lfo1_depth";
    inline constexpr const char* lfo1DestCutoff    = "lfo1_dest_cutoff";
    inline constexpr const char* lfo1DestPitch     = "lfo1_dest_pitch";
    inline constexpr const char* lfo1DestVolume    = "lfo1_dest_volume";
    inline constexpr const char* lfo1DestPW        = "lfo1_dest_pw";

    // LFO 2
    inline constexpr const char* lfo2Waveform      = "lfo2_waveform";
    inline constexpr const char* lfo2Rate          = "lfo2_rate";
    inline constexpr const char* lfo2Depth         = "lfo2_depth";
    inline constexpr const char* lfo2DestCutoff    = "lfo2_dest_cutoff";
    inline constexpr const char* lfo2DestPitch     = "lfo2_dest_pitch";
    inline constexpr const char* lfo2DestVolume    = "lfo2_dest_volume";
    inline constexpr const char* lfo2DestPW        = "lfo2_dest_pw";

    // Chorus
    inline constexpr const char* chorusMode        = "chorus_mode";
    inline constexpr const char* chorusRate        = "chorus_rate";
    inline constexpr const char* chorusDepth       = "chorus_depth";

    // Reverb
    inline constexpr const char* reverbSize        = "reverb_size";
    inline constexpr const char* reverbDamping     = "reverb_damping";
    inline constexpr const char* reverbMix         = "reverb_mix";
    inline constexpr const char* reverbWidth       = "reverb_width";

    // Noise
    inline constexpr const char* noiseType       = "noise_type";
    inline constexpr const char* noiseLevel      = "noise_level";
    inline constexpr const char* noiseAttack     = "noise_attack";
    inline constexpr const char* noiseDecay      = "noise_decay";
    inline constexpr const char* noiseSustain    = "noise_sustain";
    inline constexpr const char* noiseRelease    = "noise_release";
    inline constexpr const char* noiseSHRate     = "noise_sh_rate";

    // Master
    inline constexpr const char* masterVolume      = "master_volume";
    inline constexpr const char* masterPolyphony   = "master_polyphony";
    inline constexpr const char* masterGlide       = "master_glide";
    inline constexpr const char* masterPitchBend   = "master_pitch_bend";
    inline constexpr const char* masterMonoLegato  = "master_mono_legato";
    inline constexpr const char* masterAnalogDrift  = "master_analog_drift";
    inline constexpr const char* masterUnison       = "master_unison";
    inline constexpr const char* masterUnisonDetune = "master_unison_detune";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace tillysynth
