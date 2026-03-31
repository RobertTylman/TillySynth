#include "TillySynthLookAndFeel.h"

namespace tillysynth
{

TillySynthLookAndFeel::TillySynthLookAndFeel()
{
    applyThemeColours();
}

void TillySynthLookAndFeel::applyThemeColours()
{
    setColour (juce::Slider::rotarySliderFillColourId, Colours::warmAmber());
    setColour (juce::Slider::rotarySliderOutlineColourId, Colours::knobOutline);
    setColour (juce::Slider::thumbColourId, Colours::mutedCream);

    setColour (juce::Label::textColourId, Colours::labelText);

    setColour (juce::ComboBox::backgroundColourId, Colours::knobFill);
    setColour (juce::ComboBox::textColourId, Colours::mutedCream);
    setColour (juce::ComboBox::outlineColourId, Colours::knobOutline);
    setColour (juce::ComboBox::arrowColourId, Colours::warmAmber());

    setColour (juce::PopupMenu::backgroundColourId, Colours::panelBackground);
    setColour (juce::PopupMenu::textColourId, Colours::mutedCream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::darkAmber());
    setColour (juce::PopupMenu::highlightedTextColourId, Colours::mutedCream);

    setColour (juce::TextButton::buttonColourId, Colours::inactiveButton);
    setColour (juce::TextButton::buttonOnColourId, Colours::activeButton());
    setColour (juce::TextButton::textColourOffId, Colours::labelText);
    setColour (juce::TextButton::textColourOnId, Colours::panelBackground);
}

void TillySynthLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                               int x, int y, int width, int height,
                                               float sliderPos,
                                               float rotaryStartAngle,
                                               float rotaryEndAngle,
                                               juce::Slider&)
{
    float diameter = static_cast<float> (juce::jmin (width, height)) - 2.0f;
    float radius = diameter * 0.35f;
    float centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    float centreY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;

    // Layer 1: Drop shadow
    {
        juce::DropShadow shadow (Colours::knobShadow, 4, { 0, 2 });
        juce::Path shadowPath;
        shadowPath.addEllipse (centreX - radius, centreY - radius,
                               radius * 2.0f, radius * 2.0f);
        shadow.drawForPath (g, shadowPath);
    }

    // Layer 2: Knob body with radial gradient
    {
        juce::Path knobBody;
        knobBody.addEllipse (centreX - radius, centreY - radius,
                             radius * 2.0f, radius * 2.0f);

        juce::ColourGradient bodyGrad (Colours::knobFill.brighter (0.25f),
                                       centreX, centreY,
                                       Colours::knobFill.darker (0.2f),
                                       centreX, centreY + radius, true);
        g.setGradientFill (bodyGrad);
        g.fillPath (knobBody);
    }

    // Layer 3: Bezel ring
    {
        float bezelRadius = radius + 1.0f;
        juce::Path bezelRing;
        bezelRing.addEllipse (centreX - bezelRadius, centreY - bezelRadius,
                              bezelRadius * 2.0f, bezelRadius * 2.0f);

        juce::ColourGradient bezelGrad (Colours::knobOutline.brighter (0.3f),
                                        centreX, centreY - bezelRadius,
                                        Colours::knobOutline.darker (0.3f),
                                        centreX, centreY + bezelRadius, false);
        g.setGradientFill (bezelGrad);
        g.strokePath (bezelRing, juce::PathStrokeType (2.0f));
    }

    // Layer 4: Specular highlight
    {
        float hlW = radius * 1.2f;
        float hlH = radius * 0.5f;
        float hlX = centreX - hlW * 0.5f;
        float hlY = centreY - radius * 0.7f;

        juce::ColourGradient specGrad (juce::Colour (0x10FFFFFF),
                                       centreX, hlY,
                                       juce::Colour (0x00FFFFFF),
                                       centreX, hlY + hlH, false);
        g.setGradientFill (specGrad);
        g.fillEllipse (hlX, hlY, hlW, hlH);
    }

    // Layer 5: Arc track
    float arcRadius = juce::jmin (radius + 4.0f, diameter * 0.5f - 2.5f);
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background arc
    {
        juce::Path arcTrack;
        arcTrack.addCentredArc (centreX, centreY, arcRadius, arcRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (Colours::knobOutline.withAlpha (0.25f));
        g.strokePath (arcTrack, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    // Filled arc
    {
        juce::Path arcFilled;
        arcFilled.addCentredArc (centreX, centreY, arcRadius, arcRadius,
                                 0.0f, rotaryStartAngle, angle, true);
        g.setColour (Colours::warmAmber());
        g.strokePath (arcFilled, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
    }

    // Layer 6: Pointer with glow
    {
        float pointerInner = radius * 0.35f;
        float pointerOuter = radius * 0.9f;

        float sinAngle = std::sin (angle);
        float cosAngle = std::cos (angle);

        float x1 = centreX + sinAngle * pointerInner;
        float y1 = centreY - cosAngle * pointerInner;
        float x2 = centreX + sinAngle * pointerOuter;
        float y2 = centreY - cosAngle * pointerOuter;

        // Glow pass
        g.setColour (Colours::mutedCream.withAlpha (0.2f));
        g.drawLine (x1, y1, x2, y2, 3.5f);

        // Sharp pass
        g.setColour (Colours::mutedCream);
        g.drawLine (x1, y1, x2, y2, 2.0f);

        // Layer 7: Tip dot
        g.fillEllipse (x2 - 1.5f, y2 - 1.5f, 3.0f, 3.0f);
    }
}

void TillySynthLookAndFeel::drawLinearSlider (juce::Graphics& g,
                                               int x, int y, int width, int height,
                                               float sliderPos, float /*minSliderPos*/,
                                               float /*maxSliderPos*/,
                                               juce::Slider::SliderStyle style,
                                               juce::Slider&)
{
    bool isVertical = (style == juce::Slider::LinearVertical
                    || style == juce::Slider::LinearBarVertical);

    if (isVertical)
    {
        float trackX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
        float trackTop = static_cast<float> (y);
        float trackBottom = static_cast<float> (y + height);

        g.setColour (Colours::knobOutline.withAlpha (0.3f));
        g.drawLine (trackX, trackTop, trackX, trackBottom, 3.0f);

        g.setColour (Colours::warmAmber());
        g.drawLine (trackX, sliderPos, trackX, trackBottom, 3.0f);

        g.setColour (Colours::mutedCream);
        g.fillEllipse (trackX - 6.0f, sliderPos - 6.0f, 12.0f, 12.0f);
    }
    else
    {
        float trackY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
        float trackLeft = static_cast<float> (x);
        float trackRight = static_cast<float> (x + width);

        g.setColour (Colours::knobOutline.withAlpha (0.3f));
        g.drawLine (trackLeft, trackY, trackRight, trackY, 3.0f);

        g.setColour (Colours::warmAmber());
        g.drawLine (trackLeft, trackY, sliderPos, trackY, 3.0f);

        g.setColour (Colours::mutedCream);
        g.fillEllipse (sliderPos - 6.0f, trackY - 6.0f, 12.0f, 12.0f);
    }
}

void TillySynthLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour&,
                                                    bool shouldDrawButtonAsHighlighted,
                                                    bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);

    juce::Colour bg = button.getToggleState()
        ? button.findColour (juce::TextButton::buttonOnColourId)
        : button.findColour (juce::TextButton::buttonColourId);

    // Skip drawing for fully transparent buttons (used for title label)
    if (bg.getAlpha() == 0)
        return;

    if (shouldDrawButtonAsDown)
        bg = bg.darker (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        bg = bg.brighter (0.1f);

    g.setColour (bg);
    g.fillRoundedRectangle (bounds, 4.0f);

    // Subtle inner shadow for depth
    if (! button.getToggleState())
    {
        juce::ColourGradient innerShadow (juce::Colour (0x18000000),
                                           bounds.getX(), bounds.getY(),
                                           juce::Colour (0x00000000),
                                           bounds.getX(), bounds.getY() + 4.0f, false);
        g.setGradientFill (innerShadow);
        g.fillRoundedRectangle (bounds, 4.0f);
    }

    g.setColour (Colours::knobOutline);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

void TillySynthLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                             bool, bool)
{
    auto colour = button.getToggleState()
        ? button.findColour (juce::TextButton::textColourOnId)
        : button.findColour (juce::TextButton::textColourOffId);

    g.setColour (colour);

    // Transparent-background buttons use their own font size (for title label)
    bool isTransparent = button.findColour (juce::TextButton::buttonColourId).getAlpha() == 0;
    float fontSize = isTransparent ? 22.0f : 12.0f;
    auto style = isTransparent ? juce::Font::bold : juce::Font::plain;

    g.setFont (juce::Font (juce::FontOptions (fontSize * currentScale, style)));
    g.drawText (button.getButtonText(), button.getLocalBounds(),
                juce::Justification::centred, true);
}

void TillySynthLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                           bool /*isButtonDown*/, int, int, int, int,
                                           juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                           static_cast<float> (width),
                                           static_cast<float> (height));

    g.setColour (Colours::knobFill);
    g.fillRoundedRectangle (bounds.reduced (1.0f), 4.0f);

    g.setColour (Colours::knobOutline);
    g.drawRoundedRectangle (bounds.reduced (1.0f), 4.0f, 1.0f);

    // Arrow
    float arrowX = static_cast<float> (width) - 18.0f;
    float arrowY = static_cast<float> (height) * 0.5f;

    juce::Path arrow;
    arrow.addTriangle (arrowX, arrowY - 3.0f,
                       arrowX + 8.0f, arrowY - 3.0f,
                       arrowX + 4.0f, arrowY + 3.0f);

    g.setColour (Colours::warmAmber());
    g.fillPath (arrow);
}

void TillySynthLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (getLabelFont (label));

    auto textArea = label.getBorderSize().subtractedFrom (label.getLocalBounds());
    g.drawText (label.getText(), textArea, label.getJustificationType(), true);
}

juce::Font TillySynthLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (12.0f * currentScale));
}

juce::Font TillySynthLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (10.0f * currentScale));
}

} // namespace tillysynth
