#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

namespace tillysynth
{

TillySynthProcessor::TillySynthProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TillySynthState", createParameterLayout()),
      presetManager (apvts)
{
    // Start drift engine
    float driftAmount = apvts.getRawParameterValue (ParamIDs::masterAnalogDrift)->load() / 100.0f;
    voiceManager.getDriftEngine().start (driftAmount);
}

void TillySynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare (sampleRate, samplesPerBlock);
    chorus.prepare (sampleRate, samplesPerBlock);
    masterVolume.reset (sampleRate, 0.02);
}

void TillySynthProcessor::releaseResources()
{
    voiceManager.reset();
    chorus.reset();
}

void TillySynthProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    updateParametersFromAPVTS();

    // Feed keyboard state for visual keyboard component
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // Process MIDI
    for (const auto metadata : midiMessages)
        handleMidiMessage (metadata.getMessage());

    // Expose drift values for UI visualisation
    for (int i = 0; i < 16; ++i)
    {
        driftVisPitch[static_cast<size_t> (i)].store (
            voiceManager.getDriftEngine().getPitchDriftCents (i));
        driftVisCutoff[static_cast<size_t> (i)].store (
            voiceManager.getDriftEngine().getCutoffDriftHz (i));
    }

    // Render audio sample-by-sample
    int numSamples = buffer.getNumSamples();
    auto* leftChannel  = buffer.getWritePointer (0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        float left = 0.0f, right = 0.0f;
        voiceManager.renderNextSample (left, right);

        float vol = masterVolume.getNextValue();
        left *= vol;
        right *= vol;

        leftChannel[i] = left;
        if (rightChannel != nullptr)
            rightChannel[i] = right;
    }

    // Chorus (post-voice processing)
    chorus.process (buffer);

    // Fill scope buffer (downsample to fit)
    {
        int writePos = scopeWritePos.load();
        const float* scopeSource = buffer.getReadPointer (0);
        // Write every Nth sample to keep scope buffer at a reasonable rate
        int step = juce::jmax (1, numSamples / 64);
        for (int i = 0; i < numSamples; i += step)
        {
            scopeBuffer[static_cast<size_t> (writePos)].store (scopeSource[i]);
            writePos = (writePos + 1) % kScopeBufferSize;
        }
        scopeWritePos.store (writePos);
    }

    // Expose LFO state for UI
    lfo1Phase.store (voiceManager.getLFO1().getPhase());
    lfo2Phase.store (voiceManager.getLFO2().getPhase());
    lfo1Waveform.store (static_cast<int> (voiceManager.getLFO1().getWaveform()));
    lfo2Waveform.store (static_cast<int> (voiceManager.getLFO2().getWaveform()));
    lfo1Rate.store (voiceManager.getLFO1().getRate());
    lfo2Rate.store (voiceManager.getLFO2().getRate());

    // Update output levels for VU meter
    outputLevelLeft.store (buffer.getRMSLevel (0, 0, numSamples));
    if (buffer.getNumChannels() > 1)
        outputLevelRight.store (buffer.getRMSLevel (1, 0, numSamples));
}

void TillySynthProcessor::updateParametersFromAPVTS()
{
    auto getFloat = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto getInt   = [this] (const char* id) { return static_cast<int> (apvts.getRawParameterValue (id)->load()); };
    auto getBool  = [this] (const char* id) { return apvts.getRawParameterValue (id)->load() > 0.5f; };

    // Oscillator 1
    voiceManager.updateOsc1Params (
        static_cast<Waveform> (getInt (ParamIDs::osc1Waveform)),
        getInt (ParamIDs::osc1Octave),
        getInt (ParamIDs::osc1Semitone),
        getFloat (ParamIDs::osc1FineTune),
        getFloat (ParamIDs::osc1Level) / 100.0f,
        getFloat (ParamIDs::osc1PulseWidth) / 100.0f,
        getInt (ParamIDs::osc1UnisonVoices),
        getFloat (ParamIDs::osc1UnisonDetune),
        getFloat (ParamIDs::osc1UnisonBlend) / 100.0f);

    // Oscillator 2
    voiceManager.updateOsc2Params (
        static_cast<Waveform> (getInt (ParamIDs::osc2Waveform)),
        getInt (ParamIDs::osc2Octave),
        getInt (ParamIDs::osc2Semitone),
        getFloat (ParamIDs::osc2FineTune),
        getFloat (ParamIDs::osc2Level) / 100.0f,
        getFloat (ParamIDs::osc2PulseWidth) / 100.0f,
        getInt (ParamIDs::osc2UnisonVoices),
        getFloat (ParamIDs::osc2UnisonDetune),
        getFloat (ParamIDs::osc2UnisonBlend) / 100.0f);

    // Amp envelopes
    voiceManager.updateAmpEnv1 (
        getFloat (ParamIDs::osc1Attack), getFloat (ParamIDs::osc1Decay),
        getFloat (ParamIDs::osc1Sustain) / 100.0f, getFloat (ParamIDs::osc1Release));

    voiceManager.updateAmpEnv2 (
        getFloat (ParamIDs::osc2Attack), getFloat (ParamIDs::osc2Decay),
        getFloat (ParamIDs::osc2Sustain) / 100.0f, getFloat (ParamIDs::osc2Release));

    // Filter
    voiceManager.updateFilterParams (
        static_cast<FilterMode> (getInt (ParamIDs::filterMode)),
        getInt (ParamIDs::filterSlope) == 1,
        getFloat (ParamIDs::filterCutoff),
        getFloat (ParamIDs::filterResonance) / 100.0f,
        getFloat (ParamIDs::filterEnvAmount) / 100.0f,
        getFloat (ParamIDs::filterKeyTracking) / 100.0f,
        getFloat (ParamIDs::filterVelocity) / 100.0f);

    // Filter envelope
    voiceManager.updateFilterEnv (
        getFloat (ParamIDs::filterAttack), getFloat (ParamIDs::filterDecay),
        getFloat (ParamIDs::filterSustain) / 100.0f, getFloat (ParamIDs::filterRelease));

    // LFO 1
    voiceManager.updateLFO1 (
        static_cast<Waveform> (getInt (ParamIDs::lfo1Waveform)),
        getFloat (ParamIDs::lfo1Rate),
        getFloat (ParamIDs::lfo1Depth) / 100.0f,
        getBool (ParamIDs::lfo1DestCutoff), getBool (ParamIDs::lfo1DestPitch),
        getBool (ParamIDs::lfo1DestVolume), getBool (ParamIDs::lfo1DestPW));

    // LFO 2
    voiceManager.updateLFO2 (
        static_cast<Waveform> (getInt (ParamIDs::lfo2Waveform)),
        getFloat (ParamIDs::lfo2Rate),
        getFloat (ParamIDs::lfo2Depth) / 100.0f,
        getBool (ParamIDs::lfo2DestCutoff), getBool (ParamIDs::lfo2DestPitch),
        getBool (ParamIDs::lfo2DestVolume), getBool (ParamIDs::lfo2DestPW));

    // Chorus
    chorus.setMode (static_cast<ChorusMode> (getInt (ParamIDs::chorusMode)));
    chorus.setRate (getFloat (ParamIDs::chorusRate));
    chorus.setDepth (getFloat (ParamIDs::chorusDepth) / 100.0f);

    // Master
    masterVolume.setTargetValue (getFloat (ParamIDs::masterVolume) / 100.0f * 0.25f);
    voiceManager.setMaxPolyphony (getInt (ParamIDs::masterPolyphony));
    voiceManager.setGlideTime (getFloat (ParamIDs::masterGlide));
    voiceManager.setPitchBendRange (getInt (ParamIDs::masterPitchBend));
    voiceManager.setMonoLegato (getBool (ParamIDs::masterMonoLegato));
    voiceManager.getDriftEngine().setDriftAmount (getFloat (ParamIDs::masterAnalogDrift) / 100.0f);
}

void TillySynthProcessor::handleMidiMessage (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
        voiceManager.handleNoteOn (msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isNoteOff())
        voiceManager.handleNoteOff (msg.getNoteNumber());
    else if (msg.isPitchWheel())
        voiceManager.handlePitchWheel (msg.getPitchWheelValue());
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        voiceManager.handleAllNotesOff();
}

juce::AudioProcessorEditor* TillySynthProcessor::createEditor()
{
    return new TillySynthEditor (*this);
}

void TillySynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TillySynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

} // namespace tillysynth

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new tillysynth::TillySynthProcessor();
}
