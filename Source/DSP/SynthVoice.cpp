#include "SynthVoice.h"
#include <cmath>

namespace tillysynth
{

void SynthVoice::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    osc1.prepare (sr, samplesPerBlock);
    osc2.prepare (sr, samplesPerBlock);
    noiseOsc.prepare (sr, samplesPerBlock);
    ampEnv1.prepare (sr);
    ampEnv2.prepare (sr);
    noiseEnv.prepare (sr);
    filterEnv.prepare (sr);
    filter.prepare (sr, samplesPerBlock);

    // ~2ms fade-out for voice stealing
    stealFadeDecrement = 1.0f / static_cast<float> (sr * 0.002);
}

void SynthVoice::reset()
{
    osc1.reset();
    osc2.reset();
    noiseOsc.reset();
    ampEnv1.reset();
    ampEnv2.reset();
    noiseEnv.reset();
    filterEnv.reset();
    filter.reset();
    currentNote = -1;
    noteHeld = false;
}

void SynthVoice::noteOn (int midiNote, float velocity, bool legatoRetrigger)
{
    if (legatoRetrigger)
    {
        currentNote = midiNote;
        currentVelocity = velocity;
        noteHeld = true;
        return;
    }

    // If voice is currently active, initiate a quick fade-out before retriggering
    if (isActive() && ! stealing)
    {
        stealing = true;
        stealFadeLevel = 1.0f;
        pendingNote = midiNote;
        pendingVelocity = velocity;
        return;
    }

    // Set glide starting point from previous note if glide is active
    if (glideCoeff < 1.0f && currentNote >= 0)
        currentGlideNote = static_cast<float> (currentNote);
    else
        currentGlideNote = static_cast<float> (midiNote);

    currentNote = midiNote;
    currentVelocity = velocity;
    noteHeld = true;
    stealing = false;
    stealFadeLevel = 1.0f;

    ampEnv1.noteOn();
    ampEnv2.noteOn();
    noiseEnv.noteOn();
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
    noiseEnv.noteOff();
    filterEnv.noteOff();
}

float SynthVoice::processSample (float lfoFilterMod, float lfoPitchMod,
                                  float lfoVolumeMod, float lfoPWMod,
                                  float driftPitchCents, float driftCutoffHz)
{
    if (! isActive() && ! stealing)
        return 0.0f;

    // Handle voice stealing fade-out
    if (stealing)
    {
        stealFadeLevel -= stealFadeDecrement;
        if (stealFadeLevel <= 0.0f)
        {
            // Fade complete — retrigger with pending note
            stealFadeLevel = 1.0f;
            stealing = false;

            currentNote = pendingNote;
            currentVelocity = pendingVelocity;
            noteHeld = true;

            ampEnv1.reset();
            ampEnv2.reset();
            noiseEnv.reset();
            filterEnv.reset();
            osc1.reset();
            osc2.reset();
            noiseOsc.reset();
            filter.reset();

            ampEnv1.noteOn();
            ampEnv2.noteOn();
            noiseEnv.noteOn();
            filterEnv.noteOn();

            return 0.0f;
        }
    }

    // Apply glide (portamento) — smoothly interpolate the note number
    float targetNote = static_cast<float> (currentNote);
    if (currentGlideNote < 0.0f)
        currentGlideNote = targetNote;
    else
        currentGlideNote += (targetNote - currentGlideNote) * glideCoeff;

    float freq1 = calculateFrequency (currentGlideNote, osc1Octave, osc1Semitone,
                                       osc1FineTune, lfoPitchMod, driftPitchCents);
    float freq2 = calculateFrequency (currentGlideNote, osc2Octave, osc2Semitone,
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
    float noiseSample = noiseOsc.processSample();

    float env1 = ampEnv1.processSample();
    float env2 = ampEnv2.processSample();
    float envN = noiseEnv.processSample();

    float mixed = osc1Sample * env1 + osc2Sample * env2 + noiseSample * envN;

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

    float output = filter.processSample (mixed);

    if (stealing)
        output *= stealFadeLevel;

    return output;
}

bool SynthVoice::isActive() const
{
    return stealing || ampEnv1.isActive() || ampEnv2.isActive() || noiseEnv.isActive();
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

void SynthVoice::setNoiseParams (NoiseType type, float level01, float shRateHz)
{
    noiseOsc.setNoiseType (type);
    noiseOsc.setLevel (level01);
    noiseOsc.setSHRate (shRateHz);
}

void SynthVoice::setNoiseEnvParams (float attackMs, float decayMs, float sustain01, float releaseMs)
{
    noiseEnv.setParameters (attackMs, decayMs, sustain01, releaseMs);
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

void SynthVoice::setGlideTime (float glideMs)
{
    if (glideMs <= 0.0f)
        glideCoeff = 1.0f;
    else
    {
        float samples = static_cast<float> (glideMs * 0.001f * sampleRate);
        glideCoeff = 1.0f - std::exp (-4.0f / samples);
    }
}

float SynthVoice::calculateFrequency (float note, int octave, int semitone, float fineCents,
                                       float pitchMod, float driftCents) const
{
    float totalSemitones = note
                         + static_cast<float> (octave * 12)
                         + static_cast<float> (semitone);

    float totalCents = fineCents + driftCents + pitchMod * 200.0f;

    float midiNote = totalSemitones + totalCents / 100.0f;

    return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
}

} // namespace tillysynth
