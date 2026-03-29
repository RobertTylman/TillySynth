#pragma once

#include "PresetManager.h"

namespace tillysynth
{

class PresetVariantGenerator
{
public:
    static Preset createVariant (const Preset& sourcePreset, juce::Random& random,
                                 const juce::String& variantName);

private:
    static float mutateValue (const juce::String& paramId, float currentValue, juce::Random& random);
    static float mutateFloat (float currentValue, float minValue, float maxValue,
                              float amount, juce::Random& random);
    static int mutateInt (int currentValue, int minValue, int maxValue,
                          juce::Random& random, float probability = 0.25f);
    static bool isMostlyStableParameter (const juce::String& paramId);
};

} // namespace tillysynth
