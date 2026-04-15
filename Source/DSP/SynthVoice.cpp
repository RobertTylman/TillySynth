#include "SynthVoice.h"
#include <cmath>

namespace tillysynth
{

// Pulse width mod range now driven by modRanges.pulseWidth

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
    modEnv1.prepare (sr);
    modEnv2.prepare (sr);
    osc1Filter.prepare (sr, samplesPerBlock);
    osc2Filter.prepare (sr, samplesPerBlock);
    noiseFilter.prepare (sr, samplesPerBlock);

    // ~1 ms smoothing for retrigger discontinuities.
    clickSuppressDecay = std::exp (-1.0f / static_cast<float> (sr * 0.001));
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
    modEnv1.reset();
    modEnv2.reset();
    osc1Filter.reset();
    osc2Filter.reset();
    noiseFilter.reset();
    currentNote = -1;
    currentVelocity = 0.0f;
    noteHeld = false;
    currentGlideNote = -1.0f;
    retriggeredWhileActive = false;
    clickSuppressOffset = 0.0f;
    lastOutputSample = 0.0f;
}

void SynthVoice::noteOn (int midiNote, float velocity, bool legatoRetrigger)
{
    velocity = juce::jlimit (0.0f, 1.0f, velocity);

    if (legatoRetrigger)
    {
        currentNote = midiNote;
        currentVelocity = velocity;
        noteHeld = true;
        return;
    }

    retriggeredWhileActive = isActive();

    // Set glide starting point from previous note if glide is active
    if (glideCoeff < 1.0f && currentNote >= 0 && isActive())
        currentGlideNote = static_cast<float> (currentNote);
    else
        currentGlideNote = static_cast<float> (midiNote);

    currentNote = midiNote;
    currentVelocity = velocity;
    noteHeld = true;

    ampEnv1.noteOn();
    ampEnv2.noteOn();
    noiseEnv.noteOn();
    filterEnv.noteOn();
    modEnv1.noteOn();
    modEnv2.noteOn();

    osc1.reset();
    osc2.reset();
    noiseOsc.reset();
    osc1Filter.reset();
    osc2Filter.reset();
    noiseFilter.reset();
}

void SynthVoice::noteOff()
{
    noteHeld = false;
    ampEnv1.noteOff();
    ampEnv2.noteOff();
    noiseEnv.noteOff();
    filterEnv.noteOff();
    modEnv1.noteOff();
    modEnv2.noteOff();
}

float SynthVoice::processSample (float lfoFilterMod, float lfoPitchMod,
                                  float lfoVolumeMod, float lfoPWMod,
                                  float driftPitchCents, float driftCutoffHz,
                                  const ModulationOutput& matrixMod)
{
    if (! isActive())
    {
        lastOutputSample = 0.0f;
        clickSuppressOffset = 0.0f;
        return 0.0f;
    }

    // Apply glide (portamento) — smoothly interpolate the note number
    float targetNote = static_cast<float> (currentNote);
    if (currentGlideNote < 0.0f)
        currentGlideNote = targetNote;
    else
        currentGlideNote += (targetNote - currentGlideNote) * glideCoeff;

    lastModEnv1Value = modEnv1.processSample();
    lastModEnv2Value = modEnv2.processSample();
    float modEnv1Value = lastModEnv1Value * modEnv1Amount;
    float modEnv2Value = lastModEnv2Value * modEnv2Amount;

    float envCutoffMod = 0.0f;
    float envResonanceMod = 0.0f;
    float envPitchMod = 0.0f;
    float envVolumeMod = 0.0f;

    if (modEnv1DestCutoff)     envCutoffMod += modEnv1Value;
    if (modEnv1DestResonance)  envResonanceMod += modEnv1Value;
    if (modEnv1DestPitch)      envPitchMod += modEnv1Value;
    if (modEnv1DestVolume)     envVolumeMod += modEnv1Value;

    if (modEnv2DestCutoff)     envCutoffMod += modEnv2Value;
    if (modEnv2DestResonance)  envResonanceMod += modEnv2Value;
    if (modEnv2DestPitch)      envPitchMod += modEnv2Value;
    if (modEnv2DestVolume)     envVolumeMod += modEnv2Value;

    float totalPitchMod = lfoPitchMod + envPitchMod + matrixMod.pitch;

    float freq1 = calculateFrequency (currentGlideNote, osc1Octave, osc1Semitone,
                                       osc1FineTune, totalPitchMod, driftPitchCents);
    float freq2 = calculateFrequency (currentGlideNote, osc2Octave, osc2Semitone,
                                       osc2FineTune, totalPitchMod, driftPitchCents);

    osc1.setFrequency (freq1);
    osc2.setFrequency (freq2);
    float totalPWMod = lfoPWMod + matrixMod.pulseWidth;
    float pwRange = juce::jlimit (0.0f, 0.49f, modRanges.pulseWidth);
    osc1.setPulseWidth (juce::jlimit (0.01f, 0.99f,
                                      osc1PulseWidth + totalPWMod * pwRange));
    osc2.setPulseWidth (juce::jlimit (0.01f, 0.99f,
                                      osc2PulseWidth + totalPWMod * pwRange));

    float osc1Sample = osc1.processSample();
    float osc2Sample = osc2.processSample();
    float noiseSample = noiseOsc.processSample();

    float env1 = ampEnv1.processSample();
    float env2 = ampEnv2.processSample();
    float envN = noiseEnv.processSample();

    // Apply matrix level modulation scaled by mod ranges
    float osc1LevelScale = juce::jlimit (0.0f, 2.0f, 1.0f + matrixMod.osc1Level * modRanges.osc1Level);
    float osc2LevelScale = juce::jlimit (0.0f, 2.0f, 1.0f + matrixMod.osc2Level * modRanges.osc2Level);
    float noiseLevelScale = juce::jlimit (0.0f, 2.0f, 1.0f + matrixMod.noiseLevel * modRanges.noiseLevel);

    float osc1Signal = osc1Sample * env1 * osc1LevelScale;
    float osc2Signal = osc2Sample * env2 * osc2LevelScale;
    float noiseSignal = noiseSample * envN * noiseLevelScale;

    // Filter modulation
    float filterEnvValue = filterEnv.processSample();
    float envMod = filterEnvValue * filterEnvAmount * baseCutoff;

    // Key tracking: scale cutoff by note distance from middle C
    float keyTrackMod = filterKeyTracking * (static_cast<float> (currentNote) - 60.0f) * 50.0f;

    // Velocity-to-filter
    float velMod = filterVelocitySens * currentVelocity * baseCutoff;

    // LFO cutoff modulation (bipolar, scaled to useful range)
    float lfoCutoffMod = lfoFilterMod * baseCutoff * 0.5f;
    float modEnvCutoffAmount = envCutoffMod * baseCutoff * modRanges.cutoff;

    float matrixCutoffMod = matrixMod.filterCutoff * baseCutoff * modRanges.cutoff;
    float modulatedCutoff = baseCutoff + envMod + keyTrackMod + velMod + lfoCutoffMod
                          + modEnvCutoffAmount + driftCutoffHz + matrixCutoffMod;
    modulatedCutoff = juce::jlimit (20.0f, 20000.0f, modulatedCutoff);

    float matrixResMod = matrixMod.filterResonance * modRanges.resonance;
    float modulatedResonance = juce::jlimit (0.0f, 1.0f, baseResonance + envResonanceMod * modRanges.resonance + matrixResMod);

    osc1Filter.setCutoff (modulatedCutoff);
    osc2Filter.setCutoff (modulatedCutoff);
    noiseFilter.setCutoff (modulatedCutoff);
    osc1Filter.setResonance (modulatedResonance);
    osc2Filter.setResonance (modulatedResonance);
    noiseFilter.setResonance (modulatedResonance);

    switch (filterTarget)
    {
        case FilterTarget::Osc1:
            osc1Signal = osc1Filter.processSample (osc1Signal);
            break;
        case FilterTarget::Osc2:
            osc2Signal = osc2Filter.processSample (osc2Signal);
            break;
        case FilterTarget::BothOscillators:
            osc1Signal = osc1Filter.processSample (osc1Signal);
            osc2Signal = osc2Filter.processSample (osc2Signal);
            break;
        case FilterTarget::Noise:
            noiseSignal = noiseFilter.processSample (noiseSignal);
            break;
        case FilterTarget::All:
            osc1Signal = osc1Filter.processSample (osc1Signal);
            osc2Signal = osc2Filter.processSample (osc2Signal);
            noiseSignal = noiseFilter.processSample (noiseSignal);
            break;
    }

    float mixed = osc1Signal + osc2Signal + noiseSignal;

    // Volume LFO + matrix modulation
    float volumeScale = 1.0f + (lfoVolumeMod + envVolumeMod + matrixMod.volume) * modRanges.volume;
    volumeScale = juce::jlimit (0.0f, 2.0f, volumeScale);
    float output = mixed * volumeScale;

    // Soft-clip to tame volume spikes from heavy modulation or resonance
    output = std::tanh (output);

    if (retriggeredWhileActive)
    {
        // Force continuity with previous sample, then decay the correction.
        clickSuppressOffset = lastOutputSample - output;
        retriggeredWhileActive = false;
    }

    if (std::abs (clickSuppressOffset) > 1.0e-6f)
    {
        output += clickSuppressOffset;
        clickSuppressOffset *= clickSuppressDecay;
    }
    else
    {
        clickSuppressOffset = 0.0f;
    }

    lastOutputSample = output;

    return output;
}

bool SynthVoice::isActive() const
{
    return ampEnv1.isActive() || ampEnv2.isActive() || noiseEnv.isActive();
}

void SynthVoice::setOsc1Params (Waveform wf, int octave, int semitone, float fineTuneCents,
                                 float level01, float pw01, int unisonVoices,
                                 float unisonDetune, float unisonBlend)
{
    osc1Octave = octave;
    osc1Semitone = semitone;
    osc1FineTune = fineTuneCents;
    osc1PulseWidth = pw01;
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
    osc2PulseWidth = pw01;
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
    // Cap noise generator gain so full knob equals previous 50%.
    noiseOsc.setLevel (juce::jlimit (0.0f, 1.0f, level01) * 0.5f);
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

void SynthVoice::setFilterParams (FilterMode mode, FilterModel model, FilterSlope slope,
                                   float cutoffHz, float resonance01,
                                   float envAmount, float keyTracking01, float velocity01,
                                   FilterTarget target)
{
    for (auto* filter : { &osc1Filter, &osc2Filter, &noiseFilter })
    {
        filter->setMode (mode);
        filter->setModel (model);
        filter->setSlope (slope);
        filter->setResonance (resonance01);
    }

    baseCutoff = cutoffHz;
    baseResonance = resonance01;
    filterEnvAmount = envAmount;
    filterKeyTracking = keyTracking01;
    filterVelocitySens = velocity01;
    filterTarget = target;
}

void SynthVoice::setModEnv1Params (float attackMs, float decayMs, float sustain01, float releaseMs,
                                   float amount01, bool destCutoff, bool destResonance,
                                   bool destPitch, bool destVolume)
{
    modEnv1.setParameters (attackMs, decayMs, sustain01, releaseMs);
    modEnv1Amount = amount01;
    modEnv1DestCutoff = destCutoff;
    modEnv1DestResonance = destResonance;
    modEnv1DestPitch = destPitch;
    modEnv1DestVolume = destVolume;
}

void SynthVoice::setModEnv2Params (float attackMs, float decayMs, float sustain01, float releaseMs,
                                   float amount01, bool destCutoff, bool destResonance,
                                   bool destPitch, bool destVolume)
{
    modEnv2.setParameters (attackMs, decayMs, sustain01, releaseMs);
    modEnv2Amount = amount01;
    modEnv2DestCutoff = destCutoff;
    modEnv2DestResonance = destResonance;
    modEnv2DestPitch = destPitch;
    modEnv2DestVolume = destVolume;
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

    float totalCents = fineCents + driftCents + pitchMod * modRanges.pitch * 100.0f;

    float midiNote = totalSemitones + totalCents / 100.0f;

    return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
}

} // namespace tillysynth
