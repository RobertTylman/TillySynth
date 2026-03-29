#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/VoiceManager.h"
#include "DSP/ChorusEngine.h"
#include "PresetManager.h"
#include <atomic>
#include <array>

namespace tillysynth
{

class TillySynthProcessor : public juce::AudioProcessor
{
public:
    TillySynthProcessor();
    ~TillySynthProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TillySynth"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    void setTranspose (int semitones) { transposeSemitones = juce::jlimit (-12, 12, semitones); }
    int getTranspose() const { return transposeSemitones; }
    PresetManager& getPresetManager() { return presetManager; }

    // Access drift engine for sensor state queries
    AnalogueDriftEngine& getDriftEngine() { return voiceManager.getDriftEngine(); }

    // Drift values exposed for UI visualisation (atomic for thread-safe reading)
    std::array<std::atomic<float>, 16> driftVisPitch {};
    std::array<std::atomic<float>, 16> driftVisCutoff {};

    // Output level for VU meter (atomic for thread-safe UI reading)
    std::atomic<float> outputLevelLeft  { 0.0f };
    std::atomic<float> outputLevelRight { 0.0f };

    // Audio scope buffer for oscilloscope display
    static constexpr int kScopeBufferSize = 512;
    std::array<std::atomic<float>, kScopeBufferSize> scopeBuffer {};
    std::atomic<int> scopeWritePos { 0 };

    // LFO state for UI visualisation
    std::atomic<float> lfo1Phase { 0.0f };
    std::atomic<float> lfo2Phase { 0.0f };
    std::atomic<int> lfo1Waveform { 0 };
    std::atomic<int> lfo2Waveform { 0 };
    std::atomic<float> lfo1Rate { 1.0f };
    std::atomic<float> lfo2Rate { 1.0f };

    // Pitch bend and mod wheel state for UI indicators (normalised -1..1 and 0..1)
    std::atomic<float> pitchBendUI { 0.0f };
    std::atomic<float> modWheelUI { 0.0f };

private:
    void updateParametersFromAPVTS();
    void handleMidiMessage (const juce::MidiMessage& msg);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    VoiceManager voiceManager;
    ChorusEngine chorus;
    juce::dsp::Reverb reverb;

    PresetManager presetManager;
    juce::SmoothedValue<float> masterVolume;
    int transposeSemitones = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TillySynthProcessor)
};

} // namespace tillysynth
