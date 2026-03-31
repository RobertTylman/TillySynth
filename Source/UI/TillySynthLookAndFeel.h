#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

namespace tillysynth
{

struct ColourTheme
{
    juce::String name;

    // Accent family
    juce::Colour accent;
    juce::Colour accentBright;
    juce::Colour accentDark;
    juce::Colour accentDim;

    // Envelope display colour
    juce::Colour envFill;
};

namespace ThemeData
{
    inline const std::array<ColourTheme, 5> themes {{
        { "Amber",
          juce::Colour (0xFFD4A853), juce::Colour (0xFFE8C06A),
          juce::Colour (0xFF8B6914), juce::Colour (0xFF6B5520),
          juce::Colour (0x1AD4A853) },
        { "Ice",
          juce::Colour (0xFF5AAFD4), juce::Colour (0xFF7AC4E8),
          juce::Colour (0xFF14698B), juce::Colour (0xFF20556B),
          juce::Colour (0x1A5AAFD4) },
        { "Rose",
          juce::Colour (0xFFD45A8B), juce::Colour (0xFFE87AAC),
          juce::Colour (0xFF8B1446), juce::Colour (0xFF6B2045),
          juce::Colour (0x1AD45A8B) },
        { "Forest",
          juce::Colour (0xFF5AD47A), juce::Colour (0xFF7AE89A),
          juce::Colour (0xFF148B3A), juce::Colour (0xFF206B35),
          juce::Colour (0x1A5AD47A) },
        { "Violet",
          juce::Colour (0xFF9A5AD4), juce::Colour (0xFFB47AE8),
          juce::Colour (0xFF5A148B), juce::Colour (0xFF4A206B),
          juce::Colour (0x1A9A5AD4) },
    }};

    inline int currentThemeIndex = 0;

    inline const ColourTheme& current() { return themes[static_cast<size_t> (currentThemeIndex)]; }
}

namespace Colours
{
    // Backgrounds (theme-independent)
    inline const juce::Colour panelBackground   { 0xFF242424 };
    inline const juce::Colour panelBorder       { 0xFF383838 };
    inline const juce::Colour sectionBackground { 0xFF2E2E2E };
    inline const juce::Colour sectionHeader     { 0xFF333333 };
    inline const juce::Colour insetBackground   { 0xFF1A1A1A };

    // Text hierarchy (theme-independent)
    inline const juce::Colour mutedCream        { 0xFFE8DCC8 };
    inline const juce::Colour labelText         { 0xFFBBB0A0 };
    inline const juce::Colour dimText           { 0xFF887B6A };

    // Knob (theme-independent)
    inline const juce::Colour knobFill          { 0xFF3E3E3E };
    inline const juce::Colour knobOutline       { 0xFF5A5A5A };
    inline const juce::Colour knobShadow        { 0x40000000 };
    inline const juce::Colour knobHighlight     { 0x0FFFFFFF };

    // VU meter
    inline const juce::Colour vuGreen           { 0xFF6AAF50 };
    inline const juce::Colour vuAmber           { 0xFFD4A853 };
    inline const juce::Colour vuRed             { 0xFFC0392B };

    // Buttons
    inline const juce::Colour inactiveButton    { 0xFF4A4A4A };

    // Theme-dependent accessors
    inline juce::Colour warmAmber()    { return ThemeData::current().accent; }
    inline juce::Colour brightAmber()  { return ThemeData::current().accentBright; }
    inline juce::Colour darkAmber()    { return ThemeData::current().accentDark; }
    inline juce::Colour dimAmber()     { return ThemeData::current().accentDim; }
    inline juce::Colour activeButton() { return ThemeData::current().accent; }
    inline juce::Colour envFill()      { return ThemeData::current().envFill; }
}

class TillySynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TillySynthLookAndFeel();

    void applyThemeColours();

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
