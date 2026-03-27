#include "SynthVoice.h"
#include <cmath>

namespace tillysynth
{

void SynthVoice::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    osc1.prepare (sr, samplesPerBlock);
    osc2.prepare (sr, samplesPerBlock);
    ampEnv1.prepare (sr);
    ampEnv2.prepare (sr);
    filterEnv.prepare (sr);
    filter.prepare (sr, samplesPerBlock);
}

void SynthVoice::reset()
{
    osc1.reset();
    osc2.reset();
    ampEnv1.reset();
    ampEnv2.reset();
    filterEnv.reset();
    filter.reset();
    currentNote = -1;
    noteHeld = false;
}

void SynthVoice::noteOn (int midiNote, float velocity, bool legatoRetrigger)
{
    currentNote = midiNote;
    currentVelocity = velocity;
    noteHeld = true;

    if (legatoRetrigger)
    {
        // Legato: retrigger pitch only, envelopes continue
        return;
    }

    ampEnv1.noteOn();
    ampEnv2.noteOn();
    filterEnv.noteOn();

    osc1.reset();
    osc2.reset();
    filter.reset();
}

void SynthVoice::noteOff()
{
    noteHeld = false;
    ampEnv1.noteOff();
    ampEnv2.noteOff();
    filterEnv.noteOff();
}

float SynthVoice::processSample (float lfoFilterMod, float lfoPitchMod,
                                  float lfoVolumeMod, float lfoPWMod,
                                  float driftPitchCents, float driftCutoffHz)
{
    if (! isActive())
        return 0.0f;

    float freq1 = calculateFrequency (currentNote, osc1Octave, osc1Semitone,
                                       osc1FineTune, lfoPitchMod, driftPitchCents);
    float freq2 = calculateFrequency (currentNote, osc2Octave, osc2Semitone,
                                       osc2FineTune, lfoPitchMod, driftPitchCents);

    osc1.setFrequency (freq1);
    osc2.setFrequency (freq2);

    // Apply LFO pulse width modulation
    if (lfoPWMod != 0.0f)
    {
        // PWM shifts from current pulse width setting
        // handled externally by VoiceManager re-setting PW each block
    }

    float osc1Sample = osc1.processSample();
    float osc2Sample = osc2.processSample();

    float env1 = ampEnv1.processSample();
    float env2 = ampEnv2.processSample();

    float mixed = osc1Sample * env1 + osc2Sample * env2;

    // Volume LFO modulation
    float volumeScale = 1.0f + lfoVolumeMod * 0.5f;
    volumeScale = juce::jlimit (0.0f, 2.0f, volumeScale);
    mixed *= volumeScale;

    // Filter modulation
    float filterEnvValue = filterEnv.processSample();
    float envMod = filterEnvValue * filterEnvAmount * baseCutoff;

    // Key tracking: scale cutoff by note distance from middle C
    float keyTrackMod = filterKeyTracking * (static_cast<float> (currentNote) - 60.0f) * 50.0f;

    // Velocity-to-filter
    float velMod = filterVelocitySens * currentVelocity * baseCutoff;

    // LFO cutoff modulation (bipolar, scaled to useful range)
    float lfoCutoffMod = lfoFilterMod * baseCutoff * 0.5f;

    float modulatedCutoff = baseCutoff + envMod + keyTrackMod + velMod + lfoCutoffMod + driftCutoffHz;
    modulatedCutoff = juce::jlimit (20.0f, 20000.0f, modulatedCutoff);

    filter.setCutoff (modulatedCutoff);

    return filter.processSample (mixed);
}

bool SynthVoice::isActive() const
{
    return ampEnv1.isActive() || ampEnv2.isActive();
}

void SynthVoice::setOsc1Params (Waveform wf, int octave, int semitone, float fineTuneCents,
                                 float level01, float pw01, int unisonVoices,
                                 float unisonDetune, float unisonBlend)
{
    osc1Octave = octave;
    osc1Semitone = semitone;
    osc1FineTune = fineTuneCents;
    osc1.setWaveform (wf);
    osc1.setLevel (level01);
    osc1.setPulseWidth (pw01);
    osc1.setUnisonVoices (unisonVoices);
    osc1.setUnisonDetune (unisonDetune);
    osc1.setUnisonBlend (unisonBlend);
}

void SynthVoice::setOsc2Params (Waveform wf, int octave, int semitone, float fineTuneCents,
                                 float level01, float pw01, int unisonVoices,
                                 float unisonDetune, float unisonBlend)
{
    osc2Octave = octave;
    osc2Semitone = semitone;
    osc2FineTune = fineTuneCents;
    osc2.setWaveform (wf);
    osc2.setLevel (level01);
    osc2.setPulseWidth (pw01);
    osc2.setUnisonVoices (unisonVoices);
    osc2.setUnisonDetune (unisonDetune);
    osc2.setUnisonBlend (unisonBlend);
}

void SynthVoice::setAmpEnv1Params (float attackMs, float decayMs, float sustain01, float releaseMs)
{
    ampEnv1.setParameters (attackMs, decayMs, sustain01, releaseMs);
}

void SynthVoice::setAmpEnv2Params (float attackMs, float decayMs, float sustain01, float releaseMs)
{
    ampEnv2.setParameters (attackMs, decayMs, sustain01, releaseMs);
}

void SynthVoice::setFilterEnvParams (float attackMs, float decayMs, float sustain01, float releaseMs)
{
    filterEnv.setParameters (attackMs, decayMs, sustain01, releaseMs);
}

void SynthVoice::setFilterParams (FilterMode mode, bool is24dB, float cutoffHz, float resonance01,
                                   float envAmount, float keyTracking01, float velocity01)
{
    filter.setMode (mode);
    filter.setSlope24dB (is24dB);
    baseCutoff = cutoffHz;
    filter.setResonance (resonance01);
    filterEnvAmount = envAmount;
    filterKeyTracking = keyTracking01;
    filterVelocitySens = velocity01;
}

void SynthVoice::setGlideTime (float /*glideMs*/)
{
    // Glide handled at VoiceManager level for note transitions
}

float SynthVoice::calculateFrequency (int note, int octave, int semitone, float fineCents,
                                       float pitchMod, float driftCents) const
{
    float totalSemitones = static_cast<float> (note)
                         + static_cast<float> (octave * 12)
                         + static_cast<float> (semitone);

    float totalCents = fineCents + driftCents + pitchMod * 200.0f;

    float midiNote = totalSemitones + totalCents / 100.0f;

    return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
}

} // namespace tillysynth
