#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace tillysynth
{

namespace Colours
{
    // Backgrounds
    const juce::Colour panelBackground   { 0xFF242424 };
    const juce::Colour panelBorder       { 0xFF383838 };
    const juce::Colour sectionBackground { 0xFF2E2E2E };
    const juce::Colour sectionHeader     { 0xFF333333 };
    const juce::Colour insetBackground   { 0xFF1A1A1A };

    // Amber family
    const juce::Colour warmAmber         { 0xFFD4A853 };
    const juce::Colour brightAmber       { 0xFFE8C06A };
    const juce::Colour darkAmber         { 0xFF8B6914 };
    const juce::Colour dimAmber          { 0xFF6B5520 };

    // Text hierarchy
    const juce::Colour mutedCream        { 0xFFE8DCC8 };
    const juce::Colour labelText         { 0xFFBBB0A0 };
    const juce::Colour dimText           { 0xFF887B6A };

    // Knob
    const juce::Colour knobFill          { 0xFF3E3E3E };
    const juce::Colour knobOutline       { 0xFF5A5A5A };
    const juce::Colour knobShadow        { 0x40000000 };
    const juce::Colour knobHighlight     { 0x0FFFFFFF };

    // VU meter
    const juce::Colour vuGreen           { 0xFF6AAF50 };
    const juce::Colour vuAmber           { 0xFFD4A853 };
    const juce::Colour vuRed             { 0xFFC0392B };

    // Buttons
    const juce::Colour activeButton      { 0xFFD4A853 };
    const juce::Colour inactiveButton    { 0xFF4A4A4A };

    // Envelope display
    const juce::Colour envFill           { 0x1AD4A853 };
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

    void setScale (float s) { currentScale = s; }
    float getScale() const { return currentScale; }

private:
    float currentScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TillySynthLookAndFeel)
};

} // namespace tillysynth
