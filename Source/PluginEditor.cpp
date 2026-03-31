#include "PluginEditor.h"
#include "Parameters.h"
#include <cmath>

namespace tillysynth
{

static constexpr int kWindowWidth  = 1200;
static constexpr int kWindowHeight = 800;
static constexpr int kHeaderHeight = 44;
static constexpr int kScopeBarHeight = 40;
static constexpr int kKeyboardHeight = 64;
static constexpr int kWheelAreaWidth = 52;
static constexpr int kSectionPadding = 8;
static constexpr int kKnobSize = 50;
static constexpr int kSmallKnobSize = 42;
static constexpr int kLabelHeight = 14;
static constexpr int kSectionTitleHeight = 22;
static constexpr int kToggleHeight = 24;
static constexpr int kEnvelopeDisplayH = 60;
static constexpr int kWaveformSelectorH = 36;

namespace
{
int scaleInt (float scale, int value)
{
    return juce::jmax (1, juce::roundToInt (static_cast<float> (value) * scale));
}

float scaleFloat (float scale, float value)
{
    return value * scale;
}
}

// ============================================================
//  WaveformSelector
// ============================================================

WaveformSelector::WaveformSelector (juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& paramId)
    : apvtsRef (apvts), parameterId (paramId)
{
    auto* param = apvtsRef.getParameter (parameterId);
    if (param != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment> (
            *param,
            [this] (float newValue)
            {
                selectedIndex = static_cast<int> (newValue);
                repaint();
            },
            nullptr);
        attachment->sendInitialUpdate();
    }
}

WaveformSelector::~WaveformSelector() = default;

void WaveformSelector::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const bool useGrid = bounds.getWidth() <= bounds.getHeight() * 1.5f;
    const auto gridSize = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto gridBounds = juce::Rectangle<float> (
        bounds.getCentreX() - gridSize * 0.5f,
        bounds.getCentreY() - gridSize * 0.5f,
        gridSize, gridSize);

    for (int i = 0; i < 4; ++i)
    {
        juce::Rectangle<float> cell;
        if (useGrid)
        {
            const float cellSize = gridSize * 0.5f;
            const int row = i / 2;
            const int col = i % 2;
            cell = juce::Rectangle<float> (
                gridBounds.getX() + static_cast<float> (col) * cellSize,
                gridBounds.getY() + static_cast<float> (row) * cellSize,
                cellSize, cellSize);
        }
        else
        {
            const float cellW = bounds.getWidth() / 4.0f;
            cell = juce::Rectangle<float> (
                bounds.getX() + static_cast<float> (i) * cellW,
                bounds.getY(), cellW, bounds.getHeight());
        }

        bool selected = (i == selectedIndex);

        if (selected)
        {
            g.setColour (Colours::darkAmber().withAlpha (0.4f));
            g.fillRoundedRectangle (cell.reduced (1.0f), 3.0f);
        }

        g.setColour (Colours::knobOutline.withAlpha (0.3f));
        g.drawRoundedRectangle (cell.reduced (1.0f), 3.0f, 0.5f);

        drawWaveformIcon (g, cell.reduced (6.0f, 8.0f), i, selected);
    }
}

void WaveformSelector::drawWaveformIcon (juce::Graphics& g, juce::Rectangle<float> bounds,
                                          int waveformIndex, bool isSelected)
{
    juce::Path path;
    float x0 = bounds.getX();
    float y0 = bounds.getY();
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float cy = y0 + h * 0.5f;

    switch (waveformIndex)
    {
        case 0: // Sine
            for (int i = 0; i <= 32; ++i)
            {
                float t = static_cast<float> (i) / 32.0f;
                float px = x0 + t * w;
                float py = cy - std::sin (t * juce::MathConstants<float>::twoPi) * h * 0.4f;
                if (i == 0) path.startNewSubPath (px, py);
                else path.lineTo (px, py);
            }
            break;

        case 1: // Saw
            path.startNewSubPath (x0, cy);
            path.lineTo (x0 + w * 0.5f, y0 + h * 0.1f);
            path.lineTo (x0 + w * 0.5f, y0 + h * 0.9f);
            path.lineTo (x0 + w, cy);
            break;

        case 2: // Square
            path.startNewSubPath (x0, cy);
            path.lineTo (x0, y0 + h * 0.15f);
            path.lineTo (x0 + w * 0.5f, y0 + h * 0.15f);
            path.lineTo (x0 + w * 0.5f, y0 + h * 0.85f);
            path.lineTo (x0 + w, y0 + h * 0.85f);
            path.lineTo (x0 + w, cy);
            break;

        case 3: // Triangle
            path.startNewSubPath (x0, cy);
            path.lineTo (x0 + w * 0.25f, y0 + h * 0.1f);
            path.lineTo (x0 + w * 0.75f, y0 + h * 0.9f);
            path.lineTo (x0 + w, cy);
            break;
    }

    g.setColour (isSelected ? Colours::warmAmber() : Colours::dimAmber());
    g.strokePath (path, juce::PathStrokeType (isSelected ? 2.0f : 1.5f));
}

void WaveformSelector::mouseDown (const juce::MouseEvent& e)
{
    const auto bounds = getLocalBounds().toFloat();
    const bool useGrid = bounds.getWidth() <= bounds.getHeight() * 1.5f;

    int clicked = selectedIndex;
    if (useGrid)
    {
        const float gridSize = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto gridBounds = juce::Rectangle<float> (
            bounds.getCentreX() - gridSize * 0.5f,
            bounds.getCentreY() - gridSize * 0.5f,
            gridSize, gridSize);
        const float cellSize = gridSize * 0.5f;
        const int col = juce::jlimit (0, 1, static_cast<int> ((static_cast<float> (e.x) - gridBounds.getX()) / cellSize));
        const int row = juce::jlimit (0, 1, static_cast<int> ((static_cast<float> (e.y) - gridBounds.getY()) / cellSize));
        clicked = row * 2 + col;
    }
    else
    {
        const float cellW = static_cast<float> (getWidth()) / 4.0f;
        clicked = juce::jlimit (0, 3, static_cast<int> (static_cast<float> (e.x) / cellW));
    }

    if (clicked != selectedIndex && attachment != nullptr)
        attachment->setValueAsCompleteGesture (static_cast<float> (clicked));
}

// ============================================================
//  ADSRDisplay
// ============================================================

juce::Colour ADSRDisplay::getEnvColour() const
{
    return useBrightAccent ? Colours::brightAmber() : Colours::warmAmber();
}

ADSRDisplay::ADSRDisplay (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& attackId, const juce::String& decayId,
                          const juce::String& sustainId, const juce::String& releaseId,
                          bool bright)
    : apvtsRef (apvts), aId (attackId), dId (decayId), sId (sustainId), rId (releaseId),
      useBrightAccent (bright)
{
    startTimerHz (15);
}

ADSRDisplay::~ADSRDisplay()
{
    stopTimer();
}

void ADSRDisplay::timerCallback()
{
    auto load = [&] (const juce::String& id) -> float
    {
        auto* p = apvtsRef.getRawParameterValue (id);
        return p != nullptr ? p->load() : 0.0f;
    };

    attackMs  = load (aId);
    decayMs   = load (dId);
    sustain01 = load (sId) / 100.0f;
    releaseMs = load (rId);
    repaint();
}

float ADSRDisplay::normaliseTime (float ms) const
{
    return std::log1p (juce::jmax (0.0f, ms)) / std::log1p (10000.0f);
}

float ADSRDisplay::denormaliseTime (float norm) const
{
    return std::expm1 (norm * std::log1p (10000.0f));
}

juce::Rectangle<float> ADSRDisplay::getDrawArea() const
{
    auto bounds = getLocalBounds().toFloat();
    const float padX = juce::jmax (6.0f, bounds.getWidth() * 0.02f);
    const float padY = juce::jmax (4.0f, bounds.getHeight() * 0.08f);
    return bounds.reduced (padX, padY);
}

juce::Point<float> ADSRDisplay::getAttackPoint() const
{
    auto area = getDrawArea();
    float aPortion = 0.12f + normaliseTime (attackMs) * 0.20f;
    float dPortion = 0.12f + normaliseTime (decayMs) * 0.18f;
    float rPortion = 0.14f + normaliseTime (releaseMs) * 0.20f;
    float sPortion = juce::jmax (0.12f, 1.0f - (aPortion + dPortion + rPortion));
    float total = aPortion + dPortion + sPortion + rPortion;
    aPortion /= total;

    return { area.getX() + area.getWidth() * aPortion, area.getY() };
}

juce::Point<float> ADSRDisplay::getDecayPoint() const
{
    auto area = getDrawArea();
    float aPortion = 0.12f + normaliseTime (attackMs) * 0.20f;
    float dPortion = 0.12f + normaliseTime (decayMs) * 0.18f;
    float rPortion = 0.14f + normaliseTime (releaseMs) * 0.20f;
    float sPortion = juce::jmax (0.12f, 1.0f - (aPortion + dPortion + rPortion));
    float total = aPortion + dPortion + sPortion + rPortion;
    aPortion /= total;
    dPortion /= total;

    float sustainY = juce::jmap (juce::jlimit (0.0f, 1.0f, sustain01),
                                  area.getBottom(), area.getY());
    return { area.getX() + area.getWidth() * (aPortion + dPortion), sustainY };
}

juce::Point<float> ADSRDisplay::getSustainEnd() const
{
    auto area = getDrawArea();
    float aPortion = 0.12f + normaliseTime (attackMs) * 0.20f;
    float dPortion = 0.12f + normaliseTime (decayMs) * 0.18f;
    float rPortion = 0.14f + normaliseTime (releaseMs) * 0.20f;
    float sPortion = juce::jmax (0.12f, 1.0f - (aPortion + dPortion + rPortion));
    float total = aPortion + dPortion + sPortion + rPortion;
    aPortion /= total;
    dPortion /= total;
    sPortion /= total;

    float sustainY = juce::jmap (juce::jlimit (0.0f, 1.0f, sustain01),
                                  area.getBottom(), area.getY());
    return { area.getX() + area.getWidth() * (aPortion + dPortion + sPortion), sustainY };
}

juce::Point<float> ADSRDisplay::getReleasePoint() const
{
    auto area = getDrawArea();
    return { area.getRight(), area.getBottom() };
}

void ADSRDisplay::paint (juce::Graphics& g)
{
    auto area = getDrawArea();
    auto fullBounds = getLocalBounds().toFloat();
    const float cornerRadius = juce::jmax (4.0f, juce::jmin (fullBounds.getWidth(), fullBounds.getHeight()) * 0.05f);
    const float strokeWidth = juce::jmax (1.5f, area.getHeight() * 0.025f);
    const float stageFontSize = juce::jmax (8.0f, area.getHeight() * 0.14f);
    const float stageLabelHeight = juce::jmax (10.0f, stageFontSize + 2.0f);
    const float handleRadius = juce::jmax (kHandleRadius, area.getHeight() * 0.08f);

    // Background
    g.setColour (Colours::insetBackground);
    g.fillRoundedRectangle (fullBounds, cornerRadius);
    g.setColour (Colours::knobOutline.withAlpha (0.2f));
    g.drawRoundedRectangle (fullBounds, cornerRadius, 0.5f);

    // Baseline
    g.setColour (getEnvColour().withAlpha (0.15f));
    g.drawLine (area.getX(), area.getBottom(), area.getRight(), area.getBottom(), 0.5f);

    auto attackPt = getAttackPoint();
    auto decayPt = getDecayPoint();
    auto sustainEnd = getSustainEnd();
    auto releasePt = getReleasePoint();

    // Envelope path
    juce::Path env;
    env.startNewSubPath (area.getX(), area.getBottom());
    env.lineTo (attackPt);
    env.lineTo (decayPt);
    env.lineTo (sustainEnd);
    env.lineTo (releasePt);

    // Fill under curve
    juce::Path fillPath (env);
    fillPath.lineTo (area.getRight(), area.getBottom());
    fillPath.lineTo (area.getX(), area.getBottom());
    fillPath.closeSubPath();
    g.setColour (getEnvColour().withAlpha (0.1f));
    g.fillPath (fillPath);

    // Stroke
    g.setColour (getEnvColour().withAlpha (0.85f));
    g.strokePath (env, juce::PathStrokeType (strokeWidth));

    // Stage labels
    g.setColour (getEnvColour().withAlpha (0.3f));
    g.setFont (juce::FontOptions (stageFontSize));
    const float labelY = area.getBottom() - stageLabelHeight;
    g.drawText ("A", juce::Rectangle<float> (area.getX(), labelY, attackPt.x - area.getX(), stageLabelHeight),
                juce::Justification::centred);
    g.drawText ("D", juce::Rectangle<float> (attackPt.x, labelY, decayPt.x - attackPt.x, stageLabelHeight),
                juce::Justification::centred);
    g.drawText ("S", juce::Rectangle<float> (decayPt.x, labelY, sustainEnd.x - decayPt.x, stageLabelHeight),
                juce::Justification::centred);
    g.drawText ("R", juce::Rectangle<float> (sustainEnd.x, labelY, releasePt.x - sustainEnd.x, stageLabelHeight),
                juce::Justification::centred);

    // Drag handles
    auto drawHandle = [&] (juce::Point<float> pt, bool isHovered)
    {
        const float r = isHovered ? handleRadius + 1.0f : handleRadius;
        g.setColour (isHovered ? Colours::brightAmber() : getEnvColour());
        g.fillEllipse (pt.x - r, pt.y - r, r * 2.0f, r * 2.0f);
        g.setColour (Colours::mutedCream.withAlpha (0.6f));
        g.drawEllipse (pt.x - r, pt.y - r, r * 2.0f, r * 2.0f, 1.0f);
    };

    drawHandle (attackPt, hoverTarget == DragTarget::Attack);
    drawHandle (decayPt, hoverTarget == DragTarget::DecaySustain);
    drawHandle (sustainEnd, hoverTarget == DragTarget::Release);
}

ADSRDisplay::DragTarget ADSRDisplay::hitTestHandle (juce::Point<float> pos) const
{
    const float threshold = juce::jmax (kHandleRadius + 4.0f,
                                        juce::jmax (kHandleRadius, getDrawArea().getHeight() * 0.08f)
                                            + getDrawArea().getHeight() * 0.06f);

    if (getAttackPoint().getDistanceFrom (pos) < threshold)
        return DragTarget::Attack;
    if (getDecayPoint().getDistanceFrom (pos) < threshold)
        return DragTarget::DecaySustain;
    if (getSustainEnd().getDistanceFrom (pos) < threshold)
        return DragTarget::Release;

    return DragTarget::None;
}

void ADSRDisplay::mouseMove (const juce::MouseEvent& e)
{
    auto newHover = hitTestHandle (e.position);
    if (newHover != hoverTarget)
    {
        hoverTarget = newHover;

        if (hoverTarget == DragTarget::DecaySustain)
            setMouseCursor (juce::MouseCursor::CrosshairCursor);
        else if (hoverTarget != DragTarget::None)
            setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
        else
            setMouseCursor (juce::MouseCursor::NormalCursor);

        repaint();
    }
}

void ADSRDisplay::mouseDown (const juce::MouseEvent& e)
{
    currentDrag = hitTestHandle (e.position);
    if (currentDrag == DragTarget::None)
        return;

    if (currentDrag == DragTarget::Attack)
        apvtsRef.getParameter (aId)->beginChangeGesture();
    else if (currentDrag == DragTarget::DecaySustain)
    {
        apvtsRef.getParameter (dId)->beginChangeGesture();
        apvtsRef.getParameter (sId)->beginChangeGesture();
    }
    else if (currentDrag == DragTarget::Release)
        apvtsRef.getParameter (rId)->beginChangeGesture();
}

void ADSRDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (currentDrag == DragTarget::None)
        return;

    auto area = getDrawArea();
    float normX = (e.position.x - area.getX()) / area.getWidth();
    float normY = 1.0f - (e.position.y - area.getY()) / area.getHeight();
    normX = juce::jlimit (0.01f, 0.99f, normX);
    normY = juce::jlimit (0.0f, 1.0f, normY);

    if (currentDrag == DragTarget::Attack)
    {
        float ms = denormaliseTime (normX * 2.0f);
        auto* param = apvtsRef.getParameter (aId);
        param->setValueNotifyingHost (param->convertTo0to1 (ms));
    }
    else if (currentDrag == DragTarget::DecaySustain)
    {
        float ms = denormaliseTime (normX * 2.0f);
        auto* dParam = apvtsRef.getParameter (dId);
        dParam->setValueNotifyingHost (dParam->convertTo0to1 (ms));

        auto* sParam = apvtsRef.getParameter (sId);
        sParam->setValueNotifyingHost (sParam->convertTo0to1 (normY * 100.0f));
    }
    else if (currentDrag == DragTarget::Release)
    {
        float ms = denormaliseTime (juce::jlimit (0.01f, 1.0f, 1.0f - normX) * 2.0f);
        auto* param = apvtsRef.getParameter (rId);
        param->setValueNotifyingHost (param->convertTo0to1 (ms));
    }
}

void ADSRDisplay::mouseUp (const juce::MouseEvent&)
{
    if (currentDrag == DragTarget::Attack)
        apvtsRef.getParameter (aId)->endChangeGesture();
    else if (currentDrag == DragTarget::DecaySustain)
    {
        apvtsRef.getParameter (dId)->endChangeGesture();
        apvtsRef.getParameter (sId)->endChangeGesture();
    }
    else if (currentDrag == DragTarget::Release)
        apvtsRef.getParameter (rId)->endChangeGesture();

    currentDrag = DragTarget::None;
}

// ============================================================
//  TillySynthEditor
// ============================================================

TillySynthEditor::TillySynthEditor (TillySynthProcessor& p)
    : juce::AudioProcessorEditor (p),
      processorRef (p),
      keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    // Style the keyboard
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, Colours::mutedCream);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, Colours::panelBackground);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, Colours::knobOutline);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Colours::warmAmber().withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Colours::warmAmber().withAlpha (0.2f));
    keyboard.setOctaveForMiddleC (4);
    keyboard.setAvailableRange (0, 127);
    addAndMakeVisible (keyboard);

    // Preset selector with category submenus
    rebuildPresetMenu();
    presetSelector.setTextWhenNothingSelected ("Select Preset...");
    presetSelector.onChange = [this]
    {
        int id = presetSelector.getSelectedId();
        if (id > 0)
            processorRef.getPresetManager().loadPreset (id - 1);
    };
    addAndMakeVisible (presetSelector);

    presetPrev.setButtonText ("<");
    presetPrev.onClick = [this]
    {
        int total = processorRef.getPresetManager().getNumPresets();
        int currentId = presetSelector.getSelectedId();
        int currentIdx = (currentId > 0) ? currentId - 1 : 0;
        int newIdx = (currentIdx > 0) ? currentIdx - 1 : total - 1;
        presetSelector.setSelectedId (newIdx + 1);
    };
    addAndMakeVisible (presetPrev);

    presetNext.setButtonText (">");
    presetNext.onClick = [this]
    {
        int total = processorRef.getPresetManager().getNumPresets();
        int currentId = presetSelector.getSelectedId();
        int currentIdx = (currentId > 0) ? currentId - 1 : -1;
        int newIdx = (currentIdx < total - 1) ? currentIdx + 1 : 0;
        presetSelector.setSelectedId (newIdx + 1);
    };
    addAndMakeVisible (presetNext);

    presetSave.setButtonText ("Save");
    presetSave.setTooltip ("Save current settings as a user preset");
    presetSave.onClick = [this]
    {
        auto* alert = new juce::AlertWindow ("Save Preset", "Enter a name for your preset:",
                                              juce::MessageBoxIconType::NoIcon);
        alert->addTextEditor ("presetName", "", "Preset Name");
        alert->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        auto callback = juce::ModalCallbackFunction::create ([this, alert] (int result)
        {
            if (result == 1)
            {
                auto name = alert->getTextEditorContents ("presetName");
                if (name.isNotEmpty())
                {
                    processorRef.getPresetManager().saveUserPreset (name);
                    rebuildPresetMenu();
                }
            }
        });

        alert->enterModalState (true, callback, true);
    };
    addAndMakeVisible (presetSave);

    presetRandom.setButtonText ("RND");
    presetRandom.setTooltip ("Load a random preset");
    presetRandom.onClick = [this]
    {
        int total = processorRef.getPresetManager().getNumPresets();
        if (total > 0)
        {
            int randomIdx = juce::Random::getSystemRandom().nextInt (total);
            presetSelector.setSelectedId (randomIdx + 1);
        }
    };
    addAndMakeVisible (presetRandom);

    // Author link
    authorLink.setFont (juce::Font (juce::FontOptions (12.0f)), false);
    authorLink.setColour (juce::HyperlinkButton::textColourId,
                          Colours::labelText.withAlpha (0.6f));
    authorLink.setTooltip ("Visit Robbie Tylman's portfolio");
    addAndMakeVisible (authorLink);

    // Clickable title for colour theme cycling
    titleButton.setButtonText ("TillySynth");
    titleButton.setTooltip ("Click to change colour theme");
    titleButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    titleButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0x00000000));
    titleButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0x00000000));
    titleButton.setColour (juce::TextButton::textColourOffId, Colours::warmAmber());
    titleButton.setColour (juce::TextButton::textColourOnId, Colours::warmAmber());
    titleButton.onClick = [this] { cycleColourTheme(); };
    addAndMakeVisible (titleButton);

    // Window size button
    sizeButton.setButtonText ("100%");
    sizeButton.setTooltip ("Cycle window size: 100% / 125% / 150%");
    sizeButton.onClick = [this]
    {
        int current = getWidth() * 100 / kWindowWidth;
        int next = (current < 115) ? 125 : (current < 140) ? 150 : 100;
        setSize (kWindowWidth * next / 100, kWindowHeight * next / 100);
    };
    addAndMakeVisible (sizeButton);

    // Undo/redo buttons
    undoButton.setButtonText ("Undo");
    undoButton.setTooltip ("Undo last parameter change");
    undoButton.onClick = [this] { processorRef.getUndoManager().undo(); };
    addAndMakeVisible (undoButton);

    redoButton.setButtonText ("Redo");
    redoButton.setTooltip ("Redo last undone parameter change");
    redoButton.onClick = [this] { processorRef.getUndoManager().redo(); };
    addAndMakeVisible (redoButton);

    // Transpose buttons
    transposeDown.setButtonText ("-");
    transposeDown.setTooltip ("Transpose down one semitone");
    transposeDown.onClick = [this]
    {
        processorRef.setTranspose (processorRef.getTranspose() - 1);
        int t = processorRef.getTranspose();
        transposeLabel.setText ((t >= 0 ? "+" : "") + juce::String (t), juce::dontSendNotification);
    };
    addAndMakeVisible (transposeDown);

    transposeUp.setButtonText ("+");
    transposeUp.setTooltip ("Transpose up one semitone");
    transposeUp.onClick = [this]
    {
        processorRef.setTranspose (processorRef.getTranspose() + 1);
        int t = processorRef.getTranspose();
        transposeLabel.setText ((t >= 0 ? "+" : "") + juce::String (t), juce::dontSendNotification);
    };
    addAndMakeVisible (transposeUp);

    transposeLabel.setText ("+0", juce::dontSendNotification);
    transposeLabel.setJustificationType (juce::Justification::centred);
    transposeLabel.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    transposeLabel.setColour (juce::Label::textColourId, Colours::mutedCream);
    addAndMakeVisible (transposeLabel);

    transposeTitleLabel.setText ("Transpose", juce::dontSendNotification);
    transposeTitleLabel.setJustificationType (juce::Justification::centred);
    transposeTitleLabel.setFont (juce::Font (juce::FontOptions (8.0f)));
    transposeTitleLabel.setColour (juce::Label::textColourId, Colours::labelText.withAlpha (0.6f));
    addAndMakeVisible (transposeTitleLabel);

    // Master volume slider in header
    masterVolumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterVolumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterVolumeSlider.setTooltip ("Master output volume");
    addAndMakeVisible (masterVolumeSlider);
    masterVolumeAttachment = std::make_unique<SliderAttachment> (
        processorRef.getAPVTS(), "master_volume", masterVolumeSlider);

    masterVolumeLabel.setText ("Volume", juce::dontSendNotification);
    masterVolumeLabel.setJustificationType (juce::Justification::centredRight);
    masterVolumeLabel.setFont (juce::Font (juce::FontOptions (10.0f)));
    addAndMakeVisible (masterVolumeLabel);

    // Drift scope label
    driftLabel.setText ("ANALOGUE DRIFT", juce::dontSendNotification);
    driftLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    driftLabel.setColour (juce::Label::textColourId, Colours::warmAmber().withAlpha (0.7f));
    driftLabel.setInterceptsMouseClicks (true, false);
    addAndMakeVisible (driftLabel);

    driftBarKnob.setSliderStyle (juce::Slider::LinearHorizontal);
    driftBarKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    driftBarKnob.setTooltip ("Analogue drift amount");
    addAndMakeVisible (driftBarKnob);
    driftBarKnobAttachment = std::make_unique<SliderAttachment> (
        processorRef.getAPVTS(), "master_analog_drift", driftBarKnob);

    // Initialise drift noise buffer
    for (auto& n : driftNoise)
        n = 0.0f;

    // --- Oscillator knobs (no waveform combo — using WaveformSelector instead) ---
    auto addOscKnobs = [&] (const juce::String& prefix)
    {
        // Waveform selector component
        waveformSelectors[prefix + "_waveform"] = std::make_unique<WaveformSelector> (
            processorRef.getAPVTS(), prefix + "_waveform");
        addAndMakeVisible (*waveformSelectors[prefix + "_waveform"]);

        knobs[prefix + "_octave"]    = createKnob (prefix + "_octave", "Oct");
        knobs[prefix + "_semitone"]  = createKnob (prefix + "_semitone", "Semi");
        knobs[prefix + "_fine_tune"] = createKnob (prefix + "_fine_tune", "Fine");
        knobs[prefix + "_level"]     = createKnob (prefix + "_level", "Level");
        knobs[prefix + "_pulse_width"] = createKnob (prefix + "_pulse_width", "PW");
        knobs[prefix + "_unison_voices"] = createKnob (prefix + "_unison_voices", "Uni");
        knobs[prefix + "_unison_detune"] = createKnob (prefix + "_unison_detune", "Detune");
        knobs[prefix + "_unison_blend"]  = createKnob (prefix + "_unison_blend", "Blend");
        knobs[prefix + "_attack"]    = createKnob (prefix + "_attack", "A");
        knobs[prefix + "_decay"]     = createKnob (prefix + "_decay", "D");
        knobs[prefix + "_sustain"]   = createKnob (prefix + "_sustain", "S");
        knobs[prefix + "_release"]   = createKnob (prefix + "_release", "R");
    };

    addOscKnobs ("osc1");
    addOscKnobs ("osc2");

    // --- Noise ---
    combos["noise_type"] = createCombo ("noise_type", "Type", { "White", "Pink", "Brown", "Blue", "Digital" });
    knobs["noise_level"]   = createKnob ("noise_level", "Level");
    knobs["noise_sh_rate"] = createKnob ("noise_sh_rate", "S&H");
    knobs["noise_attack"]  = createKnob ("noise_attack", "A");
    knobs["noise_decay"]   = createKnob ("noise_decay", "D");
    knobs["noise_sustain"] = createKnob ("noise_sustain", "S");
    knobs["noise_release"] = createKnob ("noise_release", "R");

    // --- Filter ---
    combos["filter_mode"]   = createCombo ("filter_mode", "Mode", { "LP", "HP", "BP", "Notch" });
    combos["filter_slope"]  = createCombo ("filter_slope", "Slope", { "12dB", "24dB" });
    combos["filter_target"] = createCombo ("filter_target", "Target", { "Osc1", "Osc2", "Both", "Noise", "All" });
    knobs["filter_cutoff"]       = createKnob ("filter_cutoff", "Cutoff");
    knobs["filter_resonance"]    = createKnob ("filter_resonance", "Reso");
    knobs["filter_env_amount"]   = createKnob ("filter_env_amount", "Env");
    knobs["filter_key_tracking"] = createKnob ("filter_key_tracking", "Key");
    knobs["filter_velocity"]     = createKnob ("filter_velocity", "Vel");
    knobs["filter_attack"]       = createKnob ("filter_attack", "A");
    knobs["filter_decay"]        = createKnob ("filter_decay", "D");
    knobs["filter_sustain"]      = createKnob ("filter_sustain", "S");
    knobs["filter_release"]      = createKnob ("filter_release", "R");

    // --- LFOs (waveform selectors instead of combos) ---
    auto addLFOControls = [&] (const juce::String& prefix)
    {
        waveformSelectors[prefix + "_waveform"] = std::make_unique<WaveformSelector> (
            processorRef.getAPVTS(), prefix + "_waveform");
        addAndMakeVisible (*waveformSelectors[prefix + "_waveform"]);

        knobs[prefix + "_rate"]  = createKnob (prefix + "_rate", "Rate");
        knobs[prefix + "_depth"] = createKnob (prefix + "_depth", "Depth");
        toggles[prefix + "_dest_cutoff"] = createToggle (prefix + "_dest_cutoff", "Cut");
        toggles[prefix + "_dest_pitch"]  = createToggle (prefix + "_dest_pitch", "Pit");
        toggles[prefix + "_dest_volume"] = createToggle (prefix + "_dest_volume", "Vol");
        toggles[prefix + "_dest_pw"]     = createToggle (prefix + "_dest_pw", "PW");
    };

    addLFOControls ("lfo1");
    addLFOControls ("lfo2");

    // --- Mod envelopes ---
    auto addModEnvControls = [&] (const juce::String& prefix)
    {
        knobs[prefix + "_amount"]  = createKnob (prefix + "_amount", "Amt");
        knobs[prefix + "_attack"]  = createKnob (prefix + "_attack", "A");
        knobs[prefix + "_decay"]   = createKnob (prefix + "_decay", "D");
        knobs[prefix + "_sustain"] = createKnob (prefix + "_sustain", "S");
        knobs[prefix + "_release"] = createKnob (prefix + "_release", "R");
        toggles[prefix + "_dest_cutoff"]    = createToggle (prefix + "_dest_cutoff", "Cut");
        toggles[prefix + "_dest_resonance"] = createToggle (prefix + "_dest_resonance", "Res");
        toggles[prefix + "_dest_pitch"]     = createToggle (prefix + "_dest_pitch", "Pit");
        toggles[prefix + "_dest_volume"]    = createToggle (prefix + "_dest_volume", "Vol");
    };

    addModEnvControls ("modenv1");
    addModEnvControls ("modenv2");

    // --- Chorus ---
    combos["chorus_mode"] = createCombo ("chorus_mode", "Mode", { "Off", "I", "II", "I+II" });
    knobs["chorus_rate"]  = createKnob ("chorus_rate", "Rate");
    knobs["chorus_depth"] = createKnob ("chorus_depth", "Depth");

    // --- Reverb ---
    knobs["reverb_size"]    = createKnob ("reverb_size", "Size");
    knobs["reverb_damping"] = createKnob ("reverb_damping", "Damp");
    knobs["reverb_mix"]     = createKnob ("reverb_mix", "Mix");
    knobs["reverb_width"]   = createKnob ("reverb_width", "Width");

    // --- Master ---
    knobs["master_volume"]     = createKnob ("master_volume", "Volume");
    knobs["master_polyphony"]  = createKnob ("master_polyphony", "Poly");
    knobs["master_glide"]      = createKnob ("master_glide", "Glide");
    knobs["master_pitch_bend"] = createKnob ("master_pitch_bend", "PB");
    toggles["master_mono_legato"] = createToggle ("master_mono_legato", "Mono");
    knobs["master_analog_drift"]  = createKnob ("master_analog_drift", "Drift");
    knobs["master_analog_drift"].slider->setVisible (false);
    knobs["master_analog_drift"].label->setVisible (false);
    knobs["master_unison"]         = createKnob ("master_unison", "Unison");
    knobs["master_unison_detune"]  = createKnob ("master_unison_detune", "UniDet");
    knobs["sidechain_amount"]      = createKnob ("sidechain_amount", "SC Amt");

    // --- ADSR Displays ---
    osc1AdsrDisplay = std::make_unique<ADSRDisplay> (
        processorRef.getAPVTS(), "osc1_attack", "osc1_decay", "osc1_sustain", "osc1_release");
    addAndMakeVisible (*osc1AdsrDisplay);

    osc2AdsrDisplay = std::make_unique<ADSRDisplay> (
        processorRef.getAPVTS(), "osc2_attack", "osc2_decay", "osc2_sustain", "osc2_release");
    addAndMakeVisible (*osc2AdsrDisplay);

    noiseAdsrDisplay = std::make_unique<ADSRDisplay> (
        processorRef.getAPVTS(), "noise_attack", "noise_decay", "noise_sustain", "noise_release");
    addAndMakeVisible (*noiseAdsrDisplay);

    filterAdsrDisplay = std::make_unique<ADSRDisplay> (
        processorRef.getAPVTS(), "filter_attack", "filter_decay", "filter_sustain", "filter_release");
    addAndMakeVisible (*filterAdsrDisplay);

    modenv1AdsrDisplay = std::make_unique<ADSRDisplay> (
        processorRef.getAPVTS(), "modenv1_attack", "modenv1_decay", "modenv1_sustain", "modenv1_release",
        true);
    addAndMakeVisible (*modenv1AdsrDisplay);

    modenv2AdsrDisplay = std::make_unique<ADSRDisplay> (
        processorRef.getAPVTS(), "modenv2_attack", "modenv2_decay", "modenv2_sustain", "modenv2_release",
        true);
    addAndMakeVisible (*modenv2AdsrDisplay);

    // --- Tooltips ---
    auto setKnobTip = [&] (const juce::String& id, const juce::String& tip)
    {
        if (knobs.count (id)) knobs[id].slider->setTooltip (tip);
    };
    auto setComboTip = [&] (const juce::String& id, const juce::String& tip)
    {
        if (combos.count (id)) combos[id].combo->setTooltip (tip);
    };
    auto setToggleTip = [&] (const juce::String& id, const juce::String& tip)
    {
        if (toggles.count (id)) toggles[id].button->setTooltip (tip);
    };

    auto addOscTips = [&] (const juce::String& prefix)
    {
        setKnobTip (prefix + "_octave", "Transpose by octaves (-2 to +2)");
        setKnobTip (prefix + "_semitone", "Transpose by semitones (-12 to +12)");
        setKnobTip (prefix + "_fine_tune", "Fine pitch adjustment in cents");
        setKnobTip (prefix + "_level", "Oscillator output volume");
        setKnobTip (prefix + "_pulse_width", "Pulse width for square wave");
        setKnobTip (prefix + "_unison_voices", "Number of stacked unison voices");
        setKnobTip (prefix + "_unison_detune", "Pitch spread between unison voices");
        setKnobTip (prefix + "_unison_blend", "Balance between centre and side voices");
        setKnobTip (prefix + "_attack", "Amp envelope attack time (ms)");
        setKnobTip (prefix + "_decay", "Amp envelope decay time (ms)");
        setKnobTip (prefix + "_sustain", "Amp envelope sustain level");
        setKnobTip (prefix + "_release", "Amp envelope release time (ms)");
    };
    addOscTips ("osc1");
    addOscTips ("osc2");

    setComboTip ("noise_type", "Noise colour: White, Pink, Brown, Blue, or Digital (S&H)");
    setKnobTip ("noise_level", "Noise oscillator mix level");
    setKnobTip ("noise_sh_rate", "Sample rate for Digital (S&H) noise type");
    setKnobTip ("noise_attack", "Noise envelope attack time (ms)");
    setKnobTip ("noise_decay", "Noise envelope decay time (ms)");
    setKnobTip ("noise_sustain", "Noise envelope sustain level");
    setKnobTip ("noise_release", "Noise envelope release time (ms)");

    setComboTip ("filter_mode", "Filter type: Low-pass, High-pass, Band-pass, or Notch");
    setComboTip ("filter_slope", "Filter steepness: 12 or 24 dB per octave");
    setComboTip ("filter_target", "Choose which source the filter affects");
    setKnobTip ("filter_cutoff", "Filter cutoff frequency in Hz");
    setKnobTip ("filter_resonance", "Filter resonance / emphasis at cutoff");
    setKnobTip ("filter_env_amount", "How much the filter envelope modulates cutoff");
    setKnobTip ("filter_key_tracking", "Scale cutoff frequency by note pitch");
    setKnobTip ("filter_velocity", "Scale cutoff by note velocity");
    setKnobTip ("filter_attack", "Filter envelope attack time (ms)");
    setKnobTip ("filter_decay", "Filter envelope decay time (ms)");
    setKnobTip ("filter_sustain", "Filter envelope sustain level");
    setKnobTip ("filter_release", "Filter envelope release time (ms)");

    auto addLFOTips = [&] (const juce::String& prefix)
    {
        setKnobTip (prefix + "_rate", "LFO speed in Hz");
        setKnobTip (prefix + "_depth", "LFO modulation intensity");
        setToggleTip (prefix + "_dest_cutoff", "Route LFO to filter cutoff");
        setToggleTip (prefix + "_dest_pitch", "Route LFO to oscillator pitch");
        setToggleTip (prefix + "_dest_volume", "Route LFO to output volume (tremolo)");
        setToggleTip (prefix + "_dest_pw", "Route LFO to pulse width (PWM)");
    };
    addLFOTips ("lfo1");
    addLFOTips ("lfo2");

    auto addModEnvTips = [&] (const juce::String& prefix)
    {
        setKnobTip (prefix + "_amount", "Mod envelope amount");
        setKnobTip (prefix + "_attack", "Mod envelope attack time (ms)");
        setKnobTip (prefix + "_decay", "Mod envelope decay time (ms)");
        setKnobTip (prefix + "_sustain", "Mod envelope sustain level");
        setKnobTip (prefix + "_release", "Mod envelope release time (ms)");
        setToggleTip (prefix + "_dest_cutoff", "Route mod envelope to filter cutoff");
        setToggleTip (prefix + "_dest_resonance", "Route mod envelope to filter resonance");
        setToggleTip (prefix + "_dest_pitch", "Route mod envelope to oscillator pitch");
        setToggleTip (prefix + "_dest_volume", "Route mod envelope to output volume");
    };
    addModEnvTips ("modenv1");
    addModEnvTips ("modenv2");

    setComboTip ("chorus_mode", "Chorus mode: Off, I, II, or I+II (Juno-style)");
    setKnobTip ("chorus_rate", "Chorus LFO speed");
    setKnobTip ("chorus_depth", "Chorus modulation depth");

    setKnobTip ("reverb_size", "Reverb room size (small to large)");
    setKnobTip ("reverb_damping", "High-frequency damping in reverb tail");
    setKnobTip ("reverb_mix", "Dry/wet reverb balance");
    setKnobTip ("reverb_width", "Stereo width of reverb");

    setKnobTip ("master_volume", "Master output volume");
    setKnobTip ("master_polyphony", "Maximum number of simultaneous voices");
    setKnobTip ("master_glide", "Portamento / glide time between notes (ms)");
    setKnobTip ("master_pitch_bend", "Pitch bend range in semitones");
    setToggleTip ("master_mono_legato", "Mono mode with legato note transitions");
    setKnobTip ("master_analog_drift", "Random analogue-style pitch and filter drift");
    setKnobTip ("master_unison", "Global unison voices applied to both oscillators");
    setKnobTip ("master_unison_detune", "Pitch spread for master unison voices");
    setKnobTip ("sidechain_amount", "Sidechain ducking amount — route external audio to the sidechain input");

    // Pitch and mod wheels
    pitchWheel.onValueChange = [this] (float v) { processorRef.setPitchBendFromUI (v); };
    addAndMakeVisible (pitchWheel);

    modWheel.onValueChange = [this] (float v) { processorRef.setModWheelFromUI (v); };
    addAndMakeVisible (modWheel);

    // Configure resize
    setResizable (true, true);
    setResizeLimits (1200, 800, 1800, 1200);
    getConstrainer()->setFixedAspectRatio (static_cast<double> (kWindowWidth)
                                         / static_cast<double> (kWindowHeight));
    setSize (kWindowWidth, kWindowHeight);
    startTimerHz (30);
}

TillySynthEditor::~TillySynthEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TillySynthEditor::rebuildPresetMenu()
{
    presetSelector.clear (juce::dontSendNotification);
    auto& pm = processorRef.getPresetManager();
    auto* rootMenu = presetSelector.getRootMenu();

    juce::StringArray categories;
    for (int i = 0; i < pm.getNumPresets(); ++i)
    {
        auto cat = pm.getPresetCategory (i);
        if (! categories.contains (cat))
            categories.add (cat);
    }

    for (const auto& cat : categories)
    {
        juce::PopupMenu subMenu;
        for (int i = 0; i < pm.getNumPresets(); ++i)
        {
            if (pm.getPresetCategory (i) == cat)
                subMenu.addItem (i + 1, pm.getPresetName (i));
        }
        rootMenu->addSubMenu (cat, subMenu);
    }
}

TillySynthEditor::KnobWithLabel TillySynthEditor::createKnob (const juce::String& paramId,
                                                                const juce::String& labelText)
{
    KnobWithLabel kwl;
    kwl.slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                  juce::Slider::NoTextBox);
    kwl.label = std::make_unique<juce::Label> ("", labelText);
    kwl.label->setJustificationType (juce::Justification::centred);
    kwl.label->setFont (juce::Font (juce::FontOptions (10.0f)));

    addAndMakeVisible (*kwl.slider);
    addAndMakeVisible (*kwl.label);

    kwl.attachment = std::make_unique<SliderAttachment> (processorRef.getAPVTS(),
                                                          paramId, *kwl.slider);
    return kwl;
}

TillySynthEditor::ComboWithLabel TillySynthEditor::createCombo (const juce::String& paramId,
                                                                  const juce::String& labelText,
                                                                  const juce::StringArray& items)
{
    ComboWithLabel cwl;
    cwl.combo = std::make_unique<juce::ComboBox>();
    cwl.label = std::make_unique<juce::Label> ("", labelText);
    cwl.label->setJustificationType (juce::Justification::centred);
    cwl.label->setFont (juce::Font (juce::FontOptions (10.0f)));

    for (int i = 0; i < items.size(); ++i)
        cwl.combo->addItem (items[i], i + 1);

    addAndMakeVisible (*cwl.combo);
    addAndMakeVisible (*cwl.label);

    cwl.attachment = std::make_unique<ComboAttachment> (processorRef.getAPVTS(),
                                                         paramId, *cwl.combo);
    return cwl;
}

TillySynthEditor::ToggleWithLabel TillySynthEditor::createToggle (const juce::String& paramId,
                                                                    const juce::String& labelText)
{
    ToggleWithLabel twl;
    twl.button = std::make_unique<juce::TextButton> (labelText);
    twl.button->setClickingTogglesState (true);

    addAndMakeVisible (*twl.button);

    twl.attachment = std::make_unique<ButtonAttachment> (processorRef.getAPVTS(),
                                                          paramId, *twl.button);
    return twl;
}

// ============================================================
//  Paint
// ============================================================

void TillySynthEditor::paint (juce::Graphics& g)
{
    const float scale = lookAndFeel.getScale();
    const int sectionPadding = scaleInt (scale, kSectionPadding);
    const int halfSectionPadding = juce::jmax (1, sectionPadding / 2);
    const int headerHeight = scaleInt (scale, kHeaderHeight);
    const int scopeBarHeight = scaleInt (scale, kScopeBarHeight);
    const int keyboardHeight = scaleInt (scale, kKeyboardHeight);
    const int sectionTitleHeight = scaleInt (scale, kSectionTitleHeight);
    const int smallKnobSize = scaleInt (scale, kSmallKnobSize);
    const int labelHeight = scaleInt (scale, kLabelHeight);

    g.fillAll (Colours::panelBackground);

    auto bounds = getLocalBounds();
    drawHeader (g, bounds.removeFromTop (headerHeight));
    drawDriftScope (g, bounds.removeFromTop (scopeBarHeight));

    // Content area for section backgrounds
    auto contentArea = getLocalBounds()
        .withTrimmedTop (headerHeight + scopeBarHeight)
        .withTrimmedBottom (keyboardHeight)
        .reduced (sectionPadding);

    int row1H = contentArea.getHeight() * 34 / 100;
    int row2H = contentArea.getHeight() * 31 / 100;

    // Row 1: OSC1 + OSC2 + Noise
    auto row1 = contentArea.removeFromTop (row1H);
    auto noiseArea = row1.removeFromRight (row1.getWidth() * 24 / 100).reduced (halfSectionPadding);
    auto osc1Area = row1.removeFromLeft (row1.getWidth() / 2).reduced (halfSectionPadding);
    auto osc2Area = row1.reduced (halfSectionPadding);
    drawSectionBackground (g, osc1Area, "OSC 1");
    drawSectionBackground (g, osc2Area, "OSC 2");
    drawSectionBackground (g, noiseArea, "NOISE");

    contentArea.removeFromTop (sectionPadding);

    // Row 2: Filter + LFO1 + LFO2
    auto row2 = contentArea.removeFromTop (row2H);
    int filterWidth = row2.getWidth() * 42 / 100;
    auto filterArea = row2.removeFromLeft (filterWidth).reduced (halfSectionPadding);
    auto lfosArea = row2;
    auto lfo1Area = lfosArea.removeFromLeft (lfosArea.getWidth() / 2).reduced (halfSectionPadding);
    auto lfo2Area = lfosArea.reduced (halfSectionPadding);
    drawSectionBackground (g, filterArea, "FILTER");
    drawSectionBackground (g, lfo1Area, "LFO 1");
    drawSectionBackground (g, lfo2Area, "LFO 2");

    // LFO waveform visualisation
    auto& apvts = processorRef.getAPVTS();
    auto lfo1VisArea = lfo1Area.withTrimmedTop (lfo1Area.getHeight() - scaleInt (scale, 44))
                               .reduced (scaleInt (scale, 8), scaleInt (scale, 4));
    auto lfo2VisArea = lfo2Area.withTrimmedTop (lfo2Area.getHeight() - scaleInt (scale, 44))
                               .reduced (scaleInt (scale, 8), scaleInt (scale, 4));
    drawLFOWaveform (g, lfo1VisArea,
                     processorRef.lfo1Waveform.load(),
                     processorRef.lfo1Phase.load(),
                     processorRef.lfo1Rate.load(),
                     apvts.getRawParameterValue ("lfo1_depth")->load() / 100.0f);
    drawLFOWaveform (g, lfo2VisArea,
                     processorRef.lfo2Waveform.load(),
                     processorRef.lfo2Phase.load(),
                     processorRef.lfo2Rate.load(),
                     apvts.getRawParameterValue ("lfo2_depth")->load() / 100.0f);

    contentArea.removeFromTop (sectionPadding);

    // Row 3: ModEnv1 + ModEnv2 + Effects + Master/VU
    auto row3 = contentArea;
    const int vuWidth = scaleInt (scale, 60);
    auto vuArea = row3.removeFromRight (vuWidth).reduced (halfSectionPadding);
    int colW = row3.getWidth() / 4;
    auto modEnv1Area = row3.removeFromLeft (colW).reduced (halfSectionPadding);
    auto modEnv2Area = row3.removeFromLeft (colW).reduced (halfSectionPadding);
    auto effectsArea = row3.removeFromLeft (colW).reduced (halfSectionPadding);
    auto masterArea = row3.reduced (halfSectionPadding);

    drawSectionBackground (g, modEnv1Area, "MOD ENV 1");
    drawSectionBackground (g, modEnv2Area, "MOD ENV 2");
    drawSectionBackground (g, effectsArea, "EFFECTS");
    drawSectionBackground (g, masterArea, "MASTER");
    drawSectionBackground (g, vuArea, "OUT");

    auto effectsContent = effectsArea.withTrimmedTop (sectionTitleHeight);
    auto chorusLabelArea = effectsContent.removeFromTop (scaleInt (scale, 14))
                                          .withTrimmedLeft (scaleInt (scale, 8));
    drawSubSectionLabel (g, chorusLabelArea, "CHORUS");
    effectsContent.removeFromTop (smallKnobSize + labelHeight + scaleInt (scale, 8));

    auto dividerArea = effectsContent.removeFromTop (scaleInt (scale, 12));
    g.setColour (Colours::panelBorder.withAlpha (0.45f));
    g.drawLine (static_cast<float> (dividerArea.getX() + scaleInt (scale, 8)),
                static_cast<float> (dividerArea.getCentreY()),
                static_cast<float> (dividerArea.getRight() - scaleInt (scale, 8)),
                static_cast<float> (dividerArea.getCentreY()),
                0.75f);

    auto reverbLabelArea = effectsContent.removeFromTop (scaleInt (scale, 14))
                                          .withTrimmedLeft (scaleInt (scale, 8));
    drawSubSectionLabel (g, reverbLabelArea, "REVERB");

    drawVUMeter (g, vuArea.reduced (scaleInt (scale, 4), scaleInt (scale, 28)));
}

// ============================================================
//  Resized
// ============================================================

void TillySynthEditor::resized()
{
    const float scale = juce::jmin (static_cast<float> (getWidth()) / static_cast<float> (kWindowWidth),
                                    static_cast<float> (getHeight()) / static_cast<float> (kWindowHeight));
    const int sectionPadding = scaleInt (scale, kSectionPadding);
    const int headerHeight = scaleInt (scale, kHeaderHeight);
    const int scopeBarHeight = scaleInt (scale, kScopeBarHeight);
    const int keyboardHeight = scaleInt (scale, kKeyboardHeight);
    const int wheelAreaWidth = scaleInt (scale, kWheelAreaWidth);
    lookAndFeel.setScale (scale);

    // Title button in header
    titleButton.setBounds (8, 4, 140, 36);

    // Header controls
    int headerCentreX = getWidth() / 2 - 30;
    presetPrev.setBounds (headerCentreX - 155, 10, 28, 24);
    presetSelector.setBounds (headerCentreX - 125, 10, 220, 24);
    presetNext.setBounds (headerCentreX + 97, 10, 28, 24);
    presetSave.setBounds (headerCentreX + 128, 10, 42, 24);
    presetRandom.setBounds (headerCentreX + 174, 10, 38, 24);

    // Transpose
    transposeDown.setBounds (headerCentreX - 255, 7, 22, 22);
    transposeLabel.setBounds (headerCentreX - 235, 7, 30, 22);
    transposeUp.setBounds (headerCentreX - 207, 7, 22, 22);
    transposeTitleLabel.setBounds (headerCentreX - 260, 28, 80, 14);

    // Right side header
    {
        int pct = getWidth() * 100 / kWindowWidth;
        sizeButton.setButtonText (juce::String (pct) + "%");
    }
    undoButton.setBounds (getWidth() - 390, 12, 40, 20);
    redoButton.setBounds (getWidth() - 348, 12, 40, 20);
    masterVolumeLabel.setBounds (getWidth() - 290, 12, 50, 20);
    masterVolumeSlider.setBounds (getWidth() - 240, 12, 100, 20);
    authorLink.setBounds (getWidth() - 130, 10, 80, 24);
    sizeButton.setBounds (getWidth() - 50, 12, 42, 20);

    // Drift bar
    driftLabel.setBounds (scaleInt (scale, 6), headerHeight + scaleInt (scale, 2),
                          scaleInt (scale, 100), scopeBarHeight - scaleInt (scale, 4));
    driftBarKnob.setBounds (scaleInt (scale, 102),
                            headerHeight + (scopeBarHeight - scaleInt (scale, 16)) / 2,
                            scaleInt (scale, 80), scaleInt (scale, 16));

    // Keyboard + wheels
    auto bottomArea = getLocalBounds().removeFromBottom (keyboardHeight + scaleInt (scale, 4));
    bottomArea.removeFromBottom (2);
    auto wheelStrip = bottomArea.removeFromLeft (wheelAreaWidth);
    int wheelW = (wheelStrip.getWidth() - 4) / 2;
    pitchWheel.setBounds (wheelStrip.removeFromLeft (wheelW).reduced (1));
    modWheel.setBounds (wheelStrip.removeFromLeft (wheelW).reduced (1));

    auto isWhiteKey = [] (int midiNote)
    {
        switch (midiNote % 12)
        {
            case 0: case 2: case 4: case 5: case 7: case 9: case 11:
                return true;
            default:
                return false;
        }
    };

    int whiteKeyCount = 0;
    for (int midiNote = 0; midiNote <= 127; ++midiNote)
        if (isWhiteKey (midiNote))
            ++whiteKeyCount;

    if (whiteKeyCount > 0)
        keyboard.setKeyWidth (static_cast<float> (bottomArea.getWidth()) / static_cast<float> (whiteKeyCount));

    keyboard.setBounds (bottomArea);

    // Content area
    auto contentArea = getLocalBounds()
        .withTrimmedTop (headerHeight + scopeBarHeight)
        .withTrimmedBottom (keyboardHeight)
        .reduced (sectionPadding);

    int row1H = contentArea.getHeight() * 34 / 100;
    int row2H = contentArea.getHeight() * 31 / 100;

    // Row 1: OSC1 + OSC2 + Noise
    auto row1 = contentArea.removeFromTop (row1H);
    layoutNoiseSection (row1.removeFromRight (row1.getWidth() * 24 / 100).reduced (sectionPadding));
    layoutOscillatorSection (row1.removeFromLeft (row1.getWidth() / 2).reduced (sectionPadding), "osc1");
    layoutOscillatorSection (row1.reduced (sectionPadding), "osc2");

    contentArea.removeFromTop (sectionPadding);

    // Row 2: Filter + LFO1 + LFO2
    auto row2 = contentArea.removeFromTop (row2H);
    int filterWidth = row2.getWidth() * 42 / 100;
    layoutFilterSection (row2.removeFromLeft (filterWidth).reduced (sectionPadding));
    auto lfosArea = row2;
    layoutLFOSection (lfosArea.removeFromLeft (lfosArea.getWidth() / 2).reduced (sectionPadding), "lfo1");
    layoutLFOSection (lfosArea.reduced (sectionPadding), "lfo2");

    contentArea.removeFromTop (sectionPadding);

    // Row 3: ModEnv1 + ModEnv2 + Effects + Master/VU
    auto row3 = contentArea;
    row3.removeFromRight (scaleInt (scale, 60)); // VU column (paint-only)
    int colW = row3.getWidth() / 4;
    layoutModEnvSection (row3.removeFromLeft (colW).reduced (sectionPadding), "modenv1");
    layoutModEnvSection (row3.removeFromLeft (colW).reduced (sectionPadding), "modenv2");
    layoutEffectsSection (row3.removeFromLeft (colW).reduced (sectionPadding));
    layoutMasterSection (row3.reduced (sectionPadding));
}

// ============================================================
//  Section Layouts
// ============================================================

void TillySynthEditor::layoutOscillatorSection (juce::Rectangle<int> area, const juce::String& prefix)
{
    const float scale = lookAndFeel.getScale();
    const int sectionTitleHeight = scaleInt (scale, kSectionTitleHeight);
    const int labelHeight = scaleInt (scale, kLabelHeight);
    const int knobSize = scaleInt (scale, kKnobSize);
    const int smallKnobSize = scaleInt (scale, kSmallKnobSize);
    const int envelopeDisplayH = scaleInt (scale, kEnvelopeDisplayH);
    area.removeFromTop (sectionTitleHeight);

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row, int w, int knobSz)
    {
        auto col = row.removeFromLeft (w);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (knobSz).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    const int knobH = knobSize + labelHeight;
    auto row1 = area.removeFromTop (knobH);
    const int colW1 = row1.getWidth() / 5;
    placeKnob (prefix + "_octave", row1, colW1, knobSize);
    placeKnob (prefix + "_semitone", row1, colW1, knobSize);
    placeKnob (prefix + "_fine_tune", row1, colW1, knobSize);
    placeKnob (prefix + "_level", row1, colW1, knobSize);
    placeKnob (prefix + "_pulse_width", row1, colW1, knobSize);

    area.removeFromTop (scaleInt (scale, 4));

    const int smallH = smallKnobSize + labelHeight;
    auto row2 = area.removeFromTop (smallH);
    const int colW2 = row2.getWidth() / 7;
    placeKnob (prefix + "_unison_voices", row2, colW2, smallKnobSize);
    placeKnob (prefix + "_unison_detune", row2, colW2, smallKnobSize);
    placeKnob (prefix + "_unison_blend", row2, colW2, smallKnobSize);
    placeKnob (prefix + "_attack", row2, colW2, smallKnobSize);
    placeKnob (prefix + "_decay", row2, colW2, smallKnobSize);
    placeKnob (prefix + "_sustain", row2, colW2, smallKnobSize);
    placeKnob (prefix + "_release", row2, colW2, smallKnobSize);

    area.removeFromTop (scaleInt (scale, 6));

    auto envArea = area.removeFromTop (juce::jmin (envelopeDisplayH, area.getHeight()));
    const int selectorSize = juce::jmin (envArea.getHeight(), scaleInt (scale, 64));
    if (waveformSelectors.count (prefix + "_waveform"))
        waveformSelectors[prefix + "_waveform"]->setBounds (
            envArea.removeFromLeft (selectorSize).reduced (2));

    auto adsrBounds = envArea.withTrimmedLeft (scaleInt (scale, 8));

    if (prefix == "osc1" && osc1AdsrDisplay != nullptr)
        osc1AdsrDisplay->setBounds (adsrBounds);
    else if (prefix == "osc2" && osc2AdsrDisplay != nullptr)
        osc2AdsrDisplay->setBounds (adsrBounds);
}

void TillySynthEditor::layoutNoiseSection (juce::Rectangle<int> area)
{
    const float scale = lookAndFeel.getScale();
    const int sectionTitleHeight = scaleInt (scale, kSectionTitleHeight);
    const int labelHeight = scaleInt (scale, kLabelHeight);
    const int knobSize = scaleInt (scale, kKnobSize);
    const int smallKnobSize = scaleInt (scale, kSmallKnobSize);
    const int envelopeDisplayH = scaleInt (scale, kEnvelopeDisplayH);
    area.removeFromTop (sectionTitleHeight);

    int colW = area.getWidth() / 3;
    int knobH = knobSize + labelHeight;

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row, int knobSz)
    {
        auto col = row.removeFromLeft (colW);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (knobSz).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    auto placeCombo = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (colW);
        if (combos.count (id))
        {
            combos[id].combo->setBounds (col.removeFromTop (knobSize)
                                            .reduced (scaleInt (scale, 4), scaleInt (scale, 14)));
            combos[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    // Row 1: Type + Level + S&H
    auto row1 = area.removeFromTop (knobH);
    placeCombo ("noise_type", row1);
    placeKnob ("noise_level", row1, knobSize);
    placeKnob ("noise_sh_rate", row1, knobSize);

    area.removeFromTop (scaleInt (scale, 2));

    // Row 2: ADSR
    int smallH = smallKnobSize + labelHeight;
    int colW2 = area.getWidth() / 4;
    auto row2 = area.removeFromTop (smallH);

    auto placeSmallKnob = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (colW2);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (smallKnobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    placeSmallKnob ("noise_attack", row2);
    placeSmallKnob ("noise_decay", row2);
    placeSmallKnob ("noise_sustain", row2);
    placeSmallKnob ("noise_release", row2);

    area.removeFromTop (scaleInt (scale, 2));

    // ADSR display
    auto envArea = area.removeFromTop (juce::jmin (envelopeDisplayH, area.getHeight()));
    if (noiseAdsrDisplay != nullptr)
        noiseAdsrDisplay->setBounds (envArea);
}

void TillySynthEditor::layoutFilterSection (juce::Rectangle<int> area)
{
    const float scale = lookAndFeel.getScale();
    const int sectionTitleHeight = scaleInt (scale, kSectionTitleHeight);
    const int labelHeight = scaleInt (scale, kLabelHeight);
    const int knobSize = scaleInt (scale, kKnobSize);
    const int smallKnobSize = scaleInt (scale, kSmallKnobSize);
    const int envelopeDisplayH = scaleInt (scale, kEnvelopeDisplayH);
    area.removeFromTop (sectionTitleHeight);

    int knobH = knobSize + labelHeight;

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row, int w, int knobSz)
    {
        auto col = row.removeFromLeft (w);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (knobSz).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    auto placeCombo = [&] (const juce::String& id, juce::Rectangle<int>& row, int w)
    {
        auto col = row.removeFromLeft (w);
        if (combos.count (id))
        {
            combos[id].combo->setBounds (col.removeFromTop (knobSize)
                                            .reduced (scaleInt (scale, 4), scaleInt (scale, 14)));
            combos[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    // Row 1: Mode + Slope + Target + Cutoff + Reso
    int colW1 = area.getWidth() / 5;
    auto row1 = area.removeFromTop (knobH);
    placeCombo ("filter_mode", row1, colW1);
    placeCombo ("filter_slope", row1, colW1);
    placeCombo ("filter_target", row1, colW1);
    placeKnob ("filter_cutoff", row1, colW1, knobSize);
    placeKnob ("filter_resonance", row1, colW1, knobSize);

    area.removeFromTop (scaleInt (scale, 2));

    // Row 2: EnvAmt + Key + Vel + A + D + S + R
    int colW2 = area.getWidth() / 7;
    int smallH = smallKnobSize + labelHeight;
    auto row2 = area.removeFromTop (smallH);
    placeKnob ("filter_env_amount", row2, colW2, smallKnobSize);
    placeKnob ("filter_key_tracking", row2, colW2, smallKnobSize);
    placeKnob ("filter_velocity", row2, colW2, smallKnobSize);
    placeKnob ("filter_attack", row2, colW2, smallKnobSize);
    placeKnob ("filter_decay", row2, colW2, smallKnobSize);
    placeKnob ("filter_sustain", row2, colW2, smallKnobSize);
    placeKnob ("filter_release", row2, colW2, smallKnobSize);

    area.removeFromTop (scaleInt (scale, 2));

    // Filter ADSR display
    auto envArea = area.removeFromTop (juce::jmin (envelopeDisplayH, area.getHeight()));
    if (filterAdsrDisplay != nullptr)
        filterAdsrDisplay->setBounds (envArea);
}

void TillySynthEditor::layoutLFOSection (juce::Rectangle<int> area, const juce::String& prefix)
{
    const float scale = lookAndFeel.getScale();
    const int sectionTitleHeight = scaleInt (scale, kSectionTitleHeight);
    const int labelHeight = scaleInt (scale, kLabelHeight);
    const int knobSize = scaleInt (scale, kKnobSize);
    const int waveformSelectorH = scaleInt (scale, kWaveformSelectorH);
    const int toggleHeight = scaleInt (scale, kToggleHeight);
    area.removeFromTop (sectionTitleHeight);

    // Waveform selector strip
    auto wfArea = area.removeFromTop (waveformSelectorH);
    if (waveformSelectors.count (prefix + "_waveform"))
        waveformSelectors[prefix + "_waveform"]->setBounds (
            wfArea.reduced (scaleInt (scale, 4), scaleInt (scale, 2)));

    area.removeFromTop (scaleInt (scale, 4));

    // Rate + Depth
    int knobH = knobSize + labelHeight;
    int colW = area.getWidth() / 2;
    auto knobRow = area.removeFromTop (knobH);

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (colW);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (knobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    placeKnob (prefix + "_rate", knobRow);
    placeKnob (prefix + "_depth", knobRow);

    area.removeFromTop (scaleInt (scale, 4));

    // Destination toggles
    auto toggleRow = area.removeFromTop (toggleHeight);
    int btnW = area.getWidth() / 4;

    auto placeToggle = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (btnW);
        if (toggles.count (id))
            toggles[id].button->setBounds (col.reduced (2));
    };

    placeToggle (prefix + "_dest_cutoff", toggleRow);
    placeToggle (prefix + "_dest_pitch", toggleRow);
    placeToggle (prefix + "_dest_volume", toggleRow);
    placeToggle (prefix + "_dest_pw", toggleRow);

    // LFO waveform display is paint-only (handled in paint())
}

void TillySynthEditor::layoutModEnvSection (juce::Rectangle<int> area, const juce::String& prefix)
{
    const float scale = lookAndFeel.getScale();
    const int sectionTitleHeight = scaleInt (scale, kSectionTitleHeight);
    const int labelHeight = scaleInt (scale, kLabelHeight);
    const int knobSize = scaleInt (scale, kKnobSize);
    const int toggleHeight = scaleInt (scale, kToggleHeight);
    const int envelopeDisplayH = scaleInt (scale, kEnvelopeDisplayH + 20);
    area.removeFromTop (sectionTitleHeight);

    // Row 1: Amt + A + D + S + R
    int colW = area.getWidth() / 5;
    int knobH = knobSize + labelHeight;
    auto row1 = area.removeFromTop (knobH);

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (colW);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (knobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    placeKnob (prefix + "_amount", row1);
    placeKnob (prefix + "_attack", row1);
    placeKnob (prefix + "_decay", row1);
    placeKnob (prefix + "_sustain", row1);
    placeKnob (prefix + "_release", row1);

    area.removeFromTop (scaleInt (scale, 4));

    // Destination toggles
    auto toggleRow = area.removeFromTop (toggleHeight);
    int btnW = area.getWidth() / 4;

    auto placeToggle = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (btnW);
        if (toggles.count (id))
            toggles[id].button->setBounds (col.reduced (2));
    };

    placeToggle (prefix + "_dest_cutoff", toggleRow);
    placeToggle (prefix + "_dest_resonance", toggleRow);
    placeToggle (prefix + "_dest_pitch", toggleRow);
    placeToggle (prefix + "_dest_volume", toggleRow);

    area.removeFromTop (scaleInt (scale, 4));

    // ADSR display
    auto envArea = area.removeFromTop (juce::jmin (envelopeDisplayH, area.getHeight()));
    if (prefix == "modenv1" && modenv1AdsrDisplay != nullptr)
        modenv1AdsrDisplay->setBounds (envArea);
    else if (prefix == "modenv2" && modenv2AdsrDisplay != nullptr)
        modenv2AdsrDisplay->setBounds (envArea);
}

void TillySynthEditor::layoutEffectsSection (juce::Rectangle<int> area)
{
    const float scale = lookAndFeel.getScale();
    const int sectionTitleHeight = scaleInt (scale, kSectionTitleHeight);
    const int labelHeight = scaleInt (scale, kLabelHeight);
    const int smallKnobSize = scaleInt (scale, kSmallKnobSize);
    area.removeFromTop (sectionTitleHeight);

    int knobH = smallKnobSize + labelHeight;

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row, int w)
    {
        auto col = row.removeFromLeft (w);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (smallKnobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    // --- Chorus sub-section ---
    area.removeFromTop (scaleInt (scale, 14));

    auto chorusRow = area.removeFromTop (knobH);
    int chorusColW = area.getWidth() / 3;

    // Mode combo
    auto modeCol = chorusRow.removeFromLeft (chorusColW);
    if (combos.count ("chorus_mode"))
    {
        combos["chorus_mode"].combo->setBounds (modeCol.removeFromTop (smallKnobSize)
                                                    .reduced (scaleInt (scale, 4), scaleInt (scale, 8)));
        combos["chorus_mode"].label->setBounds (modeCol.removeFromTop (labelHeight));
    }

    placeKnob ("chorus_rate", chorusRow, chorusColW);
    placeKnob ("chorus_depth", chorusRow, chorusColW);

    area.removeFromTop (scaleInt (scale, 8));
    area.removeFromTop (scaleInt (scale, 12));

    // --- Reverb sub-section ---
    area.removeFromTop (scaleInt (scale, 14));
    area.removeFromTop (scaleInt (scale, 6));

    int reverbColW = area.getWidth() / 4;
    auto reverbRow1 = area.removeFromTop (knobH);
    placeKnob ("reverb_size", reverbRow1, reverbColW);
    placeKnob ("reverb_damping", reverbRow1, reverbColW);
    placeKnob ("reverb_mix", reverbRow1, reverbColW);
    placeKnob ("reverb_width", reverbRow1, reverbColW);
}

void TillySynthEditor::layoutMasterSection (juce::Rectangle<int> area)
{
    const float scale = lookAndFeel.getScale();
    const int sectionTitleHeight = scaleInt (scale, kSectionTitleHeight);
    const int labelHeight = scaleInt (scale, kLabelHeight);
    const int smallKnobSize = scaleInt (scale, kSmallKnobSize);
    const int toggleHeight = scaleInt (scale, kToggleHeight);
    area.removeFromTop (sectionTitleHeight);

    int knobH = smallKnobSize + labelHeight;

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row, int w)
    {
        auto col = row.removeFromLeft (w);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (smallKnobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (labelHeight));
        }
    };

    int colW = area.getWidth() / 3;

    // Row 1: Volume + Poly + Glide
    auto row1 = area.removeFromTop (knobH);
    placeKnob ("master_volume", row1, colW);
    placeKnob ("master_polyphony", row1, colW);
    placeKnob ("master_glide", row1, colW);

    area.removeFromTop (scaleInt (scale, 2));

    // Row 2: PB + Unison + UniDet
    auto row2 = area.removeFromTop (knobH);
    placeKnob ("master_pitch_bend", row2, colW);
    placeKnob ("master_unison", row2, colW);
    placeKnob ("master_unison_detune", row2, colW);

    area.removeFromTop (scaleInt (scale, 2));

    // Row 3: Sidechain + Legato toggle
    auto row3 = area.removeFromTop (knobH);
    placeKnob ("sidechain_amount", row3, colW);

    auto toggleArea = row3.removeFromLeft (colW * 2);
    if (toggles.count ("master_mono_legato"))
        toggles["master_mono_legato"].button->setBounds (
            toggleArea.reduced (scaleInt (scale, 4), (knobH - toggleHeight) / 2));
}

// ============================================================
//  Timer
// ============================================================

void TillySynthEditor::timerCallback()
{
    // VU meter ballistics
    float targetL = processorRef.outputLevelLeft.load();
    float targetR = processorRef.outputLevelRight.load();
    float attack = 0.3f;
    float release = 0.05f;
    vuLeft  += (targetL > vuLeft)  ? (targetL - vuLeft)  * attack : (targetL - vuLeft)  * release;
    vuRight += (targetR > vuRight) ? (targetR - vuRight) * attack : (targetR - vuRight) * release;

    // Copy scope buffer
    int writePos = processorRef.scopeWritePos.load();
    for (int i = 0; i < TillySynthProcessor::kScopeBufferSize; ++i)
    {
        int idx = (writePos + i) % TillySynthProcessor::kScopeBufferSize;
        scopeSnapshot[static_cast<size_t> (i)] = processorRef.scopeBuffer[static_cast<size_t> (idx)].load();
    }

    // Drift tooltip
    {
        auto& drift = processorRef.getDriftEngine();

        float avgPitch = 0.0f, avgCutoff = 0.0f;
        for (int i = 0; i < 16; ++i)
        {
            avgPitch  += std::abs (processorRef.driftVisPitch[static_cast<size_t> (i)].load());
            avgCutoff += std::abs (processorRef.driftVisCutoff[static_cast<size_t> (i)].load());
        }
        avgPitch  /= 16.0f;
        avgCutoff /= 16.0f;

        bool cpuOk     = drift.isCpuLoadAvailable();
        bool thermalOk = drift.isThermalAvailable();
        bool batteryOk = drift.isBatteryAvailable();
        float cpu      = drift.getCpuLoad();
        float thermal  = drift.getThermalPressure();
        float battery  = drift.getBatteryDrainRate();
        bool noSensors = !cpuOk && !thermalOk && !batteryOk;

        juce::String tip;
        tip << "ANALOGUE DRIFT ENGINE\n\n"
            << "Generates organic pitch and filter drift from\n"
            << "real hardware sensor data — every performance\n"
            << "is unique to your machine's state.\n\n"
            << "Sources:\n"
            << "  CPU load:          " << (cpuOk ? "active" : "unavailable") << "\n"
            << "  Thermal pressure:  " << (thermalOk ? "active" : "unavailable") << "\n"
            << "  Battery drain:     " << (batteryOk ? "active" : "n/a (desktop)") << "\n"
            << "  PRNG seed:         " << (noSensors ? "primary" : "supplementary") << "\n\n"
            << "Live readings:\n"
            << "  CPU load:          " << juce::String (static_cast<int> (cpu * 100.0f)) << "%\n"
            << "  Thermal pressure:  " << juce::String (thermal, 2) << "\n"
            << "  Battery drain:     " << juce::String (battery, 3) << "\n"
            << "  Avg pitch drift:   " << juce::String (avgPitch, 2) << " cents\n"
            << "  Avg cutoff drift:  " << juce::String (avgCutoff, 2) << " Hz";

        driftLabel.setTooltip (tip);

        // Evolve drift noise
        float driftAmount = processorRef.getAPVTS().getRawParameterValue ("master_analog_drift")->load() / 100.0f;
        float scaledDrift = driftAmount * driftAmount;
        auto& rng = juce::Random::getSystemRandom();
        for (size_t i = 0; i < driftNoise.size(); ++i)
        {
            float target = (rng.nextFloat() * 2.0f - 1.0f) * 0.15f * scaledDrift;
            driftNoise[i] = driftNoise[i] * 0.85f + target * 0.15f;
        }
    }

    // Wheel sync
    if (! pitchWheel.isDragging())
        pitchWheel.setValue (processorRef.pitchBendUI.load());
    if (! modWheel.isDragging())
        modWheel.setValue (processorRef.modWheelUI.load());

    repaint();
}

// ============================================================
//  Drawing helpers
// ============================================================

void TillySynthEditor::drawHeader (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const float scale = lookAndFeel.getScale();
    g.setColour (Colours::panelBackground.darker (0.3f));
    g.fillRect (bounds);

    g.setColour (Colours::warmAmber().withAlpha (0.4f));
    g.drawLine (static_cast<float> (bounds.getX()), static_cast<float> (bounds.getBottom()),
                static_cast<float> (bounds.getRight()), static_cast<float> (bounds.getBottom()),
                scaleFloat (scale, 1.5f));
}

void TillySynthEditor::cycleColourTheme()
{
    ThemeData::currentThemeIndex = (ThemeData::currentThemeIndex + 1)
        % static_cast<int> (ThemeData::themes.size());

    lookAndFeel.applyThemeColours();

    // Update keyboard overlay colours for new theme
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                        Colours::warmAmber().withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                        Colours::warmAmber().withAlpha (0.2f));

    // Update drift label colour
    driftLabel.setColour (juce::Label::textColourId, Colours::warmAmber().withAlpha (0.7f));

    // Update title button colour
    titleButton.setColour (juce::TextButton::textColourOffId, Colours::warmAmber());
    titleButton.setColour (juce::TextButton::textColourOnId, Colours::warmAmber());

    repaint();
}

void TillySynthEditor::drawDriftScope (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const float scale = lookAndFeel.getScale();

    // Background
    g.setColour (Colours::insetBackground);
    g.fillRect (bounds);

    g.setColour (Colours::panelBorder);
    g.drawLine (static_cast<float> (bounds.getX()), static_cast<float> (bounds.getY()),
                static_cast<float> (bounds.getRight()), static_cast<float> (bounds.getY()), 0.5f);

    const float leftReserved = static_cast<float> (
        juce::jmax (driftLabel.getRight(), driftBarKnob.getRight()) - bounds.getX());
    const float sensorWidth = scaleFloat (scale, 56.0f);
    const float leftPad = scaleFloat (scale, 10.0f);
    const float rightPad = scaleFloat (scale, 2.0f);
    const float scopeX = static_cast<float> (bounds.getX()) + leftReserved + leftPad;
    const float scopeW = juce::jmax (0.0f,
                                     static_cast<float> (bounds.getWidth()) - leftReserved
                                         - sensorWidth - leftPad - rightPad);
    float scopeY = static_cast<float> (bounds.getY()) + 4.0f;
    float scopeH = static_cast<float> (bounds.getHeight()) - 8.0f;
    float centreY = scopeY + scopeH * 0.5f;

    // Scope inset
    g.setColour (Colours::insetBackground.darker (0.2f));
    g.fillRoundedRectangle (scopeX - 2.0f, scopeY - 2.0f, scopeW + 4.0f, scopeH + 4.0f, 4.0f);

    // Centre line
    g.setColour (Colours::warmAmber().withAlpha (0.12f));
    g.drawLine (scopeX, centreY, scopeX + scopeW, centreY, 0.5f);

    // Grid lines
    g.setColour (Colours::warmAmber().withAlpha (0.06f));
    for (int q = 1; q <= 3; ++q)
    {
        float qx = scopeX + scopeW * static_cast<float> (q) / 4.0f;
        g.drawLine (qx, scopeY, qx, scopeY + scopeH, 0.5f);
    }
    // Horizontal grid
    for (int h = 1; h <= 3; h += 2)
    {
        float hy = scopeY + scopeH * static_cast<float> (h) / 4.0f;
        g.drawLine (scopeX, hy, scopeX + scopeW, hy, 0.5f);
    }

    // Waveform path
    juce::Path wavePath;
    int numPoints = TillySynthProcessor::kScopeBufferSize;
    float xStep = scopeW / static_cast<float> (numPoints - 1);

    for (int i = 0; i < numPoints; ++i)
    {
        float sample = scopeSnapshot[static_cast<size_t> (i)];
        sample = juce::jlimit (-1.0f, 1.0f, sample * 4.0f);
        sample += driftNoise[static_cast<size_t> (i)];

        float px = scopeX + static_cast<float> (i) * xStep;
        float py = centreY - sample * scopeH * 0.45f;

        if (i == 0)
            wavePath.startNewSubPath (px, py);
        else
            wavePath.lineTo (px, py);
    }

    // Glow pass
    g.setColour (Colours::warmAmber().withAlpha (0.15f));
    g.strokePath (wavePath, juce::PathStrokeType (4.0f));

    // Sharp pass
    g.setColour (Colours::warmAmber().withAlpha (0.8f));
    g.strokePath (wavePath, juce::PathStrokeType (1.8f));

    // Sensor readout
    auto& drift = processorRef.getDriftEngine();
    float cpu = drift.getCpuLoad();
    float thermal = drift.getThermalPressure();

    float readoutX = static_cast<float> (bounds.getRight()) - sensorWidth - 4.0f;
    float readoutY = static_cast<float> (bounds.getY()) + 4.0f;

    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.setColour (Colours::warmAmber().withAlpha (0.5f));
    g.drawText ("CPU " + juce::String (static_cast<int> (cpu * 100.0f)) + "%",
                juce::Rectangle<float> (readoutX, readoutY, sensorWidth, 14.0f),
                juce::Justification::centredRight);
    g.drawText ("TMP " + juce::String (thermal, 2),
                juce::Rectangle<float> (readoutX, readoutY + 16.0f, sensorWidth, 14.0f),
                juce::Justification::centredRight);
}

void TillySynthEditor::drawSectionBackground (juce::Graphics& g, juce::Rectangle<int> bounds,
                                                const juce::String& title)
{
    const float scale = lookAndFeel.getScale();
    const float cornerRadius = scaleFloat (scale, 6.0f);
    const int sectionTitleHeight = scaleInt (scale, kSectionTitleHeight);
    g.setColour (Colours::sectionBackground);
    g.fillRoundedRectangle (bounds.toFloat(), cornerRadius);

    g.setColour (Colours::panelBorder);
    g.drawRoundedRectangle (bounds.toFloat(), cornerRadius, scaleFloat (scale, 0.75f));

    // Title bar
    auto titleBar = bounds.removeFromTop (sectionTitleHeight);
    g.setColour (Colours::sectionHeader);
    g.fillRoundedRectangle (titleBar.toFloat().withHeight (static_cast<float> (sectionTitleHeight)),
                            cornerRadius);

    g.setColour (Colours::warmAmber().withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (scaleFloat (scale, 11.0f), juce::Font::bold)));
    const bool centreTitle = (title == "OUT");
    g.drawText (title,
                centreTitle ? titleBar : titleBar.withTrimmedLeft (scaleInt (scale, 10)),
                centreTitle ? juce::Justification::centred : juce::Justification::centredLeft);

    // Subtle line below title
    g.setColour (Colours::panelBorder.withAlpha (0.4f));
    g.drawLine (static_cast<float> (bounds.getX() + scaleInt (scale, 4)),
                static_cast<float> (titleBar.getBottom()),
                static_cast<float> (bounds.getRight() - scaleInt (scale, 4)),
                static_cast<float> (titleBar.getBottom()), scaleFloat (scale, 0.5f));
}

void TillySynthEditor::drawSubSectionLabel (juce::Graphics& g, juce::Rectangle<int> bounds,
                                              const juce::String& label)
{
    const float scale = lookAndFeel.getScale();
    g.setColour (Colours::dimText);
    g.setFont (juce::Font (juce::FontOptions (scaleFloat (scale, 9.0f), juce::Font::bold)));
    g.drawText (label, bounds.withTrimmedLeft (scaleInt (scale, 4)), juce::Justification::centredLeft);
}

void TillySynthEditor::drawVUMeter (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float meterWidth = juce::jmin (static_cast<float> (bounds.getWidth()) * 0.3f, 18.0f);
    float meterHeight = static_cast<float> (bounds.getHeight());
    float gap = meterWidth * 0.4f;

    float leftX = static_cast<float> (bounds.getCentreX()) - meterWidth - gap * 0.5f;
    float rightX = static_cast<float> (bounds.getCentreX()) + gap * 0.5f;
    float topY = static_cast<float> (bounds.getY());

    auto drawMeter = [&] (float x, float level)
    {
        g.setColour (Colours::insetBackground);
        g.fillRoundedRectangle (x, topY, meterWidth, meterHeight, 3.0f);

        float fillHeight = level * meterHeight;
        float fillY = topY + meterHeight - fillHeight;

        juce::Colour meterColour = (level > 0.9f) ? Colours::vuRed
                                 : (level > 0.6f) ? Colours::vuAmber
                                                   : Colours::vuGreen;

        g.setColour (meterColour.withAlpha (0.8f));
        g.fillRoundedRectangle (x + 2.0f, fillY, meterWidth - 4.0f, fillHeight, 2.0f);

        g.setColour (Colours::knobOutline.withAlpha (0.4f));
        g.drawRoundedRectangle (x, topY, meterWidth, meterHeight, 3.0f, 0.75f);
    };

    drawMeter (leftX, juce::jlimit (0.0f, 1.0f, vuLeft));
    drawMeter (rightX, juce::jlimit (0.0f, 1.0f, vuRight));

    g.setColour (Colours::labelText);
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText ("L", juce::Rectangle<float> (leftX, topY + meterHeight + 2.0f, meterWidth, 12.0f),
                juce::Justification::centred);
    g.drawText ("R", juce::Rectangle<float> (rightX, topY + meterHeight + 2.0f, meterWidth, 12.0f),
                juce::Justification::centred);
}

void TillySynthEditor::drawLFOWaveform (juce::Graphics& g, juce::Rectangle<int> bounds,
                                         int waveformType, float phase, float rate,
                                         float depth)
{
    const float scale = lookAndFeel.getScale();
    g.setColour (Colours::insetBackground);
    g.fillRoundedRectangle (bounds.toFloat(), scaleFloat (scale, 3.0f));

    float w = static_cast<float> (bounds.getWidth());
    float h = static_cast<float> (bounds.getHeight());
    float x0 = static_cast<float> (bounds.getX());
    float y0 = static_cast<float> (bounds.getY());
    float centreY = y0 + h * 0.5f;

    g.setColour (Colours::warmAmber().withAlpha (0.08f));
    g.drawLine (x0, centreY, x0 + w, centreY, 0.5f);

    juce::Path path;
    int numPoints = static_cast<int> (w);
    float cycles = juce::jmax (0.5f, juce::jmin (rate * 0.5f, 2.0f));

    for (int i = 0; i <= numPoints; ++i)
    {
        float t = static_cast<float> (i) / static_cast<float> (numPoints);
        float p = std::fmod (t * cycles + phase, 1.0f);
        if (p < 0.0f) p += 1.0f;

        float sample = 0.0f;
        switch (waveformType)
        {
            case 0: sample = std::sin (p * juce::MathConstants<float>::twoPi); break;
            case 1: sample = 2.0f * p - 1.0f; break;
            case 2: sample = (p < 0.5f) ? 1.0f : -1.0f; break;
            case 3: sample = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p); break;
        }

        float px = x0 + static_cast<float> (i);
        float amplitude = 0.05f + depth * 0.35f;
        float py = centreY - sample * h * amplitude;

        if (i == 0)
            path.startNewSubPath (px, py);
        else
            path.lineTo (px, py);
    }

    g.setColour (Colours::warmAmber().withAlpha (0.7f));
    g.strokePath (path, juce::PathStrokeType (scaleFloat (scale, 1.5f)));

    g.setColour (Colours::labelText.withAlpha (0.4f));
    g.setFont (juce::Font (juce::FontOptions (scaleFloat (scale, 8.0f))));
    g.drawText (juce::String (rate, 1) + " Hz",
                bounds.withTrimmedLeft (scaleInt (scale, 4)), juce::Justification::bottomLeft);
}

// ============================================================
//  WheelComponent
// ============================================================

WheelComponent::WheelComponent (bool bipolar, const juce::String& label)
    : isBipolar (bipolar), labelText (label)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void WheelComponent::setValue (float v)
{
    if (! dragging)
    {
        value = isBipolar ? juce::jlimit (-1.0f, 1.0f, v)
                          : juce::jlimit (0.0f, 1.0f, v);
        repaint();
    }
}

float WheelComponent::valueFromY (float mouseY) const
{
    auto area = getLocalBounds();
    area.removeFromTop (11);
    auto trackArea = area.reduced (0, 3);
    float ty = static_cast<float> (trackArea.getY());
    float th = static_cast<float> (trackArea.getHeight());

    float normalised = 1.0f - (mouseY - ty) / th;
    normalised = juce::jlimit (0.0f, 1.0f, normalised);

    if (isBipolar)
        return normalised * 2.0f - 1.0f;
    return normalised;
}

void WheelComponent::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    value = valueFromY (static_cast<float> (e.y));
    if (onValueChange) onValueChange (value);
    repaint();
}

void WheelComponent::mouseDrag (const juce::MouseEvent& e)
{
    value = valueFromY (static_cast<float> (e.y));
    if (onValueChange) onValueChange (value);
    repaint();
}

void WheelComponent::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
    if (isBipolar)
    {
        value = 0.0f;
        if (onValueChange) onValueChange (0.0f);
    }
    repaint();
}

void WheelComponent::paint (juce::Graphics& g)
{
    g.fillAll (Colours::panelBackground);

    auto area = getLocalBounds();

    auto labelArea = area.removeFromTop (11);
    g.setColour (Colours::labelText.withAlpha (0.5f));
    g.setFont (juce::Font (juce::FontOptions (8.0f)));
    g.drawText (labelText, labelArea, juce::Justification::centred);

    int trackW = 14;
    auto trackArea = area.reduced ((area.getWidth() - trackW) / 2, 3);
    float tx = static_cast<float> (trackArea.getX());
    float ty = static_cast<float> (trackArea.getY());
    float tw = static_cast<float> (trackArea.getWidth());
    float th = static_cast<float> (trackArea.getHeight());

    auto grooveRect = juce::Rectangle<float> (tx, ty, tw, th);
    g.setColour (Colours::insetBackground);
    g.fillRoundedRectangle (grooveRect, 4.0f);
    g.setColour (juce::Colour (0xFF111111));
    g.fillRoundedRectangle (tx, ty, tw, 6.0f, 4.0f);
    g.setColour (Colours::knobOutline.withAlpha (0.35f));
    g.drawRoundedRectangle (grooveRect, 4.0f, 0.75f);

    auto drawThumb = [&] (float thumbY)
    {
        float thumbH = 8.0f;
        auto thumbRect = juce::Rectangle<float> (tx + 1.5f, thumbY - thumbH * 0.5f,
                                                  tw - 3.0f, thumbH);
        g.setGradientFill (juce::ColourGradient (
            Colours::knobFill.brighter (0.4f), tx + 1.5f, thumbY - thumbH * 0.5f,
            Colours::knobFill.darker (0.1f), tx + 1.5f, thumbY + thumbH * 0.5f, false));
        g.fillRoundedRectangle (thumbRect, 2.5f);
        g.setColour (Colours::mutedCream.withAlpha (0.5f));
        g.drawLine (tx + 4.0f, thumbY, tx + tw - 4.0f, thumbY, 0.75f);
        g.setColour (Colours::knobOutline.withAlpha (0.6f));
        g.drawRoundedRectangle (thumbRect, 2.5f, 0.75f);
    };

    if (isBipolar)
    {
        float centreY = ty + th * 0.5f;
        g.setColour (Colours::knobOutline.withAlpha (0.3f));
        g.drawLine (tx + 2.0f, centreY, tx + tw - 2.0f, centreY, 0.5f);

        float fillH = std::abs (value) * th * 0.5f;
        if (fillH > 0.5f)
        {
            float fillY = (value >= 0.0f) ? centreY - fillH : centreY;
            g.setColour (Colours::warmAmber().withAlpha (0.25f));
            g.fillRoundedRectangle (tx + 2.0f, fillY, tw - 4.0f, fillH, 2.0f);
        }
        drawThumb (centreY - value * th * 0.5f);
    }
    else
    {
        float fillH = value * th;
        if (fillH > 0.5f)
        {
            g.setColour (Colours::warmAmber().withAlpha (0.25f));
            g.fillRoundedRectangle (tx + 2.0f, ty + th - fillH, tw - 4.0f, fillH, 2.0f);
        }
        drawThumb (ty + th - value * th);
    }
}

} // namespace tillysynth
