#include "PresetVariantGenerator.h"

namespace tillysynth
{

static float randomSignedUnit (juce::Random& random)
{
    return random.nextFloat() * 2.0f - 1.0f;
}

Preset PresetVariantGenerator::createVariant (const Preset& sourcePreset, juce::Random& random,
                                              const juce::String& variantName)
{
    Preset variant = sourcePreset;
    variant.name = variantName;
    variant.category = "Generated";

    for (auto& [paramId, value] : variant.parameters)
        value = mutateValue (paramId, value, random);

    return variant;
}

float PresetVariantGenerator::mutateValue (const juce::String& paramId, float currentValue,
                                           juce::Random& random)
{
    if (isMostlyStableParameter (paramId))
        return currentValue;

    if (paramId.endsWith ("_waveform"))
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), 0, 3, random, 0.18f));

    if (paramId == "filter_mode")
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), 0, 3, random, 0.12f));

    if (paramId == "filter_slope")
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), 0, 1, random, 0.12f));

    if (paramId == "chorus_mode")
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), 0, 3, random, 0.18f));

    if (paramId == "noise_type")
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), 0, 4, random, 0.18f));

    if (paramId.endsWith ("_octave"))
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), -2, 2, random, 0.14f));

    if (paramId.endsWith ("_semitone"))
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), -12, 12, random, 0.16f));

    if (paramId.endsWith ("_unison_voices") || paramId == "master_unison")
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), 1, 7, random, 0.22f));

    if (paramId.endsWith ("_dest_cutoff") || paramId.endsWith ("_dest_pitch")
        || paramId.endsWith ("_dest_volume") || paramId.endsWith ("_dest_pw")
        || paramId == "master_mono_legato")
    {
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), 0, 1, random, 0.16f));
    }

    if (paramId.contains ("attack") || paramId.contains ("decay") || paramId.contains ("release"))
        return mutateFloat (currentValue, 0.0f, 10000.0f, 0.24f, random);

    if (paramId.contains ("sustain") || paramId.contains ("level")
        || paramId.contains ("depth") || paramId.contains ("resonance")
        || paramId.contains ("pulse_width") || paramId.contains ("unison_detune")
        || paramId.contains ("unison_blend") || paramId.contains ("key_tracking")
        || paramId.contains ("velocity") || paramId.contains ("analog_drift")
        || paramId == "reverb_size" || paramId == "reverb_damping"
        || paramId == "reverb_mix" || paramId == "reverb_width")
    {
        return mutateFloat (currentValue, 0.0f, 100.0f, 0.14f, random);
    }

    if (paramId == "filter_cutoff")
        return mutateFloat (currentValue, 20.0f, 20000.0f, 0.22f, random);

    if (paramId == "lfo1_rate" || paramId == "lfo2_rate" || paramId == "chorus_rate")
        return mutateFloat (currentValue, 0.01f, 20.0f, 0.18f, random);

    if (paramId == "noise_sh_rate")
        return mutateFloat (currentValue, 1.0f, 20000.0f, 0.22f, random);

    if (paramId == "master_glide")
        return mutateFloat (currentValue, 0.0f, 1000.0f, 0.20f, random);

    if (paramId == "master_volume")
        return mutateFloat (currentValue, 40.0f, 100.0f, 0.08f, random);

    if (paramId == "master_polyphony")
        return static_cast<float> (mutateInt (static_cast<int> (currentValue), 1, 16, random, 0.10f));

    if (paramId == "master_pitch_bend")
        return currentValue;

    if (paramId.contains ("fine_tune") || paramId == "filter_env_amount")
        return mutateFloat (currentValue, -100.0f, 100.0f, 0.15f, random);

    return currentValue;
}

float PresetVariantGenerator::mutateFloat (float currentValue, float minValue, float maxValue,
                                           float amount, juce::Random& random)
{
    auto range = maxValue - minValue;
    auto delta = range * amount * randomSignedUnit (random);

    if (random.nextFloat() < 0.10f)
        delta += range * amount * 0.8f * randomSignedUnit (random);

    return juce::jlimit (minValue, maxValue, currentValue + delta);
}

int PresetVariantGenerator::mutateInt (int currentValue, int minValue, int maxValue,
                                       juce::Random& random, float probability)
{
    if (random.nextFloat() > probability)
        return juce::jlimit (minValue, maxValue, currentValue);

    auto step = random.nextBool() ? 1 : -1;
    if (random.nextFloat() < 0.15f)
        step *= 2;

    return juce::jlimit (minValue, maxValue, currentValue + step);
}

bool PresetVariantGenerator::isMostlyStableParameter (const juce::String& paramId)
{
    return paramId == "master_pitch_bend";
}

} // namespace tillysynth
