#include "VoiceManager.h"
#include <limits>

namespace tillysynth
{

VoiceManager::VoiceManager()
{
    heldNotes.fill (-1);
}

void VoiceManager::prepare (double sampleRate, int samplesPerBlock)
{
    for (auto& voice : voices)
        voice.prepare (sampleRate, samplesPerBlock);

    lfo1.prepare (sampleRate);
    lfo2.prepare (sampleRate);
    lfo3.prepare (sampleRate);
}

void VoiceManager::reset()
{
    for (auto& voice : voices)
        voice.reset();

    lfo1.reset();
    lfo2.reset();
    lfo3.reset();
    noteOrderCounter = 0;
    sustainPedalDown = false;
    sustainedNotes.fill (false);
    heldNotes.fill (-1);
    heldNoteCount = 0;
}

void VoiceManager::handleNoteOn (int midiNote, float velocity)
{
    velocity = juce::jlimit (0.0f, 1.0f, velocity);

    if (monoLegato)
    {
        bool wasPlaying = heldNoteCount > 0;

        // Track held note
        if (heldNoteCount < 128)
        {
            heldNotes[static_cast<size_t> (heldNoteCount)] = midiNote;
            ++heldNoteCount;
        }

        if (wasPlaying)
        {
            // Legato: retrigger pitch only on voice 0
            voices[0].noteOn (midiNote, velocity, true);
        }
        else
        {
            voices[0].noteOn (midiNote, velocity, false);
            voices[0].setNoteStartOrder (noteOrderCounter++);
        }
        return;
    }

    // Check if this note is already playing
    int existing = findVoicePlayingNote (midiNote);
    if (existing >= 0)
    {
        voices[static_cast<size_t> (existing)].noteOn (midiNote, velocity);
        voices[static_cast<size_t> (existing)].setNoteStartOrder (noteOrderCounter++);
        return;
    }

    int voiceIndex = findFreeVoice();
    if (voiceIndex < 0)
        voiceIndex = findOldestVoice();

    if (voiceIndex >= 0)
    {
        voices[static_cast<size_t> (voiceIndex)].noteOn (midiNote, velocity);
        voices[static_cast<size_t> (voiceIndex)].setNoteStartOrder (noteOrderCounter++);
    }
}

void VoiceManager::handleNoteOff (int midiNote)
{
    if (monoLegato)
    {
        // Remove from held notes
        for (int i = 0; i < heldNoteCount; ++i)
        {
            if (heldNotes[static_cast<size_t> (i)] == midiNote)
            {
                for (int j = i; j < heldNoteCount - 1; ++j)
                    heldNotes[static_cast<size_t> (j)] = heldNotes[static_cast<size_t> (j + 1)];

                --heldNoteCount;
                break;
            }
        }

        if (heldNoteCount > 0)
        {
            // Return to previous note (legato)
            int prevNote = heldNotes[static_cast<size_t> (heldNoteCount - 1)];
            voices[0].noteOn (prevNote, 1.0f, true);
        }
        else
        {
            voices[0].noteOff();
        }
        return;
    }

    if (sustainPedalDown)
    {
        sustainedNotes[static_cast<size_t> (midiNote)] = true;
        return;
    }

    for (auto& voice : voices)
    {
        if (voice.getCurrentNote() == midiNote && voice.isNoteHeld())
            voice.noteOff();
    }
}

void VoiceManager::handleSustainPedal (bool isDown)
{
    sustainPedalDown = isDown;

    if (! isDown)
    {
        // Release all notes that were sustained
        for (int note = 0; note < 128; ++note)
        {
            if (sustainedNotes[static_cast<size_t> (note)])
            {
                sustainedNotes[static_cast<size_t> (note)] = false;
                for (auto& voice : voices)
                {
                    if (voice.getCurrentNote() == note && voice.isNoteHeld())
                        voice.noteOff();
                }
            }
        }
    }
}

void VoiceManager::handlePitchWheel (int pitchWheelValue)
{
    currentPitchBend = static_cast<float> (pitchWheelValue - 8192) / 8192.0f;
    currentPitchBend *= static_cast<float> (pitchBendRange);
}

void VoiceManager::handleAllNotesOff()
{
    for (auto& voice : voices)
        voice.noteOff();

    sustainPedalDown = false;
    sustainedNotes.fill (false);
    heldNotes.fill (-1);
    heldNoteCount = 0;
}

void VoiceManager::renderNextSample (float& leftOut, float& rightOut)
{
    auto lfo1Out = lfo1.processSample();
    auto lfo2Out = lfo2.processSample();
    auto lfo3Out = lfo3.processSample();

    // Combine all LFOs (additive) + mod wheel vibrato via LFO1's raw waveform
    float totalFilterMod = lfo1Out.cutoffMod + lfo2Out.cutoffMod + lfo3Out.cutoffMod;
    float modWheelVibrato = lfo1.getRawSample() * modWheelValue;
    float totalPitchMod  = lfo1Out.pitchMod  + lfo2Out.pitchMod  + lfo3Out.pitchMod + modWheelVibrato;
    float totalVolumeMod = lfo1Out.volumeMod + lfo2Out.volumeMod + lfo3Out.volumeMod;
    float totalPWMod     = lfo1Out.pwMod     + lfo2Out.pwMod     + lfo3Out.pwMod;

    // Build global source values for the mod matrix
    ModSourceValues globalSources;
    globalSources.lfo1       = lfo1.getRawSample();
    globalSources.lfo2       = lfo2.getRawSample();
    globalSources.lfo3       = lfo3.getRawSample();
    globalSources.modWheel   = modWheelValue;
    globalSources.aftertouch = aftertouchValue;

    // Apply matrix LFO rate modulation (computed from global sources only)
    // This creates feedback-free rate mod: current LFO value modulates next rate
    ModSourceValues lfoRateSources = globalSources;
    lfoRateSources.modEnv1 = 0.0f;
    lfoRateSources.modEnv2 = 0.0f;
    lfoRateSources.velocity = 0.0f;
    auto lfoRateMod = modMatrix.compute (lfoRateSources);

    if (lfoRateMod.lfo1Rate != 0.0f)
        lfo1.setRate (juce::jmax (0.01f, lfo1BaseRate + lfoRateMod.lfo1Rate * modDestRanges.lfo1Rate));
    if (lfoRateMod.lfo2Rate != 0.0f)
        lfo2.setRate (juce::jmax (0.01f, lfo2BaseRate + lfoRateMod.lfo2Rate * modDestRanges.lfo2Rate));
    if (lfoRateMod.lfo3Rate != 0.0f)
        lfo3.setRate (juce::jmax (0.01f, lfo3BaseRate + lfoRateMod.lfo3Rate * modDestRanges.lfo3Rate));

    float mono = 0.0f;

    for (int i = 0; i < maxPolyphony; ++i)
    {
        if (! voices[static_cast<size_t> (i)].isActive())
            continue;

        float driftPitch = driftEngine.getPitchDriftCents (i);
        float driftCutoff = driftEngine.getCutoffDriftHz (i);

        // Add pitch bend to pitch drift
        float totalPitchDrift = driftPitch + currentPitchBend * 100.0f;

        // Per-voice mod matrix: fill in voice-specific sources
        ModSourceValues voiceSources = globalSources;
        voiceSources.velocity = voices[static_cast<size_t> (i)].getVelocity();
        voiceSources.modEnv1  = voices[static_cast<size_t> (i)].getModEnv1Value();
        voiceSources.modEnv2  = voices[static_cast<size_t> (i)].getModEnv2Value();

        auto matrixOut = modMatrix.compute (voiceSources);

        mono += voices[static_cast<size_t> (i)].processSample (
            totalFilterMod, totalPitchMod, totalVolumeMod, totalPWMod,
            totalPitchDrift, driftCutoff, matrixOut);
    }

    // Fixed scaling based on max polyphony to prevent volume jumps
    // as releasing voices die off
    mono *= 1.0f / std::sqrt (static_cast<float> (maxPolyphony));

    leftOut = mono;
    rightOut = mono;
}

void VoiceManager::updateOsc1Params (Waveform wf, int octave, int semitone, float fineTune,
                                      float level, float pw, int unisonVoices,
                                      float unisonDetune, float unisonBlend)
{
    for (auto& voice : voices)
        voice.setOsc1Params (wf, octave, semitone, fineTune, level, pw,
                             unisonVoices, unisonDetune, unisonBlend);
}

void VoiceManager::updateOsc2Params (Waveform wf, int octave, int semitone, float fineTune,
                                      float level, float pw, int unisonVoices,
                                      float unisonDetune, float unisonBlend)
{
    for (auto& voice : voices)
        voice.setOsc2Params (wf, octave, semitone, fineTune, level, pw,
                             unisonVoices, unisonDetune, unisonBlend);
}

void VoiceManager::updateAmpEnv1 (float attack, float decay, float sustain, float release)
{
    for (auto& voice : voices)
        voice.setAmpEnv1Params (attack, decay, sustain, release);
}

void VoiceManager::updateAmpEnv2 (float attack, float decay, float sustain, float release)
{
    for (auto& voice : voices)
        voice.setAmpEnv2Params (attack, decay, sustain, release);
}

void VoiceManager::updateFilterEnv (float attack, float decay, float sustain, float release)
{
    for (auto& voice : voices)
        voice.setFilterEnvParams (attack, decay, sustain, release);
}

void VoiceManager::updateNoiseParams (NoiseType type, float level, float shRate)
{
    for (auto& voice : voices)
        voice.setNoiseParams (type, level, shRate);
}

void VoiceManager::updateNoiseEnv (float attack, float decay, float sustain, float release)
{
    for (auto& voice : voices)
        voice.setNoiseEnvParams (attack, decay, sustain, release);
}

void VoiceManager::updateFilterParams (FilterMode mode, FilterModel model, FilterSlope slope,
                                        float cutoff, float resonance,
                                        float envAmount, float keyTracking, float velocity,
                                        FilterTarget target)
{
    for (auto& voice : voices)
        voice.setFilterParams (mode, model, slope, cutoff, resonance, envAmount, keyTracking, velocity, target);
}

void VoiceManager::updateLFO1 (Waveform wf, float rate, float depth,
                                bool destCutoff, bool destPitch, bool destVolume, bool destPW)
{
    lfo1BaseRate = rate;
    lfo1.setWaveform (wf);
    lfo1.setRate (rate);
    lfo1.setDepth (depth);
    lfo1.setDestinations (destCutoff, destPitch, destVolume, destPW);
}

void VoiceManager::updateLFO2 (Waveform wf, float rate, float depth,
                                bool destCutoff, bool destPitch, bool destVolume, bool destPW)
{
    lfo2BaseRate = rate;
    lfo2.setWaveform (wf);
    lfo2.setRate (rate);
    lfo2.setDepth (depth);
    lfo2.setDestinations (destCutoff, destPitch, destVolume, destPW);
}

void VoiceManager::updateLFO3 (Waveform wf, float rate, float depth,
                                bool destCutoff, bool destPitch, bool destVolume, bool destPW)
{
    lfo3BaseRate = rate;
    lfo3.setWaveform (wf);
    lfo3.setRate (rate);
    lfo3.setDepth (depth);
    lfo3.setDestinations (destCutoff, destPitch, destVolume, destPW);
}

void VoiceManager::updateModEnv1 (float attack, float decay, float sustain, float release, float amount,
                                   bool destCutoff, bool destResonance, bool destPitch, bool destVolume)
{
    for (auto& voice : voices)
        voice.setModEnv1Params (attack, decay, sustain, release, amount,
                                destCutoff, destResonance, destPitch, destVolume);
}

void VoiceManager::updateModEnv2 (float attack, float decay, float sustain, float release, float amount,
                                   bool destCutoff, bool destResonance, bool destPitch, bool destVolume)
{
    for (auto& voice : voices)
        voice.setModEnv2Params (attack, decay, sustain, release, amount,
                                destCutoff, destResonance, destPitch, destVolume);
}

void VoiceManager::setMaxPolyphony (int voices_)
{
    int newMax = juce::jlimit (1, kMaxVoices, voices_);
    if (newMax < maxPolyphony)
    {
        // Clear voices that are no longer in the active pool so they cannot
        // become hidden/stuck when polyphony is reduced and later increased.
        for (int i = newMax; i < kMaxVoices; ++i)
            voices[static_cast<size_t> (i)].reset();
    }

    maxPolyphony = newMax;
}

void VoiceManager::setMonoLegato (bool enabled)
{
    monoLegato = enabled;
    if (enabled)
        maxPolyphony = 1;
}

void VoiceManager::setGlideTime (float ms)
{
    glideTimeMs = ms;
    for (auto& voice : voices)
        voice.setGlideTime (ms);
}

void VoiceManager::setPitchBendRange (int semitones)
{
    pitchBendRange = semitones;
}

void VoiceManager::setModWheelValue (float value01)
{
    modWheelValue = juce::jlimit (0.0f, 1.0f, value01);
}

void VoiceManager::setAftertouchValue (float value01)
{
    aftertouchValue = juce::jlimit (0.0f, 1.0f, value01);
}

void VoiceManager::updateModMatrix (int slotIndex, ModSource source, ModDest dest, float amount)
{
    modMatrix.setSlot (slotIndex, source, dest, amount);
}

void VoiceManager::updateModDestRanges (const ModDestRanges& ranges)
{
    modDestRanges = ranges;
    for (int i = 0; i < kMaxVoices; ++i)
        voices[static_cast<size_t> (i)].setModDestRanges (ranges);
}

int VoiceManager::findFreeVoice() const
{
    for (int i = 0; i < maxPolyphony; ++i)
    {
        if (! voices[static_cast<size_t> (i)].isActive())
            return i;
    }
    return -1;
}

int VoiceManager::findOldestVoice() const
{
    int oldestReleased = -1;
    int oldestReleasedOrder = std::numeric_limits<int>::max();
    int oldestHeld = -1;
    int oldestHeldOrder = std::numeric_limits<int>::max();

    for (int i = 0; i < maxPolyphony; ++i)
    {
        const auto& voice = voices[static_cast<size_t> (i)];
        if (! voice.isActive())
            continue;

        int order = voice.getNoteStartOrder();
        if (! voice.isNoteHeld())
        {
            if (order < oldestReleasedOrder)
            {
                oldestReleased = i;
                oldestReleasedOrder = order;
            }
        }
        else if (order < oldestHeldOrder)
        {
            oldestHeld = i;
            oldestHeldOrder = order;
        }
    }

    if (oldestReleased >= 0)
        return oldestReleased;
    if (oldestHeld >= 0)
        return oldestHeld;

    return 0;
}

int VoiceManager::findVoicePlayingNote (int midiNote) const
{
    for (int i = 0; i < maxPolyphony; ++i)
    {
        if (voices[static_cast<size_t> (i)].getCurrentNote() == midiNote
            && voices[static_cast<size_t> (i)].isNoteHeld())
            return i;
    }
    return -1;
}

} // namespace tillysynth
