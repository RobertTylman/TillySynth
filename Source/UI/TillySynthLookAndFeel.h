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

    // Structural colours
    juce::Colour panelBg;
    juce::Colour panelBrd;
    juce::Colour sectionBg;
    juce::Colour sectionHdr;
    juce::Colour insetBg;

    // Knob colours
    juce::Colour knobFl;
    juce::Colour knobOl;

    // Text colours
    juce::Colour cream;
    juce::Colour label;
    juce::Colour dim;
};

namespace ThemeData
{
    inline const std::array<ColourTheme, 20> themes {{
        // --- Original themes (dark chassis) ---
        { "Amber",
          juce::Colour (0xFFD4A853), juce::Colour (0xFFE8C06A),
          juce::Colour (0xFF8B6914), juce::Colour (0xFF6B5520),
          juce::Colour (0x1AD4A853),
          juce::Colour (0xFF242424), juce::Colour (0xFF383838),
          juce::Colour (0xFF2E2E2E), juce::Colour (0xFF333333),
          juce::Colour (0xFF1A1A1A),
          juce::Colour (0xFF3E3E3E), juce::Colour (0xFF5A5A5A),
          juce::Colour (0xFFE8DCC8), juce::Colour (0xFFBBB0A0), juce::Colour (0xFF887B6A) },

        { "Ice",
          juce::Colour (0xFF5AAFD4), juce::Colour (0xFF7AC4E8),
          juce::Colour (0xFF14698B), juce::Colour (0xFF20556B),
          juce::Colour (0x1A5AAFD4),
          juce::Colour (0xFF242424), juce::Colour (0xFF383838),
          juce::Colour (0xFF2E2E2E), juce::Colour (0xFF333333),
          juce::Colour (0xFF1A1A1A),
          juce::Colour (0xFF3E3E3E), juce::Colour (0xFF5A5A5A),
          juce::Colour (0xFFE8DCC8), juce::Colour (0xFFBBB0A0), juce::Colour (0xFF887B6A) },

        { "Rose",
          juce::Colour (0xFFD45A8B), juce::Colour (0xFFE87AAC),
          juce::Colour (0xFF8B1446), juce::Colour (0xFF6B2045),
          juce::Colour (0x1AD45A8B),
          juce::Colour (0xFF242424), juce::Colour (0xFF383838),
          juce::Colour (0xFF2E2E2E), juce::Colour (0xFF333333),
          juce::Colour (0xFF1A1A1A),
          juce::Colour (0xFF3E3E3E), juce::Colour (0xFF5A5A5A),
          juce::Colour (0xFFE8DCC8), juce::Colour (0xFFBBB0A0), juce::Colour (0xFF887B6A) },

        { "Forest",
          juce::Colour (0xFF5AD47A), juce::Colour (0xFF7AE89A),
          juce::Colour (0xFF148B3A), juce::Colour (0xFF206B35),
          juce::Colour (0x1A5AD47A),
          juce::Colour (0xFF242424), juce::Colour (0xFF383838),
          juce::Colour (0xFF2E2E2E), juce::Colour (0xFF333333),
          juce::Colour (0xFF1A1A1A),
          juce::Colour (0xFF3E3E3E), juce::Colour (0xFF5A5A5A),
          juce::Colour (0xFFE8DCC8), juce::Colour (0xFFBBB0A0), juce::Colour (0xFF887B6A) },

        { "Violet",
          juce::Colour (0xFF9A5AD4), juce::Colour (0xFFB47AE8),
          juce::Colour (0xFF5A148B), juce::Colour (0xFF4A206B),
          juce::Colour (0x1A9A5AD4),
          juce::Colour (0xFF242424), juce::Colour (0xFF383838),
          juce::Colour (0xFF2E2E2E), juce::Colour (0xFF333333),
          juce::Colour (0xFF1A1A1A),
          juce::Colour (0xFF3E3E3E), juce::Colour (0xFF5A5A5A),
          juce::Colour (0xFFE8DCC8), juce::Colour (0xFFBBB0A0), juce::Colour (0xFF887B6A) },

        // --- New retro themes ---

        // Commodore 64: warm brown chassis, blue accents, tan text
        { "C64",
          juce::Colour (0xFF6C6CFF), juce::Colour (0xFF8888FF),
          juce::Colour (0xFF3B3B9E), juce::Colour (0xFF2E2E6B),
          juce::Colour (0x1A6C6CFF),
          juce::Colour (0xFF3B3222), juce::Colour (0xFF5E4F3A),
          juce::Colour (0xFF4A3F2E), juce::Colour (0xFF544736),
          juce::Colour (0xFF2A2418),
          juce::Colour (0xFF5A4E3A), juce::Colour (0xFF6E6050),
          juce::Colour (0xFFD0C4A8), juce::Colour (0xFFB8A888), juce::Colour (0xFF8A7E68) },

        // VHS / Synthwave: deep navy chassis, hot magenta-pink accents
        { "VHS",
          juce::Colour (0xFFFF2D7B), juce::Colour (0xFFFF5C9A),
          juce::Colour (0xFFA0184D), juce::Colour (0xFF6E1238),
          juce::Colour (0x1AFF2D7B),
          juce::Colour (0xFF0E0E2C), juce::Colour (0xFF2A2A52),
          juce::Colour (0xFF161638), juce::Colour (0xFF1E1E44),
          juce::Colour (0xFF08081E),
          juce::Colour (0xFF28284A), juce::Colour (0xFF3E3E68),
          juce::Colour (0xFFE0D0E8), juce::Colour (0xFFB0A0C0), juce::Colour (0xFF7A6E8A) },

        // Terminal: black-green CRT look, phosphor green accents
        { "Terminal",
          juce::Colour (0xFF33FF33), juce::Colour (0xFF66FF66),
          juce::Colour (0xFF0A8C0A), juce::Colour (0xFF0D5E0D),
          juce::Colour (0x1A33FF33),
          juce::Colour (0xFF0A0A0A), juce::Colour (0xFF1A2A1A),
          juce::Colour (0xFF101810), juce::Colour (0xFF142014),
          juce::Colour (0xFF060A06),
          juce::Colour (0xFF1E2E1E), juce::Colour (0xFF2A3E2A),
          juce::Colour (0xFFC0E8C0), juce::Colour (0xFF88B888), juce::Colour (0xFF508850) },

        // Crimson: deep red-tinted chassis, bright red accents
        { "Crimson",
          juce::Colour (0xFFFF3333), juce::Colour (0xFFFF6666),
          juce::Colour (0xFF8C0A0A), juce::Colour (0xFF5E0D0D),
          juce::Colour (0x1AFF3333),
          juce::Colour (0xFF0A0A0A), juce::Colour (0xFF2A1A1A),
          juce::Colour (0xFF181010), juce::Colour (0xFF201414),
          juce::Colour (0xFF0A0606),
          juce::Colour (0xFF2E1E1E), juce::Colour (0xFF3E2A2A),
          juce::Colour (0xFFE8C0C0), juce::Colour (0xFFB88888), juce::Colour (0xFF885050) },

        // Cobalt: deep blue-tinted chassis, electric blue accents
        { "Cobalt",
          juce::Colour (0xFF3388FF), juce::Colour (0xFF66AAFF),
          juce::Colour (0xFF0A4A8C), juce::Colour (0xFF0D355E),
          juce::Colour (0x1A3388FF),
          juce::Colour (0xFF0A0A10), juce::Colour (0xFF1A1A2A),
          juce::Colour (0xFF101018), juce::Colour (0xFF141420),
          juce::Colour (0xFF06060A),
          juce::Colour (0xFF1E1E2E), juce::Colour (0xFF2A2A3E),
          juce::Colour (0xFFC0C8E8), juce::Colour (0xFF8890B8), juce::Colour (0xFF505888) },

        // Solar: warm orange-tinted chassis, amber-orange accents
        { "Solar",
          juce::Colour (0xFFFF8833), juce::Colour (0xFFFFAA66),
          juce::Colour (0xFF8C4A0A), juce::Colour (0xFF5E350D),
          juce::Colour (0x1AFF8833),
          juce::Colour (0xFF100A0A), juce::Colour (0xFF2A1E1A),
          juce::Colour (0xFF181410), juce::Colour (0xFF201A14),
          juce::Colour (0xFF0A0806),
          juce::Colour (0xFF2E261E), juce::Colour (0xFF3E342A),
          juce::Colour (0xFFE8D8C0), juce::Colour (0xFFB8A488), juce::Colour (0xFF887850) },

        // Ultraviolet: deep purple-tinted chassis, vivid purple accents
        { "Ultraviolet",
          juce::Colour (0xFFAA33FF), juce::Colour (0xFFCC66FF),
          juce::Colour (0xFF5A0A8C), juce::Colour (0xFF400D5E),
          juce::Colour (0x1AAA33FF),
          juce::Colour (0xFF0C0A10), juce::Colour (0xFF201A2A),
          juce::Colour (0xFF141018), juce::Colour (0xFF1A1420),
          juce::Colour (0xFF08060A),
          juce::Colour (0xFF261E2E), juce::Colour (0xFF342A3E),
          juce::Colour (0xFFD8C0E8), juce::Colour (0xFFA088B8), juce::Colour (0xFF705088) },

        // Cyan: teal-tinted chassis, bright cyan accents
        { "Cyan",
          juce::Colour (0xFF33FFDD), juce::Colour (0xFF66FFE8),
          juce::Colour (0xFF0A8C7A), juce::Colour (0xFF0D5E54),
          juce::Colour (0x1A33FFDD),
          juce::Colour (0xFF0A0A0C), juce::Colour (0xFF1A2A28),
          juce::Colour (0xFF101816), juce::Colour (0xFF14201E),
          juce::Colour (0xFF060A09),
          juce::Colour (0xFF1E2E2C), juce::Colour (0xFF2A3E3A),
          juce::Colour (0xFFC0E8E4), juce::Colour (0xFF88B8B0), juce::Colour (0xFF508878) },

        // Magenta: deep pink-tinted chassis, hot pink accents
        { "Magenta",
          juce::Colour (0xFFFF33AA), juce::Colour (0xFFFF66CC),
          juce::Colour (0xFF8C0A5A), juce::Colour (0xFF5E0D40),
          juce::Colour (0x1AFF33AA),
          juce::Colour (0xFF100A0C), juce::Colour (0xFF2A1A24),
          juce::Colour (0xFF181018), juce::Colour (0xFF201420),
          juce::Colour (0xFF0A060A),
          juce::Colour (0xFF2E1E2A), juce::Colour (0xFF3E2A38),
          juce::Colour (0xFFE8C0DC), juce::Colour (0xFFB888A8), juce::Colour (0xFF885070) },

        // Gold: rich warm gold-tinted chassis, bright gold accents
        { "Gold",
          juce::Colour (0xFFFFCC33), juce::Colour (0xFFFFDD66),
          juce::Colour (0xFF8C700A), juce::Colour (0xFF5E4E0D),
          juce::Colour (0x1AFFCC33),
          juce::Colour (0xFF100E0A), juce::Colour (0xFF2A261A),
          juce::Colour (0xFF181610), juce::Colour (0xFF201C14),
          juce::Colour (0xFF0A0906),
          juce::Colour (0xFF2E2A1E), juce::Colour (0xFF3E382A),
          juce::Colour (0xFFE8E0C0), juce::Colour (0xFFB8B088), juce::Colour (0xFF888050) },

        // Slate: cool grey-blue chassis, silver-white accents
        { "Slate",
          juce::Colour (0xFFAABBCC), juce::Colour (0xFFCCDDEE),
          juce::Colour (0xFF506070), juce::Colour (0xFF3A4A58),
          juce::Colour (0x1AAABBCC),
          juce::Colour (0xFF101214), juce::Colour (0xFF222830),
          juce::Colour (0xFF181C20), juce::Colour (0xFF1E2228),
          juce::Colour (0xFF0A0C0E),
          juce::Colour (0xFF262C32), juce::Colour (0xFF343C44),
          juce::Colour (0xFFD8DEE4), juce::Colour (0xFFA0A8B0), juce::Colour (0xFF687078) },

        // Lime: yellow-green tinted chassis, electric lime accents
        { "Lime",
          juce::Colour (0xFFAAFF33), juce::Colour (0xFFCCFF66),
          juce::Colour (0xFF5A8C0A), juce::Colour (0xFF405E0D),
          juce::Colour (0x1AAAFF33),
          juce::Colour (0xFF0C0C0A), juce::Colour (0xFF222A1A),
          juce::Colour (0xFF141810), juce::Colour (0xFF1A2014),
          juce::Colour (0xFF080A06),
          juce::Colour (0xFF242E1E), juce::Colour (0xFF323E2A),
          juce::Colour (0xFFDCE8C0), juce::Colour (0xFFA4B888), juce::Colour (0xFF6C8850) },

        // Coral: warm salmon-tinted chassis, coral-peach accents
        { "Coral",
          juce::Colour (0xFFFF7755), juce::Colour (0xFFFF9977),
          juce::Colour (0xFF8C3A1E), juce::Colour (0xFF5E2A16),
          juce::Colour (0x1AFF7755),
          juce::Colour (0xFF100C0A), juce::Colour (0xFF2A201A),
          juce::Colour (0xFF181410), juce::Colour (0xFF201A14),
          juce::Colour (0xFF0A0806),
          juce::Colour (0xFF2E241E), juce::Colour (0xFF3E322A),
          juce::Colour (0xFFE8D4C0), juce::Colour (0xFFB89C88), juce::Colour (0xFF886850) },

        // Midnight: deep indigo chassis, icy white-blue accents
        { "Midnight",
          juce::Colour (0xFF7799FF), juce::Colour (0xFF99BBFF),
          juce::Colour (0xFF2244AA), juce::Colour (0xFF1A3380),
          juce::Colour (0x1A7799FF),
          juce::Colour (0xFF08080E), juce::Colour (0xFF181828),
          juce::Colour (0xFF0E0E1A), juce::Colour (0xFF121220),
          juce::Colour (0xFF050508),
          juce::Colour (0xFF1C1C30), juce::Colour (0xFF2A2A42),
          juce::Colour (0xFFD0D4E8), juce::Colour (0xFF9498B8), juce::Colour (0xFF606488) },

        // Copper: warm brown-orange chassis, burnished copper accents
        { "Copper",
          juce::Colour (0xFFCC7744), juce::Colour (0xFFDD9966),
          juce::Colour (0xFF7A3A14), juce::Colour (0xFF542A10),
          juce::Colour (0x1ACC7744),
          juce::Colour (0xFF100C08), juce::Colour (0xFF28201A),
          juce::Colour (0xFF1A1610), juce::Colour (0xFF201C14),
          juce::Colour (0xFF0A0806),
          juce::Colour (0xFF2C241C), juce::Colour (0xFF3C3228),
          juce::Colour (0xFFE0D0C0), juce::Colour (0xFFB09880), juce::Colour (0xFF806850) },
    }};

    inline int currentThemeIndex = 7;

    inline const ColourTheme& current() { return themes[static_cast<size_t> (currentThemeIndex)]; }
}

namespace Colours
{
    // Structural colours (theme-dependent)
    inline juce::Colour panelBackground()   { return ThemeData::current().panelBg; }
    inline juce::Colour panelBorder()       { return ThemeData::current().panelBrd; }
    inline juce::Colour sectionBackground() { return ThemeData::current().sectionBg; }
    inline juce::Colour sectionHeader()     { return ThemeData::current().sectionHdr; }
    inline juce::Colour insetBackground()   { return ThemeData::current().insetBg; }

    // Knob colours (theme-dependent)
    inline juce::Colour knobFill()          { return ThemeData::current().knobFl; }
    inline juce::Colour knobOutline()       { return ThemeData::current().knobOl; }

    // Fixed knob colours
    inline const juce::Colour knobShadow        { 0x40000000 };
    inline const juce::Colour knobHighlight      { 0x0FFFFFFF };

    // Text hierarchy (theme-dependent)
    inline juce::Colour mutedCream()        { return ThemeData::current().cream; }
    inline juce::Colour labelText()         { return ThemeData::current().label; }
    inline juce::Colour dimText()           { return ThemeData::current().dim; }

    // VU meter (fixed)
    inline const juce::Colour vuGreen           { 0xFF6AAF50 };
    inline const juce::Colour vuAmber           { 0xFFD4A853 };
    inline const juce::Colour vuRed             { 0xFFC0392B };

    // Buttons
    inline juce::Colour inactiveButton()    { return ThemeData::current().knobFl; }

    // Accent accessors
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
