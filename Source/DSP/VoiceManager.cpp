#include "VoiceManager.h"

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
}

void VoiceManager::reset()
{
    for (auto& voice : voices)
        voice.reset();

    lfo1.reset();
    lfo2.reset();
    noteOrderCounter = 0;
    sustainPedalDown = false;
    sustainedNotes.fill (false);
    heldNotes.fill (-1);
    heldNoteCount = 0;
}

void VoiceManager::handleNoteOn (int midiNote, float velocity)
{
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

    // Combine both LFOs (additive) + mod wheel vibrato via LFO1's raw waveform
    float totalFilterMod = lfo1Out.cutoffMod + lfo2Out.cutoffMod;
    float modWheelVibrato = lfo1.getRawSample() * modWheelValue;
    float totalPitchMod  = lfo1Out.pitchMod  + lfo2Out.pitchMod + modWheelVibrato;
    float totalVolumeMod = lfo1Out.volumeMod + lfo2Out.volumeMod;
    float totalPWMod     = lfo1Out.pwMod     + lfo2Out.pwMod;

    float mono = 0.0f;

    for (int i = 0; i < maxPolyphony; ++i)
    {
        if (! voices[static_cast<size_t> (i)].isActive())
            continue;

        float driftPitch = driftEngine.getPitchDriftCents (i);
        float driftCutoff = driftEngine.getCutoffDriftHz (i);

        // Add pitch bend to pitch drift
        float totalPitchDrift = driftPitch + currentPitchBend * 100.0f;

        mono += voices[static_cast<size_t> (i)].processSample (
            totalFilterMod, totalPitchMod, totalVolumeMod, totalPWMod,
            totalPitchDrift, driftCutoff);
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

void VoiceManager::updateFilterParams (FilterMode mode, bool is24dB, float cutoff, float resonance,
                                        float envAmount, float keyTracking, float velocity)
{
    for (auto& voice : voices)
        voice.setFilterParams (mode, is24dB, cutoff, resonance, envAmount, keyTracking, velocity);
}

void VoiceManager::updateLFO1 (Waveform wf, float rate, float depth,
                                bool destCutoff, bool destPitch, bool destVolume, bool destPW)
{
    lfo1.setWaveform (wf);
    lfo1.setRate (rate);
    lfo1.setDepth (depth);
    lfo1.setDestinations (destCutoff, destPitch, destVolume, destPW);
}

void VoiceManager::updateLFO2 (Waveform wf, float rate, float depth,
                                bool destCutoff, bool destPitch, bool destVolume, bool destPW)
{
    lfo2.setWaveform (wf);
    lfo2.setRate (rate);
    lfo2.setDepth (depth);
    lfo2.setDestinations (destCutoff, destPitch, destVolume, destPW);
}

void VoiceManager::setMaxPolyphony (int voices_)
{
    maxPolyphony = juce::jlimit (1, kMaxVoices, voices_);
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
    int oldest = 0;
    int oldestOrder = voices[0].getNoteStartOrder();

    for (int i = 1; i < maxPolyphony; ++i)
    {
        if (voices[static_cast<size_t> (i)].getNoteStartOrder() < oldestOrder)
        {
            oldest = i;
            oldestOrder = voices[static_cast<size_t> (i)].getNoteStartOrder();
        }
    }

    return oldest;
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
