#include "PluginEditor.h"
#include "Parameters.h"
#include <cmath>

namespace tillysynth
{

static constexpr int kWindowWidth  = 1000;
static constexpr int kWindowHeight = 780;
static constexpr int kHeaderHeight = 50;
static constexpr int kDriftBarHeight = 24;
static constexpr int kKeyboardHeight = 70;
static constexpr int kSectionPadding = 6;
static constexpr int kKnobSize = 60;
static constexpr int kLabelHeight = 16;

TillySynthEditor::TillySynthEditor (TillySynthProcessor& p)
    : juce::AudioProcessorEditor (p),
      processorRef (p),
      keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (700, 546, 1600, 1248);
    getConstrainer()->setFixedAspectRatio (static_cast<double> (kWindowWidth)
                                         / static_cast<double> (kWindowHeight));
    setSize (kWindowWidth, kWindowHeight);

    // Style the keyboard to match the Juno aesthetic
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, Colours::mutedCream);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, Colours::panelBackground);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, Colours::knobOutline);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Colours::warmAmber.withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Colours::warmAmber.withAlpha (0.2f));
    keyboard.setOctaveForMiddleC (4);
    addAndMakeVisible (keyboard);

    // Preset selector
    auto presetNames = processorRef.getPresetManager().getPresetNames();
    for (int i = 0; i < presetNames.size(); ++i)
        presetSelector.addItem (presetNames[i], i + 1);

    presetSelector.setTextWhenNothingSelected ("Select Preset...");
    presetSelector.onChange = [this]
    {
        int idx = presetSelector.getSelectedItemIndex();
        if (idx >= 0)
            processorRef.getPresetManager().loadPreset (idx);
    };
    addAndMakeVisible (presetSelector);

    // Generate panel wear scuffs (normalised 0–1 coordinates, scaled at paint time)
    wearRandom.setSeed (static_cast<juce::int64> (juce::Time::currentTimeMillis()));
    for (int i = 0; i < 30; ++i)
    {
        wearScuffs.push_back ({
            wearRandom.nextFloat(),
            wearRandom.nextFloat()
        });
    }

    // --- Oscillator 1 ---
    auto addOscKnobs = [&] (const juce::String& prefix, const juce::String& /*name*/)
    {
        combos[prefix + "_waveform"] = createCombo (prefix + "_waveform", "Wave",
            { "Sine", "Saw", "Square", "Tri" });
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

    addOscKnobs ("osc1", "OSC 1");
    addOscKnobs ("osc2", "OSC 2");

    // --- Filter ---
    combos["filter_mode"]  = createCombo ("filter_mode", "Mode", { "LP", "HP", "BP", "Notch" });
    combos["filter_slope"] = createCombo ("filter_slope", "Slope", { "12dB", "24dB" });
    knobs["filter_cutoff"]       = createKnob ("filter_cutoff", "Cutoff");
    knobs["filter_resonance"]    = createKnob ("filter_resonance", "Reso");
    knobs["filter_env_amount"]   = createKnob ("filter_env_amount", "Env");
    knobs["filter_key_tracking"] = createKnob ("filter_key_tracking", "Key");
    knobs["filter_velocity"]     = createKnob ("filter_velocity", "Vel");
    knobs["filter_attack"]       = createKnob ("filter_attack", "A");
    knobs["filter_decay"]        = createKnob ("filter_decay", "D");
    knobs["filter_sustain"]      = createKnob ("filter_sustain", "S");
    knobs["filter_release"]      = createKnob ("filter_release", "R");

    // --- LFOs ---
    auto addLFOControls = [&] (const juce::String& prefix)
    {
        combos[prefix + "_waveform"] = createCombo (prefix + "_waveform", "Wave",
            { "Sine", "Saw", "Sqr", "Tri" });
        knobs[prefix + "_rate"]  = createKnob (prefix + "_rate", "Rate");
        knobs[prefix + "_depth"] = createKnob (prefix + "_depth", "Depth");
        toggles[prefix + "_dest_cutoff"] = createToggle (prefix + "_dest_cutoff", "Cut");
        toggles[prefix + "_dest_pitch"]  = createToggle (prefix + "_dest_pitch", "Pit");
        toggles[prefix + "_dest_volume"] = createToggle (prefix + "_dest_volume", "Vol");
        toggles[prefix + "_dest_pw"]     = createToggle (prefix + "_dest_pw", "PW");
    };

    addLFOControls ("lfo1");
    addLFOControls ("lfo2");

    // --- Chorus ---
    combos["chorus_mode"] = createCombo ("chorus_mode", "Mode", { "Off", "I", "II", "I+II" });
    knobs["chorus_rate"]  = createKnob ("chorus_rate", "Rate");
    knobs["chorus_depth"] = createKnob ("chorus_depth", "Depth");

    // --- Master ---
    knobs["master_volume"]     = createKnob ("master_volume", "Volume");
    knobs["master_polyphony"]  = createKnob ("master_polyphony", "Poly");
    knobs["master_glide"]      = createKnob ("master_glide", "Glide");
    knobs["master_pitch_bend"] = createKnob ("master_pitch_bend", "PB");
    toggles["master_mono_legato"] = createToggle ("master_mono_legato", "Legato");
    knobs["master_analog_drift"]  = createKnob ("master_analog_drift", "Drift");

    startTimerHz (30);
}

TillySynthEditor::~TillySynthEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
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

void TillySynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::panelBackground);

    auto bounds = getLocalBounds();

    drawHeader (g, bounds.removeFromTop (kHeaderHeight));

    // Drift visualisation bar below header
    auto driftBarArea = bounds.removeFromTop (kDriftBarHeight);
    drawDriftBar (g, driftBarArea);

    drawPanelWear (g, getLocalBounds());

    // Section backgrounds (exclude keyboard at bottom)
    auto contentArea = getLocalBounds()
        .withTrimmedTop (kHeaderHeight + kDriftBarHeight)
        .withTrimmedBottom (kKeyboardHeight)
        .reduced (kSectionPadding);
    int sectionHeight = (contentArea.getHeight() - kSectionPadding * 2) / 3;

    // Row 1: Oscillators
    auto row1 = contentArea.removeFromTop (sectionHeight);
    auto osc1Area = row1.removeFromLeft (row1.getWidth() / 2).reduced (kSectionPadding / 2);
    auto osc2Area = row1.reduced (kSectionPadding / 2);
    drawSectionBackground (g, osc1Area, "OSC 1");
    drawSectionBackground (g, osc2Area, "OSC 2");

    contentArea.removeFromTop (kSectionPadding);

    // Row 2: Filter + LFOs
    auto row2 = contentArea.removeFromTop (sectionHeight);
    int filterWidth = row2.getWidth() * 45 / 100;
    auto filterArea = row2.removeFromLeft (filterWidth).reduced (kSectionPadding / 2);
    auto lfosArea = row2.reduced (kSectionPadding / 2);
    auto lfo1Area = lfosArea.removeFromLeft (lfosArea.getWidth() / 2).reduced (kSectionPadding / 2);
    auto lfo2Area = lfosArea.reduced (kSectionPadding / 2);
    drawSectionBackground (g, filterArea, "FILTER");
    drawSectionBackground (g, lfo1Area, "LFO 1");
    drawSectionBackground (g, lfo2Area, "LFO 2");

    contentArea.removeFromTop (kSectionPadding);

    // Row 3: Chorus + Master + VU
    auto row3 = contentArea.reduced (kSectionPadding / 2);
    auto chorusArea = row3.removeFromLeft (row3.getWidth() / 3).reduced (kSectionPadding / 2);
    auto masterArea = row3.removeFromLeft (row3.getWidth() / 2).reduced (kSectionPadding / 2);
    auto vuArea = row3.reduced (kSectionPadding / 2);
    drawSectionBackground (g, chorusArea, "CHORUS");
    drawSectionBackground (g, masterArea, "MASTER");
    drawSectionBackground (g, vuArea, "OUTPUT");
    drawVUMeter (g, vuArea.reduced (10, 30));
}

void TillySynthEditor::resized()
{
    // Preset selector in header area
    presetSelector.setBounds (getWidth() / 2 - 120, 12, 240, 26);

    // Keyboard at the bottom
    keyboard.setBounds (getLocalBounds().removeFromBottom (kKeyboardHeight));

    auto contentArea = getLocalBounds()
        .withTrimmedTop (kHeaderHeight + kDriftBarHeight)
        .withTrimmedBottom (kKeyboardHeight)
        .reduced (kSectionPadding);
    int sectionHeight = (contentArea.getHeight() - kSectionPadding * 2) / 3;

    // Row 1: Oscillators
    auto row1 = contentArea.removeFromTop (sectionHeight);
    layoutOscillatorSection (row1.removeFromLeft (row1.getWidth() / 2).reduced (kSectionPadding), "osc1");
    layoutOscillatorSection (row1.reduced (kSectionPadding), "osc2");

    contentArea.removeFromTop (kSectionPadding);

    // Row 2: Filter + LFOs
    auto row2 = contentArea.removeFromTop (sectionHeight);
    int filterWidth = row2.getWidth() * 45 / 100;
    layoutFilterSection (row2.removeFromLeft (filterWidth).reduced (kSectionPadding));
    auto lfosArea = row2.reduced (kSectionPadding);
    layoutLFOSection (lfosArea.removeFromLeft (lfosArea.getWidth() / 2).reduced (kSectionPadding / 2), "lfo1");
    layoutLFOSection (lfosArea.reduced (kSectionPadding / 2), "lfo2");

    contentArea.removeFromTop (kSectionPadding);

    // Row 3: Chorus + Master + VU
    auto row3 = contentArea.reduced (kSectionPadding);
    layoutChorusSection (row3.removeFromLeft (row3.getWidth() / 3).reduced (kSectionPadding));
    layoutMasterSection (row3.removeFromLeft (row3.getWidth() / 2).reduced (kSectionPadding));
    // VU area is paint-only
}

void TillySynthEditor::layoutOscillatorSection (juce::Rectangle<int> area, const juce::String& prefix)
{
    area.removeFromTop (20);  // section title space

    int knobW = area.getWidth() / 5;
    int knobH = kKnobSize + kLabelHeight;

    // Row 1: Waveform, Octave, Semi, Fine, Level
    auto row1 = area.removeFromTop (knobH + 4);

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (knobW);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (kKnobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (kLabelHeight));
        }
    };

    auto placeCombo = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (knobW);
        if (combos.count (id))
        {
            auto comboBounds = col.removeFromTop (kKnobSize).reduced (4, 16);
            combos[id].combo->setBounds (comboBounds);
            combos[id].label->setBounds (col.removeFromTop (kLabelHeight));
        }
    };

    placeCombo (prefix + "_waveform", row1);
    placeKnob (prefix + "_octave", row1);
    placeKnob (prefix + "_semitone", row1);
    placeKnob (prefix + "_fine_tune", row1);
    placeKnob (prefix + "_level", row1);

    // Row 2: PW, Unison, Detune, Blend, ADSR
    area.removeFromTop (4);
    auto row2 = area.removeFromTop (knobH + 4);

    int knobW2 = area.getWidth() / 8;
    auto placeSmallKnob = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (knobW2);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (kKnobSize - 10).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (kLabelHeight));
        }
    };

    placeSmallKnob (prefix + "_pulse_width", row2);
    placeSmallKnob (prefix + "_unison_voices", row2);
    placeSmallKnob (prefix + "_unison_detune", row2);
    placeSmallKnob (prefix + "_unison_blend", row2);
    placeSmallKnob (prefix + "_attack", row2);
    placeSmallKnob (prefix + "_decay", row2);
    placeSmallKnob (prefix + "_sustain", row2);
    placeSmallKnob (prefix + "_release", row2);
}

void TillySynthEditor::layoutFilterSection (juce::Rectangle<int> area)
{
    area.removeFromTop (20);

    int knobW = area.getWidth() / 5;
    int knobH = kKnobSize + kLabelHeight;

    auto row1 = area.removeFromTop (knobH + 4);

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row, int w)
    {
        auto col = row.removeFromLeft (w);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (kKnobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (kLabelHeight));
        }
    };

    auto placeCombo = [&] (const juce::String& id, juce::Rectangle<int>& row, int w)
    {
        auto col = row.removeFromLeft (w);
        if (combos.count (id))
        {
            combos[id].combo->setBounds (col.removeFromTop (kKnobSize).reduced (4, 16));
            combos[id].label->setBounds (col.removeFromTop (kLabelHeight));
        }
    };

    placeCombo ("filter_mode", row1, knobW);
    placeCombo ("filter_slope", row1, knobW);
    placeKnob ("filter_cutoff", row1, knobW);
    placeKnob ("filter_resonance", row1, knobW);
    placeKnob ("filter_env_amount", row1, knobW);

    area.removeFromTop (4);
    auto row2 = area.removeFromTop (knobH + 4);
    int knobW2 = area.getWidth() / 6;

    placeKnob ("filter_key_tracking", row2, knobW2);
    placeKnob ("filter_velocity", row2, knobW2);
    placeKnob ("filter_attack", row2, knobW2);
    placeKnob ("filter_decay", row2, knobW2);
    placeKnob ("filter_sustain", row2, knobW2);
    placeKnob ("filter_release", row2, knobW2);
}

void TillySynthEditor::layoutLFOSection (juce::Rectangle<int> area, const juce::String& prefix)
{
    area.removeFromTop (20);

    int knobW = area.getWidth() / 3;
    int knobH = kKnobSize + kLabelHeight;

    auto row1 = area.removeFromTop (knobH + 4);

    auto placeCombo = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (knobW);
        if (combos.count (id))
        {
            combos[id].combo->setBounds (col.removeFromTop (kKnobSize).reduced (4, 16));
            combos[id].label->setBounds (col.removeFromTop (kLabelHeight));
        }
    };

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (knobW);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (kKnobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (kLabelHeight));
        }
    };

    placeCombo (prefix + "_waveform", row1);
    placeKnob (prefix + "_rate", row1);
    placeKnob (prefix + "_depth", row1);

    // Destination toggles
    area.removeFromTop (8);
    auto row2 = area.removeFromTop (28);
    int btnW = area.getWidth() / 4;

    auto placeToggle = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (btnW);
        if (toggles.count (id))
            toggles[id].button->setBounds (col.reduced (2));
    };

    placeToggle (prefix + "_dest_cutoff", row2);
    placeToggle (prefix + "_dest_pitch", row2);
    placeToggle (prefix + "_dest_volume", row2);
    placeToggle (prefix + "_dest_pw", row2);
}

void TillySynthEditor::layoutChorusSection (juce::Rectangle<int> area)
{
    area.removeFromTop (20);

    int knobW = area.getWidth() / 3;
    int knobH = kKnobSize + kLabelHeight;

    auto row = area.removeFromTop (knobH + 4);

    auto col1 = row.removeFromLeft (knobW);
    if (combos.count ("chorus_mode"))
    {
        combos["chorus_mode"].combo->setBounds (col1.removeFromTop (kKnobSize).reduced (4, 16));
        combos["chorus_mode"].label->setBounds (col1.removeFromTop (kLabelHeight));
    }

    auto col2 = row.removeFromLeft (knobW);
    if (knobs.count ("chorus_rate"))
    {
        knobs["chorus_rate"].slider->setBounds (col2.removeFromTop (kKnobSize).reduced (2));
        knobs["chorus_rate"].label->setBounds (col2.removeFromTop (kLabelHeight));
    }

    auto col3 = row.removeFromLeft (knobW);
    if (knobs.count ("chorus_depth"))
    {
        knobs["chorus_depth"].slider->setBounds (col3.removeFromTop (kKnobSize).reduced (2));
        knobs["chorus_depth"].label->setBounds (col3.removeFromTop (kLabelHeight));
    }
}

void TillySynthEditor::layoutMasterSection (juce::Rectangle<int> area)
{
    area.removeFromTop (20);

    int knobW = area.getWidth() / 3;
    int knobH = kKnobSize + kLabelHeight;

    auto row1 = area.removeFromTop (knobH + 4);

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (knobW);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (kKnobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (kLabelHeight));
        }
    };

    placeKnob ("master_volume", row1);
    placeKnob ("master_polyphony", row1);
    placeKnob ("master_glide", row1);

    area.removeFromTop (4);
    auto row2 = area.removeFromTop (knobH + 4);

    placeKnob ("master_pitch_bend", row2);
    placeKnob ("master_analog_drift", row2);

    auto col = row2.removeFromLeft (knobW);
    if (toggles.count ("master_mono_legato"))
        toggles["master_mono_legato"].button->setBounds (col.reduced (4, 20));
}

void TillySynthEditor::timerCallback()
{
    // Analogue lag ballistics for VU meter
    float targetL = processorRef.outputLevelLeft.load();
    float targetR = processorRef.outputLevelRight.load();

    float attack = 0.3f;
    float release = 0.05f;

    vuLeft  += (targetL > vuLeft)  ? (targetL - vuLeft) * attack  : (targetL - vuLeft) * release;
    vuRight += (targetR > vuRight) ? (targetR - vuRight) * attack : (targetR - vuRight) * release;

    // Smooth drift values for visualisation
    for (int i = 0; i < 16; ++i)
    {
        float target = std::abs (processorRef.driftVisPitch[static_cast<size_t> (i)].load());
        smoothedDrift[static_cast<size_t> (i)] +=
            (target - smoothedDrift[static_cast<size_t> (i)]) * 0.2f;
    }

    repaint();
}

void TillySynthEditor::drawHeader (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour (Colours::panelBackground.darker (0.3f));
    g.fillRect (bounds);

    // Branding
    g.setColour (Colours::warmAmber);
    g.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    g.drawText ("TillySynth", bounds.withTrimmedLeft (20), juce::Justification::centredLeft);

    g.setColour (Colours::labelText.withAlpha (0.6f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("Robbie Tylman", bounds.withTrimmedRight (20), juce::Justification::centredRight);

    // Bottom line
    g.setColour (Colours::warmAmber.withAlpha (0.4f));
    g.drawLine (static_cast<float> (bounds.getX()), static_cast<float> (bounds.getBottom()),
                static_cast<float> (bounds.getRight()), static_cast<float> (bounds.getBottom()), 1.5f);
}

void TillySynthEditor::drawDriftBar (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour (Colours::panelBackground.darker (0.2f));
    g.fillRect (bounds);

    auto inner = bounds.reduced (8, 3);
    float barWidth = static_cast<float> (inner.getWidth()) / 16.0f;
    float maxBarHeight = static_cast<float> (inner.getHeight());

    // Label
    g.setColour (Colours::labelText.withAlpha (0.4f));
    g.setFont (juce::Font (juce::FontOptions (9.0f)));
    g.drawText ("DRIFT", bounds.withWidth (40).withTrimmedLeft (4), juce::Justification::centredLeft);

    float startX = static_cast<float> (inner.getX()) + 30.0f;
    float availableWidth = static_cast<float> (inner.getWidth()) - 30.0f;
    barWidth = availableWidth / 16.0f;

    for (int i = 0; i < 16; ++i)
    {
        float normDrift = smoothedDrift[static_cast<size_t> (i)] / 8.0f;
        normDrift = juce::jlimit (0.0f, 1.0f, normDrift);

        float h = normDrift * maxBarHeight;
        float x = startX + static_cast<float> (i) * barWidth;
        float y = static_cast<float> (inner.getY()) + maxBarHeight - h;

        // Colour shifts from amber to warm red with intensity
        auto barColour = Colours::warmAmber.interpolatedWith (Colours::vuRed, normDrift * 0.6f);
        g.setColour (barColour.withAlpha (0.7f + normDrift * 0.3f));
        g.fillRoundedRectangle (x + 1.0f, y, barWidth - 2.0f, h, 1.5f);
    }
}

void TillySynthEditor::drawSectionBackground (juce::Graphics& g, juce::Rectangle<int> bounds,
                                                const juce::String& title)
{
    g.setColour (Colours::sectionHeader);
    g.fillRoundedRectangle (bounds.toFloat(), 6.0f);

    g.setColour (Colours::panelBorder);
    g.drawRoundedRectangle (bounds.toFloat(), 6.0f, 1.0f);

    // Section title
    g.setColour (Colours::warmAmber.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText (title, bounds.withHeight (18).withTrimmedLeft (10),
                juce::Justification::centredLeft);
}

void TillySynthEditor::drawVUMeter (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float meterWidth = static_cast<float> (bounds.getWidth()) * 0.35f;
    float meterHeight = static_cast<float> (bounds.getHeight());
    float gap = static_cast<float> (bounds.getWidth()) * 0.1f;

    float leftX = static_cast<float> (bounds.getCentreX()) - meterWidth - gap * 0.5f;
    float rightX = static_cast<float> (bounds.getCentreX()) + gap * 0.5f;
    float topY = static_cast<float> (bounds.getY());

    auto drawMeter = [&] (float x, float level)
    {
        // Background
        g.setColour (Colours::panelBackground);
        g.fillRoundedRectangle (x, topY, meterWidth, meterHeight, 3.0f);

        // Level bar
        float fillHeight = level * meterHeight;
        float fillY = topY + meterHeight - fillHeight;

        juce::Colour meterColour = (level > 0.9f) ? Colours::vuRed
                                 : (level > 0.6f) ? Colours::vuAmber
                                                   : Colours::vuGreen;

        g.setColour (meterColour.withAlpha (0.8f));
        g.fillRoundedRectangle (x + 2.0f, fillY, meterWidth - 4.0f, fillHeight, 2.0f);

        // Outline
        g.setColour (Colours::knobOutline);
        g.drawRoundedRectangle (x, topY, meterWidth, meterHeight, 3.0f, 1.0f);
    };

    drawMeter (leftX, juce::jlimit (0.0f, 1.0f, vuLeft));
    drawMeter (rightX, juce::jlimit (0.0f, 1.0f, vuRight));

    // Labels
    g.setColour (Colours::labelText);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("L", juce::Rectangle<float> (leftX, topY + meterHeight + 2.0f, meterWidth, 14.0f),
                juce::Justification::centred);
    g.drawText ("R", juce::Rectangle<float> (rightX, topY + meterHeight + 2.0f, meterWidth, 14.0f),
                juce::Justification::centred);
}

void TillySynthEditor::drawPanelWear (juce::Graphics& g, juce::Rectangle<int> /*bounds*/)
{
    // Subtle per-instance randomised scuffs and surface texture
    g.setColour (juce::Colour (0x06FFFFFF));

    float w = static_cast<float> (getWidth());
    float h = static_cast<float> (getHeight());
    float scale = w / static_cast<float> (kWindowWidth);

    for (auto& scuff : wearScuffs)
    {
        float size = (5.0f + wearRandom.nextFloat() * 20.0f) * scale;
        float sx = scuff.x * w;
        float sy = scuff.y * h;
        g.fillEllipse (sx - size * 0.5f, sy - size * 0.5f, size, size);
    }

    // Subtle noise texture overlay
    g.setColour (juce::Colour (0x03000000));
    for (int i = 0; i < 200; ++i)
    {
        float x = wearRandom.nextFloat() * static_cast<float> (getWidth());
        float y = wearRandom.nextFloat() * static_cast<float> (getHeight());
        g.fillRect (x, y, 1.0f, 1.0f);
    }
}

} // namespace tillysynth
