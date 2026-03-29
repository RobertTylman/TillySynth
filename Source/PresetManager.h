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
    juce::String getPresetKey (int index) const;
    const Preset* getPreset (int index) const;

    void loadPreset (int index);
    void applyPreset (const Preset& preset);
    int getCurrentPreset() const { return currentPreset; }

    juce::StringArray getPresetNames() const;

    void saveUserPreset (const juce::String& name);
    void saveUserPreset (const Preset& preset);
    Preset captureCurrentPreset (const juce::String& name,
                                 const juce::String& category = "User") const;
    void deleteUserPreset (int index);
    bool isUserPreset (int index) const;
    int getFactoryPresetCount() const { return factoryPresetCount; }

    void refreshUserPresets();

private:
    void buildFactoryPresets();
    void loadUserPresets();
    juce::File getUserPresetDirectory() const;
    juce::File getUserPresetFile (const juce::String& name) const;
    static juce::String sanitisePresetFileName (const juce::String& name);
    void writePresetToFile (const Preset& preset, const juce::File& file) const;

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Preset> presets;
    int currentPreset = -1;
    int factoryPresetCount = 0;
};

} // namespace tillysynth
