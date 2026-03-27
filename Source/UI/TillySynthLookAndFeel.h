#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace tillysynth
{

namespace Colours
{
    const juce::Colour panelBackground { 0xFF2A2A2A };
    const juce::Colour panelBorder     { 0xFF3A3A3A };
    const juce::Colour warmAmber       { 0xFFD4A853 };
    const juce::Colour darkAmber       { 0xFF8B6914 };
    const juce::Colour mutedCream      { 0xFFE8DCC8 };
    const juce::Colour labelText       { 0xFFCCC0AA };
    const juce::Colour knobFill        { 0xFF444444 };
    const juce::Colour knobOutline     { 0xFF666666 };
    const juce::Colour vuGreen         { 0xFF6AAF50 };
    const juce::Colour vuAmber         { 0xFFD4A853 };
    const juce::Colour vuRed           { 0xFFC0392B };
    const juce::Colour sectionHeader   { 0xFF353535 };
    const juce::Colour activeButton    { 0xFFD4A853 };
    const juce::Colour inactiveButton  { 0xFF555555 };
}

class TillySynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TillySynthLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    juce::Font getComboBoxFont (juce::ComboBox& box) override;
    juce::Font getLabelFont (juce::Label& label) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TillySynthLookAndFeel)
};

} // namespace tillysynth
