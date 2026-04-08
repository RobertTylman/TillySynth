#pragma once
#include <juce_events/juce_events.h>
#include "DriftSensorReader.h"
#include <array>
#include <atomic>

namespace tillysynth
{

class AnalogueDriftEngine : private juce::Timer
{
public:
    static constexpr int kMaxVoices = 16;
    static constexpr float kMaxPitchDriftCents = 22.5f;
    static constexpr float kMaxCutoffDriftHz = 12.0f;

    AnalogueDriftEngine();
    ~AnalogueDriftEngine() override;

    void start (float driftAmount01);
    void stop();

    void setDriftAmount (float amount01);

    float getPitchDriftCents (int voiceIndex) const;
    float getCutoffDriftHz (int voiceIndex) const;

    // Sensor state for UI display
    bool isCpuLoadAvailable() const { return lastCpuLoadAvailable.load(); }
    bool isThermalAvailable() const { return lastThermalAvailable.load(); }
    bool isBatteryAvailable() const { return lastBatteryAvailable.load(); }
    float getCpuLoad() const { return lastCpuLoad.load(); }
    float getThermalPressure() const { return lastThermalPressure.load(); }
    float getBatteryDrainRate() const { return lastBatteryDrainRate.load(); }

private:
    void timerCallback() override;
    void updateDriftValues (const DriftSensorData& sensorData);

    std::atomic<float> driftAmount { 0.0f };

    // Atomic arrays for lock-free audio thread reading
    std::array<std::atomic<float>, kMaxVoices> pitchDrifts {};
    std::array<std::atomic<float>, kMaxVoices> cutoffDrifts {};

    // Per-voice random seeds for unique drift character
    std::array<float, kMaxVoices> driftSeeds {};

    // Cached sensor state for UI access
    std::atomic<bool> lastCpuLoadAvailable { false };
    std::atomic<bool> lastThermalAvailable { false };
    std::atomic<bool> lastBatteryAvailable { false };
    std::atomic<float> lastCpuLoad { 0.0f };
    std::atomic<float> lastThermalPressure { 0.0f };
    std::atomic<float> lastBatteryDrainRate { 0.0f };

    DriftSensorReader sensorReader;
    juce::Random random;
};

} // namespace tillysynth
