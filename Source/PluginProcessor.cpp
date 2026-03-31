#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"
#include "DSP/ModulationMatrix.h"

namespace tillysynth
{

TillySynthProcessor::TillySynthProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, &undoManager, "TillySynthState", createParameterLayout()),
      presetManager (apvts)
{
    // Start drift engine
    float driftAmount = apvts.getRawParameterValue (ParamIDs::masterAnalogDrift)->load() / 100.0f;
    voiceManager.getDriftEngine().start (driftAmount);
}

bool TillySynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::stereo())
        return false;

    auto scIn = layouts.getChannelSet (true, 0);
    if (! scIn.isDisabled() && scIn != juce::AudioChannelSet::mono()
        && scIn != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void TillySynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare (sampleRate, samplesPerBlock);
    chorus.prepare (sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec reverbSpec;
    reverbSpec.sampleRate = sampleRate;
    reverbSpec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    reverbSpec.numChannels = 2;
    reverb.prepare (reverbSpec);

    masterVolume.reset (sampleRate, 0.02);
}

void TillySynthProcessor::releaseResources()
{
    voiceManager.reset();
    chorus.reset();
    reverb.reset();
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

    // Reverb (post-chorus)
    juce::dsp::AudioBlock<float> reverbBlock (buffer);
    juce::dsp::ProcessContextReplacing<float> reverbCtx (reverbBlock);
    reverb.process (reverbCtx);

    // Sidechain ducking
    {
        float scAmount = apvts.getRawParameterValue (ParamIDs::sidechainAmount)->load() / 100.0f;
        auto scBus = getBus (true, 0);
        if (scAmount > 0.0f && scBus != nullptr && scBus->isEnabled())
        {
            auto scBuffer = getBusBuffer (buffer, true, 0);
            int scChannels = scBuffer.getNumChannels();
            if (scChannels > 0)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    float scSample = 0.0f;
                    for (int ch = 0; ch < scChannels; ++ch)
                        scSample += std::abs (scBuffer.getSample (ch, i));
                    scSample /= static_cast<float> (scChannels);

                    float duckGain = 1.0f - (scSample * scAmount);
                    duckGain = juce::jmax (0.0f, duckGain);

                    leftChannel[i] *= duckGain;
                    if (rightChannel != nullptr)
                        rightChannel[i] *= duckGain;
                }
            }
        }
    }

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

    // Master unison applies to both oscillators
    int masterUni = getInt (ParamIDs::masterUnison);
    float masterUniDetune = getFloat (ParamIDs::masterUnisonDetune);

    // Use the higher of per-osc or master unison; add detune values
    int osc1Uni = std::max (getInt (ParamIDs::osc1UnisonVoices), masterUni);
    int osc2Uni = std::max (getInt (ParamIDs::osc2UnisonVoices), masterUni);
    float osc1Det = getFloat (ParamIDs::osc1UnisonDetune) + (masterUni > 1 ? masterUniDetune : 0.0f);
    float osc2Det = getFloat (ParamIDs::osc2UnisonDetune) + (masterUni > 1 ? masterUniDetune : 0.0f);

    // Oscillator 1
    voiceManager.updateOsc1Params (
        static_cast<Waveform> (getInt (ParamIDs::osc1Waveform)),
        getInt (ParamIDs::osc1Octave),
        getInt (ParamIDs::osc1Semitone),
        getFloat (ParamIDs::osc1FineTune),
        getFloat (ParamIDs::osc1Level) / 100.0f,
        getFloat (ParamIDs::osc1PulseWidth) / 100.0f,
        osc1Uni, osc1Det,
        getFloat (ParamIDs::osc1UnisonBlend) / 100.0f);

    // Oscillator 2
    voiceManager.updateOsc2Params (
        static_cast<Waveform> (getInt (ParamIDs::osc2Waveform)),
        getInt (ParamIDs::osc2Octave),
        getInt (ParamIDs::osc2Semitone),
        getFloat (ParamIDs::osc2FineTune),
        getFloat (ParamIDs::osc2Level) / 100.0f,
        getFloat (ParamIDs::osc2PulseWidth) / 100.0f,
        osc2Uni, osc2Det,
        getFloat (ParamIDs::osc2UnisonBlend) / 100.0f);

    // Amp envelopes
    voiceManager.updateAmpEnv1 (
        getFloat (ParamIDs::osc1Attack), getFloat (ParamIDs::osc1Decay),
        getFloat (ParamIDs::osc1Sustain) / 100.0f, getFloat (ParamIDs::osc1Release));

    voiceManager.updateAmpEnv2 (
        getFloat (ParamIDs::osc2Attack), getFloat (ParamIDs::osc2Decay),
        getFloat (ParamIDs::osc2Sustain) / 100.0f, getFloat (ParamIDs::osc2Release));

    // Noise
    voiceManager.updateNoiseParams (
        static_cast<NoiseType> (getInt (ParamIDs::noiseType)),
        getFloat (ParamIDs::noiseLevel) / 100.0f,
        getFloat (ParamIDs::noiseSHRate));

    voiceManager.updateNoiseEnv (
        getFloat (ParamIDs::noiseAttack), getFloat (ParamIDs::noiseDecay),
        getFloat (ParamIDs::noiseSustain) / 100.0f, getFloat (ParamIDs::noiseRelease));

    // Filter
    voiceManager.updateFilterParams (
        static_cast<FilterMode> (getInt (ParamIDs::filterMode)),
        getInt (ParamIDs::filterSlope) == 1,
        getFloat (ParamIDs::filterCutoff),
        getFloat (ParamIDs::filterResonance) / 100.0f,
        getFloat (ParamIDs::filterEnvAmount) / 100.0f,
        getFloat (ParamIDs::filterKeyTracking) / 100.0f,
        getFloat (ParamIDs::filterVelocity) / 100.0f,
        static_cast<FilterTarget> (getInt (ParamIDs::filterTarget)));

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

    // Mod envelope 1
    voiceManager.updateModEnv1 (
        getFloat (ParamIDs::modEnv1Attack), getFloat (ParamIDs::modEnv1Decay),
        getFloat (ParamIDs::modEnv1Sustain) / 100.0f, getFloat (ParamIDs::modEnv1Release),
        getFloat (ParamIDs::modEnv1Amount) / 100.0f,
        getBool (ParamIDs::modEnv1DestCutoff), getBool (ParamIDs::modEnv1DestResonance),
        getBool (ParamIDs::modEnv1DestPitch), getBool (ParamIDs::modEnv1DestVolume));

    // Mod envelope 2
    voiceManager.updateModEnv2 (
        getFloat (ParamIDs::modEnv2Attack), getFloat (ParamIDs::modEnv2Decay),
        getFloat (ParamIDs::modEnv2Sustain) / 100.0f, getFloat (ParamIDs::modEnv2Release),
        getFloat (ParamIDs::modEnv2Amount) / 100.0f,
        getBool (ParamIDs::modEnv2DestCutoff), getBool (ParamIDs::modEnv2DestResonance),
        getBool (ParamIDs::modEnv2DestPitch), getBool (ParamIDs::modEnv2DestVolume));

    // Modulation Matrix
    for (int i = 0; i < 8; ++i)
    {
        auto source = static_cast<ModSource> (getInt (ParamIDs::modMatrixSource[i]));
        auto dest   = static_cast<ModDest>   (getInt (ParamIDs::modMatrixDest[i]));
        float amount = getFloat (ParamIDs::modMatrixAmount[i]) / 100.0f;
        voiceManager.updateModMatrix (i, source, dest, amount);
    }

    // Chorus
    chorus.setMode (static_cast<ChorusMode> (getInt (ParamIDs::chorusMode)));
    chorus.setRate (getFloat (ParamIDs::chorusRate));
    chorus.setDepth (getFloat (ParamIDs::chorusDepth) / 100.0f);

    // Reverb
    {
        juce::dsp::Reverb::Parameters reverbParams;
        reverbParams.roomSize   = getFloat (ParamIDs::reverbSize) / 100.0f;
        reverbParams.damping    = getFloat (ParamIDs::reverbDamping) / 100.0f;
        reverbParams.wetLevel   = getFloat (ParamIDs::reverbMix) / 100.0f;
        reverbParams.dryLevel   = 1.0f - reverbParams.wetLevel;
        reverbParams.width      = getFloat (ParamIDs::reverbWidth) / 100.0f;
        reverb.setParameters (reverbParams);
    }

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
    {
        int note = juce::jlimit (0, 127, msg.getNoteNumber() + transposeSemitones);
        voiceManager.handleNoteOn (note, msg.getFloatVelocity());
    }
    else if (msg.isNoteOff())
    {
        int note = juce::jlimit (0, 127, msg.getNoteNumber() + transposeSemitones);
        voiceManager.handleNoteOff (note);
    }
    else if (msg.isPitchWheel())
    {
        voiceManager.handlePitchWheel (msg.getPitchWheelValue());
        pitchBendUI.store (static_cast<float> (msg.getPitchWheelValue() - 8192) / 8192.0f);
    }
    else if (msg.isControllerOfType (1))
    {
        float val = msg.getControllerValue() / 127.0f;
        modWheelUI.store (val);
        voiceManager.setModWheelValue (val);
    }
    else if (msg.isControllerOfType (64))
    {
        voiceManager.handleSustainPedal (msg.getControllerValue() >= 64);
    }
    else if (msg.isChannelPressure())
    {
        voiceManager.setAftertouchValue (msg.getChannelPressureValue() / 127.0f);
    }
    else if (msg.isAftertouch())
    {
        // Polyphonic aftertouch — use as channel-wide for now
        voiceManager.setAftertouchValue (msg.getAfterTouchValue() / 127.0f);
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        voiceManager.handleAllNotesOff();
}

void TillySynthProcessor::setPitchBendFromUI (float normalised)
{
    int midiValue = static_cast<int> (normalised * 8192.0f) + 8192;
    midiValue = juce::jlimit (0, 16383, midiValue);
    voiceManager.handlePitchWheel (midiValue);
    pitchBendUI.store (normalised);
}

void TillySynthProcessor::setModWheelFromUI (float value01)
{
    modWheelUI.store (value01);
    voiceManager.setModWheelValue (value01);
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
