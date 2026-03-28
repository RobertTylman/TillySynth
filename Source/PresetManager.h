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

    void saveUserPreset (const juce::String& name);
    void deleteUserPreset (int index);
    bool isUserPreset (int index) const;
    int getFactoryPresetCount() const { return factoryPresetCount; }

    void refreshUserPresets();

private:
    void buildFactoryPresets();
    void loadUserPresets();
    juce::File getUserPresetDirectory() const;

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Preset> presets;
    int currentPreset = -1;
    int factoryPresetCount = 0;
};

} // namespace tillysynth
