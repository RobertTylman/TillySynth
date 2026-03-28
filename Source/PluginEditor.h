#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/TillySynthLookAndFeel.h"
#include <vector>
#include <memory>
#include <array>

namespace tillysynth
{

class TillySynthEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TillySynthEditor (TillySynthProcessor& processor);
    ~TillySynthEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct KnobWithLabel
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    KnobWithLabel createKnob (const juce::String& paramId, const juce::String& labelText);

    struct ComboWithLabel
    {
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<ComboAttachment> attachment;
    };

    ComboWithLabel createCombo (const juce::String& paramId, const juce::String& labelText,
                                const juce::StringArray& items);

    struct ToggleWithLabel
    {
        std::unique_ptr<juce::TextButton> button;
        std::unique_ptr<ButtonAttachment> attachment;
    };

    ToggleWithLabel createToggle (const juce::String& paramId, const juce::String& labelText);

    void timerCallback() override;
    void drawHeader (juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawDriftScope (juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawSectionBackground (juce::Graphics& g, juce::Rectangle<int> bounds,
                                const juce::String& title);
    void drawVUMeter (juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawPanelWear (juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawLFOWaveform (juce::Graphics& g, juce::Rectangle<int> bounds,
                          int waveformType, float phase, float rate);

    void layoutOscillatorSection (juce::Rectangle<int> area, const juce::String& prefix);
    void layoutFilterSection (juce::Rectangle<int> area);
    void layoutLFOSection (juce::Rectangle<int> area, const juce::String& prefix);
    void layoutChorusSection (juce::Rectangle<int> area);
    void layoutReverbSection (juce::Rectangle<int> area);
    void layoutMasterSection (juce::Rectangle<int> area);

    TillySynthProcessor& processorRef;
    TillySynthLookAndFeel lookAndFeel;

    // Preset selector with nav buttons
    juce::ComboBox presetSelector;
    juce::TextButton presetPrev, presetNext, presetSave, presetRandom;
    void rebuildPresetMenu();

    // Master volume in header
    juce::Slider masterVolumeSlider;
    juce::Label masterVolumeLabel;
    std::unique_ptr<SliderAttachment> masterVolumeAttachment;

    // MIDI keyboard
    juce::MidiKeyboardComponent keyboard;

    // Drift scope label (interactive tooltip)
    juce::Label driftLabel;

    // Drift knob in the drift bar
    juce::Slider driftBarKnob;
    std::unique_ptr<SliderAttachment> driftBarKnobAttachment;

    // Scope buffer snapshot for oscilloscope display
    std::array<float, 512> scopeSnapshot {};

    // Drift noise buffer for idle oscilloscope fluctuation
    std::array<float, 512> driftNoise {};

    // VU meter values with analogue ballistics
    float vuLeft = 0.0f, vuRight = 0.0f;

    // Panel wear seed for per-instance variation
    juce::Random wearRandom;
    std::vector<juce::Point<float>> wearScuffs;

    // Author link in header
    juce::HyperlinkButton authorLink { "Robbie Tylman",
        juce::URL ("https://roberttylman.github.io/portfolio-site/") };

    // Tooltip window for parameter descriptions (instant on hover)
    juce::TooltipWindow tooltipWindow { this, 200 };

    // UI components stored by param ID
    std::map<juce::String, KnobWithLabel> knobs;
    std::map<juce::String, ComboWithLabel> combos;
    std::map<juce::String, ToggleWithLabel> toggles;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TillySynthEditor)
};

} // namespace tillysynth
