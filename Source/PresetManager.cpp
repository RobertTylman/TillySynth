#include "PresetManager.h"

namespace tillysynth
{

// Helper to build a preset with defaults, then override specific params
static Preset makePreset (const juce::String& name, const juce::String& category,
                           std::initializer_list<std::pair<juce::String, float>> overrides)
{
    // Default parameter values matching the APVTS defaults
    std::vector<std::pair<juce::String, float>> defaults = {
        { "osc1_waveform", 1 }, { "osc1_octave", 0 }, { "osc1_semitone", 0 },
        { "osc1_fine_tune", 0 }, { "osc1_level", 100 }, { "osc1_pulse_width", 50 },
        { "osc1_unison_voices", 1 }, { "osc1_unison_detune", 20 }, { "osc1_unison_blend", 50 },
        { "osc1_attack", 5 }, { "osc1_decay", 200 }, { "osc1_sustain", 70 }, { "osc1_release", 300 },

        { "osc2_waveform", 1 }, { "osc2_octave", 0 }, { "osc2_semitone", 0 },
        { "osc2_fine_tune", 0 }, { "osc2_level", 100 }, { "osc2_pulse_width", 50 },
        { "osc2_unison_voices", 1 }, { "osc2_unison_detune", 20 }, { "osc2_unison_blend", 50 },
        { "osc2_attack", 5 }, { "osc2_decay", 200 }, { "osc2_sustain", 70 }, { "osc2_release", 300 },

        { "filter_mode", 0 }, { "filter_slope", 1 }, { "filter_cutoff", 8000 },
        { "filter_resonance", 20 }, { "filter_env_amount", 50 }, { "filter_key_tracking", 0 },
        { "filter_velocity", 0 },
        { "filter_attack", 10 }, { "filter_decay", 400 }, { "filter_sustain", 30 },
        { "filter_release", 500 },

        { "lfo1_waveform", 0 }, { "lfo1_rate", 1 }, { "lfo1_depth", 0 },
        { "lfo1_dest_cutoff", 0 }, { "lfo1_dest_pitch", 0 },
        { "lfo1_dest_volume", 0 }, { "lfo1_dest_pw", 0 },

        { "lfo2_waveform", 0 }, { "lfo2_rate", 1 }, { "lfo2_depth", 0 },
        { "lfo2_dest_cutoff", 0 }, { "lfo2_dest_pitch", 0 },
        { "lfo2_dest_volume", 0 }, { "lfo2_dest_pw", 0 },

        { "chorus_mode", 0 }, { "chorus_rate", 0.5f }, { "chorus_depth", 50 },

        { "master_volume", 80 }, { "master_polyphony", 16 }, { "master_glide", 0 },
        { "master_pitch_bend", 2 }, { "master_mono_legato", 0 }, { "master_analog_drift", 0 }
    };

    // Apply overrides
    for (auto& ov : overrides)
    {
        for (auto& def : defaults)
        {
            if (def.first == ov.first)
            {
                def.second = ov.second;
                break;
            }
        }
    }

    return { name, category, defaults };
}

void PresetManager::buildFactoryPresets()
{
    // ==================== PADS (1-10) ====================

    presets.push_back (makePreset ("Warm Velvet Pad", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 15 },
        { "osc1_attack", 800 }, { "osc1_decay", 2000 }, { "osc1_sustain", 80 }, { "osc1_release", 1500 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 7 }, { "osc2_level", 70 },
        { "osc2_attack", 1000 }, { "osc2_decay", 2000 }, { "osc2_sustain", 80 }, { "osc2_release", 1500 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 10 }, { "filter_env_amount", 20 },
        { "chorus_mode", 3 }, { "master_analog_drift", 15 }
    }));

    presets.push_back (makePreset ("Crystal Shimmer", "Pads", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 25 },
        { "osc1_attack", 1200 }, { "osc1_sustain", 90 }, { "osc1_release", 3000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 40 },
        { "osc2_attack", 1500 }, { "osc2_sustain", 90 }, { "osc2_release", 3000 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 25 },
        { "lfo1_depth", 15 }, { "lfo1_rate", 0.3f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 20 }
    }));

    presets.push_back (makePreset ("Dark Atmosphere", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 4 }, { "osc1_unison_detune", 30 },
        { "osc1_attack", 2000 }, { "osc1_sustain", 60 }, { "osc1_release", 4000 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_level", 50 }, { "osc2_pulse_width", 30 },
        { "osc2_attack", 2500 }, { "osc2_sustain", 60 }, { "osc2_release", 4000 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 30 }, { "filter_env_amount", 30 },
        { "filter_decay", 3000 }, { "filter_sustain", 20 },
        { "chorus_mode", 3 }, { "master_analog_drift", 30 }
    }));

    presets.push_back (makePreset ("Juno Strings", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 12 },
        { "osc1_attack", 400 }, { "osc1_sustain", 85 }, { "osc1_release", 800 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", -8 }, { "osc2_level", 80 },
        { "osc2_attack", 500 }, { "osc2_sustain", 85 }, { "osc2_release", 800 },
        { "filter_cutoff", 4500 }, { "filter_resonance", 5 }, { "filter_env_amount", 15 },
        { "chorus_mode", 1 }, { "master_analog_drift", 10 }
    }));

    presets.push_back (makePreset ("Floating Clouds", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 20 },
        { "osc1_attack", 3000 }, { "osc1_sustain", 90 }, { "osc1_release", 5000 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 4000 }, { "osc2_sustain", 90 }, { "osc2_release", 5000 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 15 },
        { "lfo1_depth", 20 }, { "lfo1_rate", 0.15f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 25 }
    }));

    presets.push_back (makePreset ("Analog Heaven", "Pads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 40 }, { "osc1_unison_voices", 3 },
        { "osc1_attack", 600 }, { "osc1_sustain", 75 }, { "osc1_release", 2000 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 5 }, { "osc2_level", 60 },
        { "osc2_attack", 800 }, { "osc2_sustain", 75 }, { "osc2_release", 2000 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 20 }, { "filter_env_amount", 35 },
        { "lfo1_depth", 10 }, { "lfo1_rate", 0.5f }, { "lfo1_dest_pw", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 40 }
    }));

    presets.push_back (makePreset ("Choir of Angels", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 1500 }, { "osc1_sustain", 95 }, { "osc1_release", 3000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 50 },
        { "osc2_unison_voices", 5 }, { "osc2_unison_detune", 12 },
        { "osc2_attack", 2000 }, { "osc2_sustain", 95 }, { "osc2_release", 3000 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 8 },
        { "chorus_mode", 2 }, { "master_analog_drift", 15 }
    }));

    presets.push_back (makePreset ("Glacial Drift", "Pads", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 4 }, { "osc1_unison_detune", 35 },
        { "osc1_attack", 4000 }, { "osc1_sustain", 70 }, { "osc1_release", 6000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 25 },
        { "osc2_attack", 5000 }, { "osc2_sustain", 70 }, { "osc2_release", 6000 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 35 }, { "filter_env_amount", 40 },
        { "filter_decay", 5000 }, { "filter_sustain", 10 },
        { "lfo1_depth", 25 }, { "lfo1_rate", 0.08f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 50 }
    }));

    presets.push_back (makePreset ("Warm Blanket", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 8 },
        { "osc1_attack", 500 }, { "osc1_sustain", 90 }, { "osc1_release", 1200 },
        { "osc2_waveform", 3 }, { "osc2_fine_tune", -3 }, { "osc2_level", 90 },
        { "osc2_attack", 600 }, { "osc2_sustain", 90 }, { "osc2_release", 1200 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 5 }, { "filter_env_amount", 10 },
        { "chorus_mode", 1 }, { "master_analog_drift", 12 }
    }));

    presets.push_back (makePreset ("Neon Horizons", "Pads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 35 }, { "osc1_unison_voices", 5 },
        { "osc1_unison_detune", 22 },
        { "osc1_attack", 1000 }, { "osc1_sustain", 80 }, { "osc1_release", 2500 },
        { "osc2_waveform", 1 }, { "osc2_octave", 1 }, { "osc2_level", 45 },
        { "osc2_attack", 1200 }, { "osc2_sustain", 80 }, { "osc2_release", 2500 },
        { "filter_cutoff", 5500 }, { "filter_resonance", 30 }, { "filter_env_amount", 45 },
        { "filter_decay", 2000 },
        { "lfo1_depth", 12 }, { "lfo1_rate", 0.4f }, { "lfo1_dest_pw", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 20 }
    }));

    // ==================== LEADS (11-20) ====================

    presets.push_back (makePreset ("Classic Mono Lead", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_level", 100 },
        { "osc1_attack", 2 }, { "osc1_decay", 300 }, { "osc1_sustain", 60 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_octave", 1 }, { "osc2_level", 50 }, { "osc2_pulse_width", 40 },
        { "osc2_attack", 2 }, { "osc2_decay", 300 }, { "osc2_sustain", 60 }, { "osc2_release", 100 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 40 }, { "filter_env_amount", 70 },
        { "filter_decay", 500 }, { "filter_sustain", 20 },
        { "master_mono_legato", 1 }, { "master_glide", 60 }
    }));

    presets.push_back (makePreset ("Screaming Saw", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 150 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 5 }, { "osc2_level", 90 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 150 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 35 }, { "filter_env_amount", 50 },
        { "filter_decay", 400 },
        { "master_mono_legato", 1 }, { "master_glide", 30 }
    }));

    presets.push_back (makePreset ("Soft Sine Lead", "Leads", {
        { "osc1_waveform", 0 }, { "osc1_attack", 10 }, { "osc1_sustain", 90 }, { "osc1_release", 200 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 10 }, { "osc2_sustain", 90 }, { "osc2_release", 200 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 10 },
        { "lfo1_depth", 8 }, { "lfo1_rate", 5.5f }, { "lfo1_dest_pitch", 1 },
        { "master_mono_legato", 1 }, { "master_glide", 80 }
    }));

    presets.push_back (makePreset ("PWM Lead", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 30 },
        { "osc1_attack", 3 }, { "osc1_sustain", 75 }, { "osc1_release", 120 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 60 }, { "osc2_fine_tune", 8 },
        { "osc2_attack", 3 }, { "osc2_sustain", 75 }, { "osc2_release", 120 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 25 }, { "filter_env_amount", 40 },
        { "lfo1_depth", 30 }, { "lfo1_rate", 1.2f }, { "lfo1_dest_pw", 1 },
        { "chorus_mode", 1 }
    }));

    presets.push_back (makePreset ("Acid Squelch", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_attack", 1 }, { "osc1_sustain", 50 }, { "osc1_release", 80 },
        { "osc2_level", 0 },
        { "filter_cutoff", 500 }, { "filter_resonance", 80 }, { "filter_env_amount", 90 },
        { "filter_slope", 1 }, { "filter_decay", 300 }, { "filter_sustain", 5 },
        { "master_mono_legato", 1 }, { "master_glide", 40 }, { "master_analog_drift", 10 }
    }));

    presets.push_back (makePreset ("Thick Unison Lead", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 18 },
        { "osc1_attack", 2 }, { "osc1_sustain", 85 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_level", 60 }, { "osc2_pulse_width", 45 },
        { "osc2_attack", 2 }, { "osc2_sustain", 85 }, { "osc2_release", 100 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 15 }, { "filter_env_amount", 30 },
        { "chorus_mode", 1 }, { "master_analog_drift", 8 }
    }));

    presets.push_back (makePreset ("Ethereal Whistle", "Leads", {
        { "osc1_waveform", 0 }, { "osc1_octave", 1 },
        { "osc1_attack", 200 }, { "osc1_sustain", 95 }, { "osc1_release", 500 },
        { "osc2_waveform", 3 }, { "osc2_octave", 2 }, { "osc2_level", 20 },
        { "osc2_attack", 300 }, { "osc2_sustain", 95 }, { "osc2_release", 500 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 10 },
        { "lfo1_depth", 6 }, { "lfo1_rate", 4.0f }, { "lfo1_dest_pitch", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 5 }
    }));

    presets.push_back (makePreset ("Detuned Aggression", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 40 },
        { "osc1_attack", 1 }, { "osc1_sustain", 70 }, { "osc1_release", 80 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 70 },
        { "osc2_attack", 1 }, { "osc2_sustain", 70 }, { "osc2_release", 80 },
        { "filter_cutoff", 7000 }, { "filter_resonance", 20 },
        { "filter_env_amount", 60 }, { "filter_decay", 250 },
        { "master_analog_drift", 15 }
    }));

    presets.push_back (makePreset ("Vibrato Solo", "Leads", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 5 }, { "osc1_sustain", 80 }, { "osc1_release", 150 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 40 },
        { "osc2_attack", 5 }, { "osc2_sustain", 80 }, { "osc2_release", 150 },
        { "filter_cutoff", 4500 }, { "filter_resonance", 20 }, { "filter_env_amount", 35 },
        { "lfo1_depth", 15 }, { "lfo1_rate", 5.0f }, { "lfo1_dest_pitch", 1 },
        { "master_mono_legato", 1 }, { "master_glide", 50 }
    }));

    presets.push_back (makePreset ("Retro Poly Lead", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 45 },
        { "osc1_attack", 5 }, { "osc1_sustain", 70 }, { "osc1_release", 200 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 3 }, { "osc2_level", 80 },
        { "osc2_attack", 5 }, { "osc2_sustain", 70 }, { "osc2_release", 200 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 30 }, { "filter_env_amount", 45 },
        { "filter_decay", 500 }, { "filter_sustain", 25 },
        { "chorus_mode", 1 }, { "master_analog_drift", 12 }
    }));

    // ==================== BASS (21-30) ====================

    presets.push_back (makePreset ("Sub Bass", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -2 },
        { "osc1_attack", 2 }, { "osc1_decay", 100 }, { "osc1_sustain", 90 }, { "osc1_release", 50 },
        { "osc2_level", 0 },
        { "filter_cutoff", 500 }, { "filter_resonance", 5 },
        { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("Analog Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 2 }, { "osc1_decay", 300 }, { "osc1_sustain", 60 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_level", 70 }, { "osc2_pulse_width", 40 },
        { "osc2_attack", 2 }, { "osc2_decay", 300 }, { "osc2_sustain", 60 }, { "osc2_release", 80 },
        { "filter_cutoff", 1200 }, { "filter_resonance", 30 }, { "filter_env_amount", 60 },
        { "filter_decay", 400 }, { "filter_sustain", 15 },
        { "master_mono_legato", 1 }, { "master_analog_drift", 8 }
    }));

    presets.push_back (makePreset ("Rubber Bass", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_decay", 200 }, { "osc1_sustain", 50 }, { "osc1_release", 60 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 50 },
        { "osc2_attack", 1 }, { "osc2_decay", 200 }, { "osc2_sustain", 50 }, { "osc2_release", 60 },
        { "filter_cutoff", 800 }, { "filter_resonance", 45 }, { "filter_env_amount", 80 },
        { "filter_decay", 250 }, { "filter_sustain", 10 },
        { "master_mono_legato", 1 }, { "master_glide", 20 }
    }));

    presets.push_back (makePreset ("Acid Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_sustain", 40 }, { "osc1_release", 50 },
        { "osc2_level", 0 },
        { "filter_cutoff", 400 }, { "filter_resonance", 75 }, { "filter_env_amount", 85 },
        { "filter_slope", 1 }, { "filter_decay", 200 }, { "filter_sustain", 5 },
        { "master_mono_legato", 1 }, { "master_glide", 30 }, { "master_analog_drift", 5 }
    }));

    presets.push_back (makePreset ("Warm Pluck Bass", "Bass", {
        { "osc1_waveform", 3 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 0 }, { "osc1_release", 100 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 80 },
        { "osc2_attack", 1 }, { "osc2_decay", 500 }, { "osc2_sustain", 0 }, { "osc2_release", 100 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 15 }, { "filter_env_amount", 50 },
        { "filter_decay", 300 }, { "filter_sustain", 5 }
    }));

    presets.push_back (makePreset ("Wobble Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_level", 60 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 80 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 50 },
        { "lfo1_depth", 60 }, { "lfo1_rate", 3.0f }, { "lfo1_dest_cutoff", 1 },
        { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("808 Style", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -2 },
        { "osc1_attack", 1 }, { "osc1_decay", 800 }, { "osc1_sustain", 0 }, { "osc1_release", 300 },
        { "osc2_level", 0 },
        { "filter_cutoff", 1000 }, { "filter_resonance", 5 }, { "filter_env_amount", 40 },
        { "filter_decay", 600 }, { "filter_sustain", 0 },
        { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("Reese Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 2 }, { "osc1_sustain", 90 }, { "osc1_release", 100 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_fine_tune", 10 }, { "osc2_level", 100 },
        { "osc2_attack", 2 }, { "osc2_sustain", 90 }, { "osc2_release", 100 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 15 },
        { "master_mono_legato", 1 }, { "master_analog_drift", 10 }
    }));

    presets.push_back (makePreset ("Funky Slap", "Bass", {
        { "osc1_waveform", 2 }, { "osc1_octave", -1 }, { "osc1_pulse_width", 30 },
        { "osc1_attack", 1 }, { "osc1_decay", 150 }, { "osc1_sustain", 40 }, { "osc1_release", 60 },
        { "osc2_level", 0 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 35 }, { "filter_env_amount", 70 },
        { "filter_decay", 120 }, { "filter_sustain", 10 }, { "filter_velocity", 60 }
    }));

    presets.push_back (makePreset ("Deep Dub", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -2 },
        { "osc1_attack", 5 }, { "osc1_decay", 600 }, { "osc1_sustain", 70 }, { "osc1_release", 200 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 40 },
        { "osc2_attack", 5 }, { "osc2_decay", 600 }, { "osc2_sustain", 70 }, { "osc2_release", 200 },
        { "filter_cutoff", 600 }, { "filter_resonance", 20 }, { "filter_env_amount", 30 },
        { "filter_decay", 500 },
        { "chorus_mode", 1 }, { "master_mono_legato", 1 }, { "master_analog_drift", 15 }
    }));

    // ==================== KEYS (31-38) ====================

    presets.push_back (makePreset ("Electric Piano", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 2 }, { "osc1_decay", 600 }, { "osc1_sustain", 40 }, { "osc1_release", 200 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 50 },
        { "osc2_attack", 2 }, { "osc2_decay", 400 }, { "osc2_sustain", 20 }, { "osc2_release", 200 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 10 },
        { "filter_env_amount", 30 }, { "filter_decay", 300 },
        { "filter_velocity", 50 }, { "chorus_mode", 1 }
    }));

    presets.push_back (makePreset ("Bright Pluck", "Keys", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 300 }, { "osc1_sustain", 0 }, { "osc1_release", 150 },
        { "osc2_waveform", 3 }, { "osc2_level", 60 },
        { "osc2_attack", 1 }, { "osc2_decay", 250 }, { "osc2_sustain", 0 }, { "osc2_release", 150 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 15 },
        { "filter_env_amount", 60 }, { "filter_decay", 200 }, { "filter_sustain", 0 },
        { "filter_velocity", 40 }
    }));

    presets.push_back (makePreset ("Clav", "Keys", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 25 },
        { "osc1_attack", 1 }, { "osc1_decay", 200 }, { "osc1_sustain", 30 }, { "osc1_release", 80 },
        { "osc2_level", 0 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 25 },
        { "filter_env_amount", 50 }, { "filter_decay", 150 }, { "filter_sustain", 10 },
        { "filter_velocity", 70 }
    }));

    presets.push_back (makePreset ("Vibraphone", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 5 }, { "osc1_decay", 1500 }, { "osc1_sustain", 0 }, { "osc1_release", 800 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 25 },
        { "osc2_attack", 5 }, { "osc2_decay", 800 }, { "osc2_sustain", 0 }, { "osc2_release", 800 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 8 },
        { "lfo1_depth", 8 }, { "lfo1_rate", 5.0f }, { "lfo1_dest_volume", 1 }
    }));

    presets.push_back (makePreset ("Organ", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 10 }, { "osc1_sustain", 100 }, { "osc1_release", 30 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 70 },
        { "osc2_attack", 10 }, { "osc2_sustain", 100 }, { "osc2_release", 30 },
        { "filter_cutoff", 10000 }, { "filter_resonance", 5 },
        { "lfo1_depth", 5 }, { "lfo1_rate", 6.0f }, { "lfo1_dest_volume", 1 },
        { "chorus_mode", 2 }
    }));

    presets.push_back (makePreset ("Bell Tone", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 1 }, { "osc1_decay", 2000 }, { "osc1_sustain", 0 }, { "osc1_release", 1000 },
        { "osc2_waveform", 0 }, { "osc2_semitone", 7 }, { "osc2_level", 60 },
        { "osc2_attack", 1 }, { "osc2_decay", 1500 }, { "osc2_sustain", 0 }, { "osc2_release", 1000 },
        { "filter_cutoff", 12000 }, { "filter_resonance", 5 }
    }));

    presets.push_back (makePreset ("Harpsichord", "Keys", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 15 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 10 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_octave", 1 }, { "osc2_pulse_width", 20 }, { "osc2_level", 50 },
        { "osc2_attack", 1 }, { "osc2_decay", 300 }, { "osc2_sustain", 5 }, { "osc2_release", 100 },
        { "filter_cutoff", 9000 }, { "filter_resonance", 15 },
        { "filter_env_amount", 40 }, { "filter_decay", 200 }, { "filter_sustain", 5 }
    }));

    presets.push_back (makePreset ("Muted Keys", "Keys", {
        { "osc1_waveform", 3 },
        { "osc1_attack", 3 }, { "osc1_decay", 400 }, { "osc1_sustain", 40 }, { "osc1_release", 150 },
        { "osc2_waveform", 0 }, { "osc2_level", 60 },
        { "osc2_attack", 3 }, { "osc2_decay", 400 }, { "osc2_sustain", 40 }, { "osc2_release", 150 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 10 },
        { "filter_env_amount", 30 }, { "filter_decay", 200 },
        { "chorus_mode", 1 }
    }));

    // ==================== FX / TEXTURES (39-45) ====================

    presets.push_back (makePreset ("Sci-Fi Sweep", "FX", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 50 },
        { "osc1_attack", 3000 }, { "osc1_sustain", 50 }, { "osc1_release", 4000 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 20 }, { "osc2_level", 60 },
        { "osc2_attack", 3000 }, { "osc2_sustain", 50 }, { "osc2_release", 4000 },
        { "filter_cutoff", 300 }, { "filter_resonance", 60 }, { "filter_env_amount", 90 },
        { "filter_attack", 3000 }, { "filter_decay", 5000 }, { "filter_sustain", 10 },
        { "master_analog_drift", 40 }
    }));

    presets.push_back (makePreset ("Laser Zap", "FX", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 100 }, { "osc1_sustain", 0 }, { "osc1_release", 50 },
        { "osc2_level", 0 },
        { "filter_cutoff", 15000 }, { "filter_resonance", 70 }, { "filter_env_amount", -90 },
        { "filter_decay", 80 }, { "filter_sustain", 0 },
        { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("Wind Noise", "FX", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 100 },
        { "osc1_attack", 2000 }, { "osc1_sustain", 60 }, { "osc1_release", 3000 },
        { "osc2_waveform", 3 }, { "osc2_unison_voices", 7 }, { "osc2_unison_detune", 80 },
        { "osc2_level", 80 },
        { "osc2_attack", 2500 }, { "osc2_sustain", 60 }, { "osc2_release", 3000 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 20 },
        { "lfo1_depth", 40 }, { "lfo1_rate", 0.2f }, { "lfo1_dest_cutoff", 1 },
        { "lfo2_depth", 15 }, { "lfo2_rate", 0.05f }, { "lfo2_dest_volume", 1 },
        { "master_analog_drift", 60 }
    }));

    presets.push_back (makePreset ("Metallic Ring", "FX", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 10 },
        { "osc1_attack", 1 }, { "osc1_decay", 3000 }, { "osc1_sustain", 0 }, { "osc1_release", 2000 },
        { "osc2_waveform", 2 }, { "osc2_semitone", 5 }, { "osc2_pulse_width", 15 }, { "osc2_level", 80 },
        { "osc2_attack", 1 }, { "osc2_decay", 2500 }, { "osc2_sustain", 0 }, { "osc2_release", 2000 },
        { "filter_cutoff", 10000 }, { "filter_resonance", 40 },
        { "chorus_mode", 3 }
    }));

    presets.push_back (makePreset ("Underwater", "FX", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 3 },
        { "osc1_attack", 500 }, { "osc1_sustain", 80 }, { "osc1_release", 2000 },
        { "osc2_waveform", 0 }, { "osc2_octave", -1 }, { "osc2_level", 60 },
        { "osc2_attack", 600 }, { "osc2_sustain", 80 }, { "osc2_release", 2000 },
        { "filter_cutoff", 800 }, { "filter_resonance", 50 },
        { "lfo1_depth", 50 }, { "lfo1_rate", 0.4f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 35 }
    }));

    presets.push_back (makePreset ("Bit Crusher Tone", "FX", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 5 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 50 },
        { "osc2_waveform", 2 }, { "osc2_semitone", 7 }, { "osc2_pulse_width", 8 }, { "osc2_level", 90 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 50 },
        { "filter_cutoff", 12000 }, { "filter_resonance", 10 },
        { "master_analog_drift", 20 }
    }));

    presets.push_back (makePreset ("Ghost Whisper", "FX", {
        { "osc1_waveform", 0 }, { "osc1_octave", 2 },
        { "osc1_attack", 1000 }, { "osc1_decay", 3000 }, { "osc1_sustain", 20 }, { "osc1_release", 5000 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 1500 }, { "osc2_decay", 3000 }, { "osc2_sustain", 20 }, { "osc2_release", 5000 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 30 },
        { "filter_env_amount", 50 }, { "filter_decay", 2000 }, { "filter_sustain", 5 },
        { "lfo1_depth", 10 }, { "lfo1_rate", 0.1f }, { "lfo1_dest_pitch", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 45 }
    }));

    // ==================== INIT / UTILITY (46-50) ====================

    presets.push_back (makePreset ("Init Saw", "Utility", {
        { "osc1_waveform", 1 }, { "osc2_level", 0 }
    }));

    presets.push_back (makePreset ("Init Square", "Utility", {
        { "osc1_waveform", 2 }, { "osc2_level", 0 }
    }));

    presets.push_back (makePreset ("Init Dual Osc", "Utility", {
        { "osc1_waveform", 1 }, { "osc2_waveform", 2 }, { "osc2_fine_tune", 5 }
    }));

    presets.push_back (makePreset ("Full Drift Demo", "Utility", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 },
        { "osc1_attack", 200 }, { "osc1_sustain", 80 }, { "osc1_release", 500 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 6 }, { "osc2_level", 80 },
        { "osc2_attack", 200 }, { "osc2_sustain", 80 }, { "osc2_release", 500 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 20 },
        { "chorus_mode", 3 }, { "master_analog_drift", 100 }
    }));

    presets.push_back (makePreset ("Chorus Showcase", "Utility", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 100 }, { "osc1_sustain", 85 }, { "osc1_release", 400 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 40 }, { "osc2_level", 60 },
        { "osc2_attack", 100 }, { "osc2_sustain", 85 }, { "osc2_release", 400 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 15 },
        { "chorus_mode", 3 }, { "chorus_depth", 80 }, { "chorus_rate", 1.0f },
        { "master_analog_drift", 15 }
    }));
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& a)
    : apvts (a)
{
    buildFactoryPresets();
}

int PresetManager::getNumPresets() const
{
    return static_cast<int> (presets.size());
}

juce::String PresetManager::getPresetName (int index) const
{
    if (index < 0 || index >= getNumPresets())
        return {};

    return presets[static_cast<size_t> (index)].name;
}

juce::String PresetManager::getPresetCategory (int index) const
{
    if (index < 0 || index >= getNumPresets())
        return {};

    return presets[static_cast<size_t> (index)].category;
}

void PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= getNumPresets())
        return;

    currentPreset = index;
    const auto& preset = presets[static_cast<size_t> (index)];

    for (const auto& [paramId, value] : preset.parameters)
    {
        if (auto* param = apvts.getRawParameterValue (paramId))
        {
            if (auto* p = apvts.getParameter (paramId))
            {
                float normValue = p->convertTo0to1 (value);
                p->setValueNotifyingHost (normValue);
            }
        }
    }
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : presets)
        names.add (preset.name);
    return names;
}

} // namespace tillysynth
