#include "TillySynthLookAndFeel.h"

namespace tillysynth
{

TillySynthLookAndFeel::TillySynthLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, Colours::warmAmber);
    setColour (juce::Slider::rotarySliderOutlineColourId, Colours::knobOutline);
    setColour (juce::Slider::thumbColourId, Colours::mutedCream);

    setColour (juce::Label::textColourId, Colours::labelText);

    setColour (juce::ComboBox::backgroundColourId, Colours::knobFill);
    setColour (juce::ComboBox::textColourId, Colours::mutedCream);
    setColour (juce::ComboBox::outlineColourId, Colours::knobOutline);
    setColour (juce::ComboBox::arrowColourId, Colours::warmAmber);

    setColour (juce::PopupMenu::backgroundColourId, Colours::panelBackground);
    setColour (juce::PopupMenu::textColourId, Colours::mutedCream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::darkAmber);
    setColour (juce::PopupMenu::highlightedTextColourId, Colours::mutedCream);

    setColour (juce::TextButton::buttonColourId, Colours::inactiveButton);
    setColour (juce::TextButton::buttonOnColourId, Colours::activeButton);
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
    float radius = static_cast<float> (juce::jmin (width, height)) * 0.4f;
    float centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    float centreY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;

    // Knob body
    juce::Path knobBody;
    knobBody.addEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Gradient for 3D look
    juce::ColourGradient bodyGrad (Colours::knobFill.brighter (0.3f),
                                   centreX, centreY - radius * 0.5f,
                                   Colours::knobFill.darker (0.3f),
                                   centreX, centreY + radius, false);
    g.setGradientFill (bodyGrad);
    g.fillPath (knobBody);

    // Knob outline
    g.setColour (Colours::knobOutline);
    g.strokePath (knobBody, juce::PathStrokeType (1.5f));

    // Arc track (background)
    float arcRadius = radius + 4.0f;
    juce::Path arcTrack;
    arcTrack.addCentredArc (centreX, centreY, arcRadius, arcRadius,
                            0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (Colours::knobOutline.withAlpha (0.3f));
    g.strokePath (arcTrack, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    // Arc track (filled)
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path arcFilled;
    arcFilled.addCentredArc (centreX, centreY, arcRadius, arcRadius,
                             0.0f, rotaryStartAngle, angle, true);
    g.setColour (Colours::warmAmber);
    g.strokePath (arcFilled, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    // Pointer line
    float pointerLength = radius * 0.6f;
    float pointerThickness = 2.5f;

    juce::Path pointer;
    pointer.addRectangle (-pointerThickness * 0.5f, -radius + 3.0f,
                          pointerThickness, pointerLength);

    pointer.applyTransform (juce::AffineTransform::rotation (angle)
                               .translated (centreX, centreY));

    g.setColour (Colours::mutedCream);
    g.fillPath (pointer);
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

        // Track background
        g.setColour (Colours::knobOutline.withAlpha (0.3f));
        g.drawLine (trackX, trackTop, trackX, trackBottom, 3.0f);

        // Filled portion
        g.setColour (Colours::warmAmber);
        g.drawLine (trackX, sliderPos, trackX, trackBottom, 3.0f);

        // Thumb
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

        g.setColour (Colours::warmAmber);
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

    juce::Colour bg = button.getToggleState() ? Colours::activeButton : Colours::inactiveButton;

    if (shouldDrawButtonAsDown)
        bg = bg.darker (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        bg = bg.brighter (0.1f);

    g.setColour (bg);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (Colours::knobOutline);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

void TillySynthLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                             bool, bool)
{
    auto colour = button.getToggleState()
        ? findColour (juce::TextButton::textColourOnId)
        : findColour (juce::TextButton::textColourOffId);

    g.setColour (colour);
    g.setFont (juce::FontOptions (12.0f));
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

    g.setColour (Colours::warmAmber);
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
    return juce::Font (juce::FontOptions (13.0f));
}

juce::Font TillySynthLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (11.0f));
}

} // namespace tillysynth
