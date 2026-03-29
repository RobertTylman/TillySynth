#include <cmath>
#include <map>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "PresetVariantGenerator.h"

namespace tillysynth
{

struct ReviewStore
{
    juce::File getFile() const
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                       .getChildFile ("TillySynth");
        dir.createDirectory();
        return dir.getChildFile ("PresetReviewRatings.xml");
    }

    void load()
    {
        ratings.clear();
        notes.clear();

        auto xml = juce::XmlDocument::parse (getFile());
        if (xml == nullptr || ! xml->hasTagName ("PresetReviewRatings"))
            return;

        for (auto* child : xml->getChildIterator())
        {
            if (child->hasTagName ("Preset"))
            {
                auto key = child->getStringAttribute ("key");
                ratings.set (key, child->getIntAttribute ("rating", 0));
                notes[key] = child->getStringAttribute ("notes");
            }
        }
    }

    void save() const
    {
        juce::XmlElement xml ("PresetReviewRatings");

        for (const auto& entry : ratings)
        {
            auto* preset = xml.createNewChildElement ("Preset");
            auto key = entry.name.toString();
            preset->setAttribute ("key", key);
            preset->setAttribute ("rating", static_cast<int> (entry.value));
            preset->setAttribute ("notes", getNotes (key));
        }

        for (const auto& [key, value] : notes)
            if (! ratings.contains (key))
            {
                auto* preset = xml.createNewChildElement ("Preset");
                preset->setAttribute ("key", key);
                preset->setAttribute ("rating", 0);
                preset->setAttribute ("notes", value);
            }

        xml.writeTo (getFile());
    }

    int getRating (const juce::String& key) const
    {
        return ratings[key];
    }

    void setRating (const juce::String& key, int rating)
    {
        ratings.set (key, juce::jlimit (0, 5, rating));
        save();
    }

    juce::String getNotes (const juce::String& key) const
    {
        auto it = notes.find (key);
        return it != notes.end() ? it->second : juce::String();
    }

    void setNotes (const juce::String& key, const juce::String& value)
    {
        notes[key] = value;
        save();
    }

    int getRatedCount() const
    {
        int count = 0;
        for (const auto& entry : ratings)
            if (static_cast<int> (entry.value) > 0)
                ++count;
        return count;
    }

    juce::NamedValueSet ratings;
    std::map<juce::String, juce::String> notes;
};

struct NoteEvent
{
    int note = 60;
    double startBeat = 0.0;
    double lengthBeats = 0.5;
    uint8_t velocity = 96;
};

struct Melody
{
    juce::String name;
    std::vector<NoteEvent> notes;
    double totalBeats = 4.0;
    double bpm = 112.0;
};

static std::vector<Melody> createMelodies()
{
    return {
        { "Hook A",
          { { 60, 0.0, 0.5, 100 }, { 64, 0.5, 0.5, 94 }, { 67, 1.0, 1.0, 105 },
            { 72, 2.0, 0.5, 96 }, { 71, 2.5, 0.5, 90 }, { 67, 3.0, 1.0, 104 } },
          4.5, 112.0 },
        { "Hook B",
          { { 48, 0.0, 0.75, 108 }, { 55, 0.75, 0.75, 100 }, { 60, 1.5, 0.5, 92 },
            { 62, 2.0, 0.5, 92 }, { 67, 2.5, 0.75, 110 }, { 64, 3.25, 0.75, 88 } },
          4.5, 104.0 },
        { "Hook C",
          { { 60, 0.0, 0.25, 98 }, { 67, 0.25, 0.25, 96 }, { 72, 0.5, 0.5, 110 },
            { 67, 1.0, 0.5, 90 }, { 63, 1.5, 0.5, 92 }, { 70, 2.0, 1.0, 104 },
            { 65, 3.0, 0.5, 88 }, { 72, 3.5, 0.75, 106 } },
          4.5, 120.0 },
        { "Hook D",
          { { 57, 0.0, 1.0, 100 }, { 64, 0.0, 1.0, 82 }, { 69, 1.0, 0.75, 96 },
            { 72, 1.75, 0.5, 92 }, { 67, 2.25, 0.5, 90 }, { 64, 2.75, 1.25, 98 } },
          4.5, 98.0 },
        { "Hook E",
          { { 52, 0.0, 0.5, 104 }, { 59, 0.5, 0.5, 100 }, { 64, 1.0, 0.5, 96 },
            { 71, 1.5, 0.75, 108 }, { 67, 2.5, 0.5, 94 }, { 64, 3.0, 1.0, 102 } },
          4.5, 110.0 },
        { "Hook F",
          { { 60, 0.0, 0.25, 100 }, { 62, 0.25, 0.25, 96 }, { 67, 0.5, 0.5, 102 },
            { 69, 1.0, 0.5, 98 }, { 74, 1.5, 0.75, 108 }, { 72, 2.5, 0.25, 92 },
            { 67, 2.75, 0.5, 96 }, { 64, 3.25, 0.75, 100 } },
          4.5, 126.0 },
        { "Hook G",
          { { 45, 0.0, 0.75, 112 }, { 52, 0.75, 0.5, 102 }, { 57, 1.25, 0.5, 98 },
            { 60, 1.75, 0.5, 92 }, { 64, 2.25, 0.75, 104 }, { 57, 3.25, 0.75, 96 } },
          4.5, 102.0 },
        { "Hook H",
          { { 65, 0.0, 0.5, 96 }, { 72, 0.5, 0.5, 104 }, { 76, 1.0, 0.5, 108 },
            { 72, 1.5, 0.5, 96 }, { 69, 2.0, 0.5, 92 }, { 74, 2.5, 0.5, 100 },
            { 77, 3.0, 0.75, 106 } },
          4.5, 118.0 },
        { "Hook I",
          { { 48, 0.0, 1.0, 106 }, { 55, 0.0, 1.0, 84 }, { 60, 1.0, 0.5, 96 },
            { 63, 1.5, 0.5, 94 }, { 67, 2.0, 0.75, 102 }, { 70, 3.0, 0.75, 100 } },
          4.5, 96.0 },
        { "Hook J",
          { { 60, 0.0, 0.5, 98 }, { 67, 0.5, 0.25, 96 }, { 72, 0.75, 0.25, 104 },
            { 79, 1.0, 0.5, 110 }, { 76, 1.75, 0.5, 98 }, { 72, 2.5, 0.5, 96 },
            { 67, 3.0, 0.5, 92 }, { 64, 3.5, 0.5, 90 } },
          4.5, 124.0 },
        { "Hook K",
          { { 53, 0.0, 0.5, 102 }, { 60, 0.5, 0.5, 98 }, { 65, 1.0, 0.75, 104 },
            { 68, 2.0, 0.25, 92 }, { 72, 2.25, 0.5, 108 }, { 70, 3.0, 0.5, 94 },
            { 65, 3.5, 0.5, 98 } },
          4.5, 108.0 },
        { "Hook L",
          { { 57, 0.0, 0.25, 96 }, { 60, 0.25, 0.25, 94 }, { 64, 0.5, 0.5, 102 },
            { 69, 1.0, 0.5, 108 }, { 72, 1.5, 0.5, 104 }, { 76, 2.0, 0.75, 110 },
            { 72, 3.0, 0.5, 96 }, { 69, 3.5, 0.5, 92 } },
          4.5, 122.0 },
        { "Hook M",
          { { 43, 0.0, 0.75, 110 }, { 50, 0.75, 0.75, 102 }, { 55, 1.5, 0.5, 96 },
            { 58, 2.0, 0.5, 94 }, { 62, 2.5, 0.75, 104 }, { 65, 3.25, 0.75, 100 } },
          4.5, 100.0 },
        { "Hook N",
          { { 62, 0.0, 0.5, 100 }, { 69, 0.5, 0.5, 104 }, { 74, 1.0, 0.25, 108 },
            { 76, 1.25, 0.25, 100 }, { 81, 1.5, 0.75, 112 }, { 79, 2.5, 0.5, 98 },
            { 74, 3.0, 0.5, 96 }, { 69, 3.5, 0.5, 92 } },
          4.5, 128.0 }
    };
}

class PresetReviewComponent : public juce::AudioAppComponent,
                              public juce::ListBoxModel
{
public:
    PresetReviewComponent()
        : audioSettingsButton ("Audio Settings"),
          previousButton ("Previous"),
          skipButton ("Skip"),
          replayButton ("Replay"),
          generateButton ("Generate Variant"),
          deletePresetButton ("Delete Preset"),
          exportButton ("Export CSV"),
          pruneButton ("Delete 1-2* Users")
    {
        reviewStore.load();
        melodies = createMelodies();

        titleLabel.setJustificationType (juce::Justification::centred);
        titleLabel.setFont (juce::FontOptions (26.0f, juce::Font::bold));
        addAndMakeVisible (titleLabel);

        for (auto* label : { &detailLabel, &progressLabel, &ratingLabel, &melodyLabel, &statusLabel })
        {
            label->setJustificationType (juce::Justification::centred);
            addAndMakeVisible (*label);
        }

        detailLabel.setFont (juce::FontOptions (15.0f));
        progressLabel.setFont (juce::FontOptions (14.0f));
        ratingLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        melodyLabel.setFont (juce::FontOptions (14.0f));
        statusLabel.setFont (juce::FontOptions (13.0f));

        volumeLabel.setText ("Review Volume", juce::dontSendNotification);
        volumeLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (volumeLabel);

        volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        volumeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 22);
        volumeSlider.setRange (0.0, 150.0, 1.0);
        volumeSlider.setValue (100.0, juce::dontSendNotification);
        volumeSlider.setTextValueSuffix ("%");
        volumeSlider.onValueChange = [this]
        {
            reviewOutputGain.store (static_cast<float> (volumeSlider.getValue() / 100.0));
        };
        addAndMakeVisible (volumeSlider);

        notesLabel.setText ("Preset Notes", juce::dontSendNotification);
        notesLabel.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (notesLabel);

        notesEditor.setMultiLine (true);
        notesEditor.setReturnKeyStartsNewLine (true);
        notesEditor.setScrollbarsShown (true);
        notesEditor.setTextToShowWhenEmpty ("Type notes about this preset here...", juce::Colours::grey);
        notesEditor.onTextChange = [this] { saveNotesForCurrentPreset(); };
        addAndMakeVisible (notesEditor);

        presetList.setModel (this);
        presetList.setRowHeight (24);
        presetList.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff1c232b));
        presetList.setColour (juce::ListBox::outlineColourId, juce::Colour (0xff394757));
        addAndMakeVisible (presetList);

        for (int i = 0; i < 5; ++i)
        {
            auto button = std::make_unique<juce::TextButton> (juce::String (i + 1) + " Star");
            button->onClick = [this, rating = i + 1] { applyRating (rating); };
            addAndMakeVisible (*button);
            ratingButtons.push_back (std::move (button));
        }

        audioSettingsButton.onClick = [this] { showAudioSettings(); };
        previousButton.onClick = [this] { loadPresetByIndex (currentPresetIndex - 1, true); };
        skipButton.onClick = [this] { loadPresetByIndex (currentPresetIndex + 1, true); };
        replayButton.onClick = [this] { restartAudition(); };
        generateButton.onClick = [this] { generateVariantFromCurrentPreset(); };
        deletePresetButton.onClick = [this] { deleteCurrentPreset(); };
        exportButton.onClick = [this] { exportRatingsCsv(); };
        pruneButton.onClick = [this] { deleteLowRatedUserPresets(); };

        for (auto* button : { &audioSettingsButton, &previousButton, &skipButton, &replayButton,
                              &generateButton, &deletePresetButton, &exportButton, &pruneButton })
        {
            addAndMakeVisible (*button);
        }

        setSize (1220, 620);
        setAudioChannels (0, 2);
        loadInitialPreset();
    }

    ~PresetReviewComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        currentSampleRate = sampleRate;
        renderBuffer.setSize (2, juce::jmax (samplesPerBlockExpected, 512));
        processor.prepareToPlay (sampleRate, samplesPerBlockExpected);
        restartAudition();
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        renderBuffer.setSize (2, bufferToFill.numSamples, false, false, true);
        renderBuffer.clear();

        juce::MidiBuffer midi;
        renderAuditionMidi (midi, bufferToFill.numSamples);
        processor.processBlock (renderBuffer, midi);
        renderBuffer.applyGain (reviewOutputGain.load());

        for (int channel = 0; channel < juce::jmin (bufferToFill.buffer->getNumChannels(), renderBuffer.getNumChannels()); ++channel)
            bufferToFill.buffer->copyFrom (channel, bufferToFill.startSample, renderBuffer, channel, 0, bufferToFill.numSamples);
    }

    void releaseResources() override
    {
        processor.releaseResources();
        renderBuffer.setSize (0, 0);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff14181d));

        auto bounds = getLocalBounds().reduced (18);
        g.setColour (juce::Colour (0xff242c35));
        g.fillRoundedRectangle (bounds.toFloat(), 16.0f);

        g.setColour (juce::Colour (0xff394757));
        g.drawRoundedRectangle (bounds.toFloat(), 16.0f, 1.2f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (32);

        auto listArea = area.removeFromRight (430);
        listArea.removeFromLeft (16);
        presetList.setBounds (listArea);

        auto mainArea = area;
        titleLabel.setBounds (mainArea.removeFromTop (40));
        detailLabel.setBounds (mainArea.removeFromTop (26));
        progressLabel.setBounds (mainArea.removeFromTop (24));
        ratingLabel.setBounds (mainArea.removeFromTop (34));
        melodyLabel.setBounds (mainArea.removeFromTop (24));
        statusLabel.setBounds (mainArea.removeFromTop (28));
        auto volumeRow = mainArea.removeFromTop (28);
        volumeLabel.setBounds (volumeRow.removeFromLeft (110));
        volumeSlider.setBounds (volumeRow.removeFromLeft (260));
        mainArea.removeFromTop (18);

        auto stars = mainArea.removeFromTop (40);
        auto starWidth = stars.getWidth() / juce::jmax (1, static_cast<int> (ratingButtons.size()));
        for (auto& button : ratingButtons)
            button->setBounds (stars.removeFromLeft (starWidth).reduced (4, 0));

        mainArea.removeFromTop (18);
        auto controlsRow1 = mainArea.removeFromTop (34);
        previousButton.setBounds (controlsRow1.removeFromLeft (110).reduced (4, 0));
        skipButton.setBounds (controlsRow1.removeFromLeft (90).reduced (4, 0));
        replayButton.setBounds (controlsRow1.removeFromLeft (100).reduced (4, 0));
        generateButton.setBounds (controlsRow1.removeFromLeft (160).reduced (4, 0));
        deletePresetButton.setBounds (controlsRow1.removeFromLeft (130).reduced (4, 0));
        audioSettingsButton.setBounds (controlsRow1.removeFromLeft (130).reduced (4, 0));

        mainArea.removeFromTop (12);
        auto controlsRow2 = mainArea.removeFromTop (34);
        exportButton.setBounds (controlsRow2.removeFromLeft (140).reduced (4, 0));
        pruneButton.setBounds (controlsRow2.removeFromLeft (170).reduced (4, 0));

        mainArea.removeFromTop (18);
        notesLabel.setBounds (mainArea.removeFromTop (22));
        notesEditor.setBounds (mainArea.removeFromTop (180));
    }

    int getNumRows() override
    {
        return processor.getPresetManager().getNumPresets();
    }

    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        auto& presetManager = processor.getPresetManager();
        if (rowNumber < 0 || rowNumber >= presetManager.getNumPresets())
            return;

        auto isCurrent = rowNumber == currentPresetIndex;
        auto background = rowIsSelected ? juce::Colour (0xff314152)
                          : isCurrent ? juce::Colour (0xff25313d)
                                      : juce::Colour (0xff1c232b);
        g.fillAll (background);

        auto rating = reviewStore.getRating (presetManager.getPresetKey (rowNumber));
        auto stars = rating > 0 ? juce::String::repeatedString ("*", rating) : "-";
        auto type = presetManager.isUserPreset (rowNumber) ? "U" : "F";
        auto notes = reviewStore.getNotes (presetManager.getPresetKey (rowNumber))
                         .replaceCharacters ("\r\n", "  ")
                         .trim();
        if (notes.length() > 48)
            notes = notes.substring (0, 48).trimEnd() + "...";

        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f));
        auto text = juce::String (rowNumber + 1).paddedLeft ('0', 3)
                  + "  [" + stars.paddedRight (' ', 5) + "]  "
                  + type + "  "
                  + presetManager.getPresetCategory (rowNumber).paddedRight (' ', 10)
                  + "  " + presetManager.getPresetName (rowNumber)
                  + (notes.isNotEmpty() ? "  |  " + notes : "");
        g.drawText (text, 8, 0, width - 16, height, juce::Justification::centredLeft, false);
    }

    void selectedRowsChanged (int lastRowSelected) override
    {
        if (lastRowSelected >= 0 && lastRowSelected < processor.getPresetManager().getNumPresets()
            && lastRowSelected != currentPresetIndex)
        {
            loadPresetByIndex (lastRowSelected, true);
        }
    }

private:
    void renderAuditionMidi (juce::MidiBuffer& midi, int numSamples)
    {
        if (currentPresetIndex < 0 || melodies.empty())
            return;

        if (restartPending.exchange (false))
        {
            midi.addEvent (juce::MidiMessage::allNotesOff (1), 0);
            melodySamplePosition = 0;
            auditionFinished = false;
        }

        if (auditionFinished)
            return;

        const auto& melody = melodies[static_cast<size_t> (currentMelodyIndex)];
        const auto samplesPerBeat = (60.0 / melody.bpm) * currentSampleRate;
        const auto startSample = melodySamplePosition;
        const auto endSample = melodySamplePosition + numSamples;

        for (const auto& note : melody.notes)
        {
            auto noteOnSample = static_cast<int64_t> (std::llround (note.startBeat * samplesPerBeat));
            auto noteOffSample = static_cast<int64_t> (std::llround ((note.startBeat + note.lengthBeats) * samplesPerBeat));

            if (noteOnSample >= startSample && noteOnSample < endSample)
                midi.addEvent (juce::MidiMessage::noteOn (1, note.note, note.velocity), static_cast<int> (noteOnSample - startSample));

            if (noteOffSample >= startSample && noteOffSample < endSample)
                midi.addEvent (juce::MidiMessage::noteOff (1, note.note), static_cast<int> (noteOffSample - startSample));
        }

        melodySamplePosition = endSample;
        auto totalSamples = static_cast<int64_t> (std::llround (melody.totalBeats * samplesPerBeat));
        if (melodySamplePosition >= totalSamples)
        {
            midi.addEvent (juce::MidiMessage::allNotesOff (1), juce::jmax (0, numSamples - 1));
            auditionFinished = true;
        }
    }

    void loadInitialPreset()
    {
        auto nextUnrated = findNextUnratedFrom (-1);
        loadPresetByIndex (nextUnrated >= 0 ? nextUnrated : 0, true);
    }

    void loadPresetByIndex (int requestedIndex, bool restartPlayback)
    {
        auto total = processor.getPresetManager().getNumPresets();
        if (total <= 0)
            return;

        if (requestedIndex < 0)
            requestedIndex = total - 1;
        else if (requestedIndex >= total)
            requestedIndex = 0;

        currentPresetIndex = requestedIndex;
        processor.getPresetManager().loadPreset (currentPresetIndex);
        currentMelodyIndex = currentPresetIndex % static_cast<int> (melodies.size());
        loadNotesForCurrentPreset();
        presetList.selectRow (currentPresetIndex, juce::dontSendNotification);

        if (restartPlayback)
            restartAudition();

        refreshLabels();
    }

    void restartAudition()
    {
        restartPending.store (true);
        auditionFinished = false;
        refreshLabels();
    }

    void applyRating (int rating)
    {
        auto key = processor.getPresetManager().getPresetKey (currentPresetIndex);
        reviewStore.setRating (key, rating);
        statusLabel.setText ("Saved " + juce::String (rating) + "-star rating.", juce::dontSendNotification);
        presetList.repaint();

        auto nextUnrated = findNextUnratedFrom (currentPresetIndex);
        if (nextUnrated >= 0)
            loadPresetByIndex (nextUnrated, true);
        else
            refreshLabels();
    }

    int findNextUnratedFrom (int startIndex) const
    {
        auto total = processor.getPresetManager().getNumPresets();
        if (total <= 0)
            return -1;

        for (int offset = 1; offset <= total; ++offset)
        {
            auto index = (startIndex + offset + total) % total;
            if (reviewStore.getRating (processor.getPresetManager().getPresetKey (index)) == 0)
                return index;
        }

        return -1;
    }

    void refreshLabels()
    {
        auto& presetManager = processor.getPresetManager();
        auto total = presetManager.getNumPresets();
        if (total <= 0)
            return;

        auto name = presetManager.getPresetName (currentPresetIndex);
        auto category = presetManager.getPresetCategory (currentPresetIndex);
        auto rating = reviewStore.getRating (presetManager.getPresetKey (currentPresetIndex));
        auto ratedCount = 0;
        auto fourAndFive = 0;

        for (int i = 0; i < total; ++i)
        {
            auto presetRating = reviewStore.getRating (presetManager.getPresetKey (i));
            if (presetRating > 0)
                ++ratedCount;
            if (presetRating >= 4)
                ++fourAndFive;
        }

        titleLabel.setText (name, juce::dontSendNotification);
        detailLabel.setText ("Preset " + juce::String (currentPresetIndex + 1) + " / " + juce::String (total)
                             + "   |   " + category
                             + (presetManager.isUserPreset (currentPresetIndex) ? "   |   User/Generated" : "   |   Factory"),
                             juce::dontSendNotification);
        progressLabel.setText ("Rated: " + juce::String (ratedCount) + " / " + juce::String (total)
                               + "   |   Keepers (4-5): " + juce::String (fourAndFive),
                               juce::dontSendNotification);
        ratingLabel.setText (rating > 0 ? "Current rating: " + juce::String (rating) + " / 5"
                                        : "Current rating: Unrated",
                             juce::dontSendNotification);
        melodyLabel.setText ("Audition melody: " + melodies[static_cast<size_t> (currentMelodyIndex)].name
                             + (auditionFinished ? "   |   Playback finished" : "   |   Playing"),
                             juce::dontSendNotification);

        for (size_t i = 0; i < ratingButtons.size(); ++i)
            ratingButtons[i]->setColour (juce::TextButton::buttonColourId,
                                         static_cast<int> (i + 1) == rating ? juce::Colour (0xffe2a83b)
                                                                            : juce::Colour (0xff435264));

        presetList.repaint();
    }

    juce::String createUniqueVariantName (const juce::String& baseName) const
    {
        auto& presetManager = processor.getPresetManager();
        juce::String candidate;

        for (int counter = 1; counter < 1000; ++counter)
        {
            candidate = baseName + " Variant " + juce::String (counter);

            bool exists = false;
            for (int i = 0; i < presetManager.getNumPresets(); ++i)
                if (presetManager.getPresetName (i) == candidate)
                    exists = true;

            if (! exists)
                return candidate;
        }

        return baseName + " Variant";
    }

    void generateVariantFromCurrentPreset()
    {
        const auto* sourcePreset = processor.getPresetManager().getPreset (currentPresetIndex);
        if (sourcePreset == nullptr)
            return;

        auto variantName = createUniqueVariantName (sourcePreset->name);
        auto variant = PresetVariantGenerator::createVariant (*sourcePreset, random, variantName);

        processor.getPresetManager().saveUserPreset (variant);
        statusLabel.setText ("Created new preset " + variant.name + ". Current preset stayed unchanged.",
                             juce::dontSendNotification);
        presetList.updateContent();
        refreshLabels();
    }

    void deleteCurrentPreset()
    {
        auto& presetManager = processor.getPresetManager();
        if (! presetManager.isUserPreset (currentPresetIndex))
        {
            statusLabel.setText ("Only user/generated presets can be deleted individually.",
                                 juce::dontSendNotification);
            return;
        }

        presetManager.deleteUserPreset (currentPresetIndex);
        presetList.updateContent();

        auto total = presetManager.getNumPresets();
        if (total <= 0)
        {
            currentPresetIndex = -1;
            statusLabel.setText ("Preset deleted.", juce::dontSendNotification);
            presetList.repaint();
            return;
        }

        auto nextIndex = juce::jlimit (0, total - 1, currentPresetIndex);
        statusLabel.setText ("Deleted preset.", juce::dontSendNotification);
        loadPresetByIndex (nextIndex, true);
    }

    void loadNotesForCurrentPreset()
    {
        suppressNoteSave = true;
        notesEditor.setText (reviewStore.getNotes (processor.getPresetManager().getPresetKey (currentPresetIndex)),
                             juce::dontSendNotification);
        suppressNoteSave = false;
    }

    void saveNotesForCurrentPreset()
    {
        if (suppressNoteSave || currentPresetIndex < 0)
            return;

        reviewStore.setNotes (processor.getPresetManager().getPresetKey (currentPresetIndex),
                              notesEditor.getText());
    }

    void exportRatingsCsv()
    {
        auto file = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                        .getChildFile ("TillySynthPresetRatings.csv");

        juce::String csv = "type,index,name,category,rating\n";
        auto& presetManager = processor.getPresetManager();
        for (int i = 0; i < presetManager.getNumPresets(); ++i)
        {
            auto type = presetManager.isUserPreset (i) ? "user" : "factory";
            auto rating = reviewStore.getRating (presetManager.getPresetKey (i));
            csv += juce::String (type) + "," + juce::String (i + 1) + ",\""
                   + presetManager.getPresetName (i).replaceCharacter ('"', '\'') + "\",\""
                   + presetManager.getPresetCategory (i).replaceCharacter ('"', '\'') + "\","
                   + juce::String (rating) + "\n";
        }

        file.replaceWithText (csv);
        statusLabel.setText ("Exported ratings to " + file.getFullPathName(), juce::dontSendNotification);
        file.revealToUser();
    }

    void deleteLowRatedUserPresets()
    {
        auto& presetManager = processor.getPresetManager();
        auto deletedCount = 0;

        for (int i = presetManager.getNumPresets() - 1; i >= presetManager.getFactoryPresetCount(); --i)
        {
            auto rating = reviewStore.getRating (presetManager.getPresetKey (i));
            if (rating > 0 && rating <= 2)
            {
                presetManager.deleteUserPreset (i);
                ++deletedCount;
            }
        }

        if (deletedCount == 0)
        {
            statusLabel.setText ("No 1-2 star user presets to delete.", juce::dontSendNotification);
            return;
        }

        statusLabel.setText ("Deleted " + juce::String (deletedCount) + " low-rated user presets.",
                             juce::dontSendNotification);
        presetList.updateContent();
        loadPresetByIndex (juce::jmin (currentPresetIndex, presetManager.getNumPresets() - 1), false);
    }

    void showAudioSettings()
    {
        auto* selector = new juce::AudioDeviceSelectorComponent (deviceManager, 0, 0, 0, 2, false, false, true, false);
        selector->setSize (520, 360);

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (selector);
        options.dialogTitle = "Audio Settings";
        options.componentToCentreAround = this;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
    }

    TillySynthProcessor processor;
    ReviewStore reviewStore;
    juce::Random random;
    std::vector<Melody> melodies;
    juce::AudioBuffer<float> renderBuffer;

    juce::Label titleLabel;
    juce::Label detailLabel;
    juce::Label progressLabel;
    juce::Label ratingLabel;
    juce::Label melodyLabel;
    juce::Label statusLabel;
    juce::Label volumeLabel;
    juce::Label notesLabel;
    juce::Slider volumeSlider;
    juce::TextEditor notesEditor;
    juce::ListBox presetList;

    juce::TextButton audioSettingsButton;
    juce::TextButton previousButton;
    juce::TextButton skipButton;
    juce::TextButton replayButton;
    juce::TextButton generateButton;
    juce::TextButton deletePresetButton;
    juce::TextButton exportButton;
    juce::TextButton pruneButton;
    std::vector<std::unique_ptr<juce::TextButton>> ratingButtons;

    std::atomic<bool> restartPending { true };
    std::atomic<float> reviewOutputGain { 1.0f };
    bool auditionFinished = false;
    bool suppressNoteSave = false;
    double currentSampleRate = 44100.0;
    int64_t melodySamplePosition = 0;
    int currentPresetIndex = 0;
    int currentMelodyIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetReviewComponent)
};

class PresetReviewWindow : public juce::DocumentWindow
{
public:
    PresetReviewWindow()
        : juce::DocumentWindow ("TillySynth Preset Review",
                                juce::Colours::black,
                                juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        setContentOwned (new PresetReviewComponent(), true);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class PresetReviewApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override     { return "TillySynth Preset Review"; }
    const juce::String getApplicationVersion() override  { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override           { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<PresetReviewWindow>();
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    std::unique_ptr<PresetReviewWindow> mainWindow;
};

} // namespace tillysynth

START_JUCE_APPLICATION (tillysynth::PresetReviewApplication)
