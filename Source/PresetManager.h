#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace tillysynth
{

struct Preset
{
    juce::String name;
    juce::String category;
    std::vector<std::pair<juce::String, float>> parameters;
};

class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);

    int getNumPresets() const;
    juce::String getPresetName (int index) const;
    juce::String getPresetCategory (int index) const;

    void loadPreset (int index);
    int getCurrentPreset() const { return currentPreset; }

    juce::StringArray getPresetNames() const;

private:
    void buildFactoryPresets();

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Preset> presets;
    int currentPreset = -1;
};

} // namespace tillysynth
