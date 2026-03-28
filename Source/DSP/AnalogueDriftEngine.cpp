#include "AnalogueDriftEngine.h"
#include <cmath>

namespace tillysynth
{

AnalogueDriftEngine::AnalogueDriftEngine()
{
    // Randomise drift seeds so no two instances sound identical
    for (auto& seed : driftSeeds)
        seed = random.nextFloat() * 100.0f;

    for (size_t i = 0; i < static_cast<size_t> (kMaxVoices); ++i)
    {
        pitchDrifts[i].store (0.0f);
        cutoffDrifts[i].store (0.0f);
    }
}

AnalogueDriftEngine::~AnalogueDriftEngine()
{
    stopTimer();
    sensorReader.stop();
}

void AnalogueDriftEngine::start (float amount01)
{
    driftAmount.store (amount01);
    sensorReader.start();

    // Irregular polling interval adds organic quality
    startTimer (150 + random.nextInt (100));
}

void AnalogueDriftEngine::stop()
{
    stopTimer();
    sensorReader.stop();

    for (size_t i = 0; i < static_cast<size_t> (kMaxVoices); ++i)
    {
        pitchDrifts[i].store (0.0f);
        cutoffDrifts[i].store (0.0f);
    }
}

void AnalogueDriftEngine::setDriftAmount (float amount01)
{
    driftAmount.store (amount01);
}

float AnalogueDriftEngine::getPitchDriftCents (int voiceIndex) const
{
    if (voiceIndex < 0 || voiceIndex >= kMaxVoices)
        return 0.0f;

    return pitchDrifts[static_cast<size_t> (voiceIndex)].load();
}

float AnalogueDriftEngine::getCutoffDriftHz (int voiceIndex) const
{
    if (voiceIndex < 0 || voiceIndex >= kMaxVoices)
        return 0.0f;

    return cutoffDrifts[static_cast<size_t> (voiceIndex)].load();
}

void AnalogueDriftEngine::timerCallback()
{
    auto sensorData = sensorReader.read();
    updateDriftValues (sensorData);

    // Vary the next callback interval for organic feel
    startTimer (120 + random.nextInt (160));
}

void AnalogueDriftEngine::updateDriftValues (const DriftSensorData& sensorData)
{
    // Cache sensor state for UI display
    lastCpuLoadAvailable.store (sensorData.cpuLoadAvailable);
    lastThermalAvailable.store (sensorData.thermalAvailable);
    lastBatteryAvailable.store (sensorData.batteryAvailable);
    lastCpuLoad.store (sensorData.cpuLoad);
    lastThermalPressure.store (sensorData.thermalPressure);
    lastBatteryDrainRate.store (sensorData.batteryDrainRate);

    float amount = driftAmount.load();

    // CPU load provides fast, variable drift (like component-level instability)
    // Thermal pressure provides slow drift (like warming capacitors)
    // Battery drain provides very slow drift (thermal correlation on laptops)
    float cpuSignal = sensorData.cpuLoadAvailable ? sensorData.cpuLoad : 0.0f;
    float thermalSignal = sensorData.thermalAvailable ? sensorData.thermalPressure : 0.0f;
    float batterySignal = sensorData.batteryAvailable ? sensorData.batteryDrainRate : 0.0f;

    // Combine available signals; always have some base drift via PRNG
    bool hasSensors = sensorData.cpuLoadAvailable || sensorData.thermalAvailable
                   || sensorData.batteryAvailable;

    // Environmental signal: weighted blend of available sources
    float envSignal = hasSensors
        ? (cpuSignal * 0.5f + thermalSignal * 0.3f + batterySignal * 0.2f)
        : 0.5f;

    for (size_t i = 0; i < static_cast<size_t> (kMaxVoices); ++i)
    {
        // PRNG component ensures each voice drifts uniquely
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Per-voice seed evolves slowly, modulated by the environmental signal
        float seedPhase = std::sin (driftSeeds[i] + envSignal * 3.0f);

        // CPU load creates faster pitch wobble (like oscillator instability under heat)
        float cpuComponent = cpuSignal * std::sin (driftSeeds[i] * 7.3f);

        // Thermal creates slow cutoff wander (like drifting capacitor values)
        float thermalComponent = thermalSignal * std::cos (driftSeeds[i] * 2.1f);

        float pitchDrift = (noise * 0.3f + seedPhase * 0.3f + cpuComponent * 0.4f)
                         * kMaxPitchDriftCents * amount;

        float cutoffDrift = (noise * 0.2f + seedPhase * 0.3f + thermalComponent * 0.5f)
                          * kMaxCutoffDriftHz * amount;

        pitchDrifts[i].store (pitchDrift);
        cutoffDrifts[i].store (cutoffDrift);

        // Slowly evolve the seeds — rate influenced by CPU load for liveliness
        driftSeeds[i] += 0.01f + cpuSignal * 0.02f;
    }
}

} // namespace tillysynth
