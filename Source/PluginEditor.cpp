#include "PluginEditor.h"
#include "Parameters.h"
#include <cmath>

namespace tillysynth
{

static constexpr int kWindowWidth  = 1000;
static constexpr int kWindowHeight = 780;
static constexpr int kHeaderHeight = 50;
static constexpr int kDriftBarHeight = 36;
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

    // Style the keyboard to match the Juno aesthetic
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, Colours::mutedCream);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, Colours::panelBackground);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, Colours::knobOutline);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Colours::warmAmber.withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Colours::warmAmber.withAlpha (0.2f));
    keyboard.setOctaveForMiddleC (4);
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

    // Author link in header
    authorLink.setFont (juce::Font (juce::FontOptions (12.0f)), false);
    authorLink.setColour (juce::HyperlinkButton::textColourId,
                          Colours::labelText.withAlpha (0.6f));
    authorLink.setTooltip ("Visit Robbie Tylman's portfolio");
    addAndMakeVisible (authorLink);

    // Master volume slider in header
    masterVolumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterVolumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (masterVolumeSlider);
    masterVolumeAttachment = std::make_unique<SliderAttachment> (
        processorRef.getAPVTS(), "master_volume", masterVolumeSlider);

    masterVolumeLabel.setText ("Volume", juce::dontSendNotification);
    masterVolumeLabel.setJustificationType (juce::Justification::centredRight);
    masterVolumeLabel.setFont (juce::Font (juce::FontOptions (10.0f)));
    addAndMakeVisible (masterVolumeLabel);

    // Drift scope label with interactive tooltip
    driftLabel.setText ("ANALOGUE DRIFT", juce::dontSendNotification);
    driftLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    driftLabel.setColour (juce::Label::textColourId, Colours::warmAmber.withAlpha (0.7f));
    driftLabel.setInterceptsMouseClicks (true, false);
    driftLabel.setTooltip ("Analogue drift is generated from device sensor data:\n"
                           "- Motion (gyro/accelerometer) for fast pitch wobble\n"
                           "- Battery drain rate for slow thermal drift\n"
                           "- Falls back to PRNG if no sensors are available\n\n"
                           "Hover to see live values.");
    addAndMakeVisible (driftLabel);

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
    toggles["master_mono_legato"] = createToggle ("master_mono_legato", "Legato");
    knobs["master_analog_drift"]  = createKnob ("master_analog_drift", "Drift");
    knobs["master_unison"]         = createKnob ("master_unison", "Unison");
    knobs["master_unison_detune"]  = createKnob ("master_unison_detune", "UniDet");

    // Parameter tooltips
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
        setComboTip (prefix + "_waveform", "Oscillator waveform shape");
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

    setComboTip ("filter_mode", "Filter type: Low-pass, High-pass, Band-pass, or Notch");
    setComboTip ("filter_slope", "Filter steepness: 12 or 24 dB per octave");
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
        setComboTip (prefix + "_waveform", "LFO modulation waveform shape");
        setKnobTip (prefix + "_rate", "LFO speed in Hz");
        setKnobTip (prefix + "_depth", "LFO modulation intensity");
        setToggleTip (prefix + "_dest_cutoff", "Route LFO to filter cutoff");
        setToggleTip (prefix + "_dest_pitch", "Route LFO to oscillator pitch");
        setToggleTip (prefix + "_dest_volume", "Route LFO to output volume (tremolo)");
        setToggleTip (prefix + "_dest_pw", "Route LFO to pulse width (PWM)");
    };

    addLFOTips ("lfo1");
    addLFOTips ("lfo2");

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

    masterVolumeSlider.setTooltip ("Master output volume");

    // Set size AFTER all components exist so resized() can lay them out
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

    // Group presets by category
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

void TillySynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::panelBackground);

    auto bounds = getLocalBounds();

    drawHeader (g, bounds.removeFromTop (kHeaderHeight));

    // Oscilloscope drift display below header
    auto driftBarArea = bounds.removeFromTop (kDriftBarHeight);
    drawDriftScope (g, driftBarArea);

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

    // LFO waveform visualisation (bottom portion of each LFO section)
    auto lfo1VisArea = lfo1Area.withTrimmedTop (lfo1Area.getHeight() - 40).reduced (8, 4);
    auto lfo2VisArea = lfo2Area.withTrimmedTop (lfo2Area.getHeight() - 40).reduced (8, 4);
    drawLFOWaveform (g, lfo1VisArea,
                     processorRef.lfo1Waveform.load(),
                     processorRef.lfo1Phase.load(),
                     processorRef.lfo1Rate.load());
    drawLFOWaveform (g, lfo2VisArea,
                     processorRef.lfo2Waveform.load(),
                     processorRef.lfo2Phase.load(),
                     processorRef.lfo2Rate.load());

    contentArea.removeFromTop (kSectionPadding);

    // Row 3: Chorus + Reverb + Master + VU
    auto row3 = contentArea.reduced (kSectionPadding / 2);
    int vuWidth = 70;
    auto vuArea = row3.removeFromRight (vuWidth).reduced (kSectionPadding / 2);
    int thirdW = row3.getWidth() / 3;
    auto chorusArea = row3.removeFromLeft (thirdW).reduced (kSectionPadding / 2);
    auto reverbArea = row3.removeFromLeft (row3.getWidth() / 2).reduced (kSectionPadding / 2);
    auto masterArea = row3.reduced (kSectionPadding / 2);
    drawSectionBackground (g, chorusArea, "CHORUS");
    drawSectionBackground (g, reverbArea, "REVERB");
    drawSectionBackground (g, masterArea, "MASTER");
    drawSectionBackground (g, vuArea, "OUTPUT");
    drawVUMeter (g, vuArea.reduced (4, 30));
}

void TillySynthEditor::resized()
{
    // Header controls
    int headerCentreX = getWidth() / 2;
    presetPrev.setBounds (headerCentreX - 155, 12, 28, 26);
    presetSelector.setBounds (headerCentreX - 125, 12, 220, 26);
    presetNext.setBounds (headerCentreX + 97, 12, 28, 26);
    presetSave.setBounds (headerCentreX + 128, 12, 42, 26);
    presetRandom.setBounds (headerCentreX + 174, 12, 38, 26);

    // Author link (right edge)
    authorLink.setBounds (getWidth() - 120, 12, 110, 26);

    // Master volume slider in header (to the left of author link)
    masterVolumeLabel.setBounds (getWidth() - 280, 14, 50, 22);
    masterVolumeSlider.setBounds (getWidth() - 230, 14, 100, 22);

    // Drift label (positioned over the drift scope area)
    driftLabel.setBounds (6, kHeaderHeight + 2, 92, kDriftBarHeight - 4);

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

    // Row 3: Chorus + Reverb + Master + VU
    auto row3 = contentArea.reduced (kSectionPadding);
    row3.removeFromRight (70); // reserve VU space (paint-only)
    int thirdW = row3.getWidth() / 3;
    layoutChorusSection (row3.removeFromLeft (thirdW).reduced (kSectionPadding));
    layoutReverbSection (row3.removeFromLeft (row3.getWidth() / 2).reduced (kSectionPadding));
    layoutMasterSection (row3.reduced (kSectionPadding));
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

void TillySynthEditor::layoutReverbSection (juce::Rectangle<int> area)
{
    area.removeFromTop (20);

    int knobW = area.getWidth() / 2;
    int knobH = kKnobSize + kLabelHeight;

    auto placeKnob = [&] (const juce::String& id, juce::Rectangle<int>& row)
    {
        auto col = row.removeFromLeft (knobW);
        if (knobs.count (id))
        {
            knobs[id].slider->setBounds (col.removeFromTop (kKnobSize).reduced (2));
            knobs[id].label->setBounds (col.removeFromTop (kLabelHeight));
        }
    };

    auto row1 = area.removeFromTop (knobH + 4);
    placeKnob ("reverb_size", row1);
    placeKnob ("reverb_damping", row1);

    area.removeFromTop (4);
    auto row2 = area.removeFromTop (knobH + 4);
    placeKnob ("reverb_mix", row2);
    placeKnob ("reverb_width", row2);
}

void TillySynthEditor::layoutMasterSection (juce::Rectangle<int> area)
{
    area.removeFromTop (20);

    int knobW = area.getWidth() / 4;
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
    placeKnob ("master_unison", row1);

    area.removeFromTop (4);
    auto row2 = area.removeFromTop (knobH + 4);

    placeKnob ("master_pitch_bend", row2);
    placeKnob ("master_analog_drift", row2);
    placeKnob ("master_unison_detune", row2);

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

    // Copy scope buffer for oscilloscope display
    int writePos = processorRef.scopeWritePos.load();
    for (int i = 0; i < TillySynthProcessor::kScopeBufferSize; ++i)
    {
        int idx = (writePos + i) % TillySynthProcessor::kScopeBufferSize;
        scopeSnapshot[static_cast<size_t> (i)] = processorRef.scopeBuffer[static_cast<size_t> (idx)].load();
    }

    // Update drift tooltip with live sensor values
    {
        auto& drift = processorRef.voiceManager.getDriftEngine();

        float avgPitch = 0.0f, avgCutoff = 0.0f;
        for (int i = 0; i < 16; ++i)
        {
            avgPitch  += std::abs (processorRef.driftVisPitch[static_cast<size_t> (i)].load());
            avgCutoff += std::abs (processorRef.driftVisCutoff[static_cast<size_t> (i)].load());
        }
        avgPitch  /= 16.0f;
        avgCutoff /= 16.0f;

        bool motionOk  = drift.isMotionAvailable();
        bool batteryOk = drift.isBatteryAvailable();
        float motion   = drift.getMotionIntensity();
        float battery  = drift.getBatteryDrainRate();

        juce::String tip;
        tip << "ANALOGUE DRIFT ENGINE\n\n"
            << "Drift is generated from real-world sensor data to\n"
            << "create unique, organic pitch and filter variations.\n\n"
            << "Sources:\n"
            << "  Motion (gyro/accel): " << (motionOk ? "active" : "unavailable") << "\n"
            << "  Battery drain rate:  " << (batteryOk ? "active" : "unavailable") << "\n"
            << "  PRNG fallback:       " << ((!motionOk && !batteryOk) ? "active" : "standby") << "\n\n"
            << "Live values:\n"
            << "  Motion intensity:   " << juce::String (motion, 3) << "\n"
            << "  Battery drain:      " << juce::String (battery, 3) << "\n"
            << "  Avg pitch drift:    " << juce::String (avgPitch, 2) << " cents\n"
            << "  Avg cutoff drift:   " << juce::String (avgCutoff, 2) << " Hz";

        driftLabel.setTooltip (tip);
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

    // "Robbie Tylman" is rendered via authorLink HyperlinkButton (positioned in resized)

    // Bottom line
    g.setColour (Colours::warmAmber.withAlpha (0.4f));
    g.drawLine (static_cast<float> (bounds.getX()), static_cast<float> (bounds.getBottom()),
                static_cast<float> (bounds.getRight()), static_cast<float> (bounds.getBottom()), 1.5f);
}

void TillySynthEditor::drawDriftScope (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour (Colours::panelBackground.darker (0.4f));
    g.fillRect (bounds);

    // Borders
    g.setColour (Colours::panelBorder);
    g.drawLine (static_cast<float> (bounds.getX()), static_cast<float> (bounds.getY()),
                static_cast<float> (bounds.getRight()), static_cast<float> (bounds.getY()), 0.5f);

    // Label is rendered by the driftLabel component (with interactive tooltip)

    // Draw oscilloscope waveform (equal padding on both sides)
    float labelEnd = 94.0f;
    float pad = 6.0f;
    float scopeX = static_cast<float> (bounds.getX()) + labelEnd + pad;
    float scopeW = static_cast<float> (bounds.getWidth()) - labelEnd - pad * 2.0f;
    float scopeY = static_cast<float> (bounds.getY()) + 3.0f;
    float scopeH = static_cast<float> (bounds.getHeight()) - 6.0f;
    float centreY = scopeY + scopeH * 0.5f;

    // Centre line
    g.setColour (Colours::warmAmber.withAlpha (0.15f));
    g.drawLine (scopeX, centreY, scopeX + scopeW, centreY, 0.5f);

    // Grid lines
    g.setColour (Colours::warmAmber.withAlpha (0.07f));
    for (int q = 1; q <= 3; ++q)
    {
        float qx = scopeX + scopeW * static_cast<float> (q) / 4.0f;
        g.drawLine (qx, scopeY, qx, scopeY + scopeH, 0.5f);
    }

    // Draw waveform
    juce::Path wavePath;
    int numPoints = TillySynthProcessor::kScopeBufferSize;
    float xStep = scopeW / static_cast<float> (numPoints - 1);

    for (int i = 0; i < numPoints; ++i)
    {
        float sample = scopeSnapshot[static_cast<size_t> (i)];
        sample = juce::jlimit (-1.0f, 1.0f, sample * 4.0f); // amplify for visibility
        float px = scopeX + static_cast<float> (i) * xStep;
        float py = centreY - sample * scopeH * 0.45f;

        if (i == 0)
            wavePath.startNewSubPath (px, py);
        else
            wavePath.lineTo (px, py);
    }

    g.setColour (Colours::warmAmber.withAlpha (0.8f));
    g.strokePath (wavePath, juce::PathStrokeType (1.2f));

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
    float meterWidth = juce::jmin (static_cast<float> (bounds.getWidth()) * 0.12f, 14.0f);
    float meterHeight = static_cast<float> (bounds.getHeight());
    float gap = meterWidth * 0.6f;

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

void TillySynthEditor::drawLFOWaveform (juce::Graphics& g, juce::Rectangle<int> bounds,
                                         int waveformType, float phase, float rate)
{
    // Background
    g.setColour (Colours::panelBackground.darker (0.2f));
    g.fillRoundedRectangle (bounds.toFloat(), 3.0f);

    float w = static_cast<float> (bounds.getWidth());
    float h = static_cast<float> (bounds.getHeight());
    float x0 = static_cast<float> (bounds.getX());
    float y0 = static_cast<float> (bounds.getY());
    float centreY = y0 + h * 0.5f;

    // Centre line
    g.setColour (Colours::warmAmber.withAlpha (0.1f));
    g.drawLine (x0, centreY, x0 + w, centreY, 0.5f);

    // Draw 2 cycles of the waveform, shifted by current phase
    juce::Path path;
    int numPoints = static_cast<int> (w);
    float cycles = juce::jmax (1.0f, juce::jmin (rate, 4.0f));

    for (int i = 0; i <= numPoints; ++i)
    {
        float t = static_cast<float> (i) / static_cast<float> (numPoints);
        float p = std::fmod (t * cycles + phase, 1.0f);
        if (p < 0.0f) p += 1.0f;

        float sample = 0.0f;
        switch (waveformType)
        {
            case 0: // Sine
                sample = std::sin (p * juce::MathConstants<float>::twoPi);
                break;
            case 1: // Sawtooth
                sample = 2.0f * p - 1.0f;
                break;
            case 2: // Square
                sample = (p < 0.5f) ? 1.0f : -1.0f;
                break;
            case 3: // Triangle
                sample = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
                break;
        }

        float px = x0 + static_cast<float> (i);
        float py = centreY - sample * h * 0.4f;

        if (i == 0)
            path.startNewSubPath (px, py);
        else
            path.lineTo (px, py);
    }

    g.setColour (Colours::warmAmber.withAlpha (0.7f));
    g.strokePath (path, juce::PathStrokeType (1.5f));

    // Rate label
    g.setColour (Colours::labelText.withAlpha (0.4f));
    g.setFont (juce::Font (juce::FontOptions (8.0f)));
    g.drawText (juce::String (rate, 1) + " Hz",
                bounds.withTrimmedLeft (4), juce::Justification::bottomLeft);
}

} // namespace tillysynth
