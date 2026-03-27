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
    float amount = driftAmount.load();

    // Blend motion and battery into a combined environmental signal.
    // Motion (gyro/accelerometer) provides fast, transient variation.
    // Battery drain rate provides slow, thermal-correlated drift.
    float motionSignal = sensorData.motionAvailable ? sensorData.motionIntensity : 0.0f;
    float batterySignal = sensorData.batteryAvailable ? sensorData.batteryDrainRate : 0.0f;

    // If neither sensor is available, fall back to pure PRNG
    bool hasSensors = sensorData.motionAvailable || sensorData.batteryAvailable;
    float envSignal = hasSensors ? (motionSignal * 0.6f + batterySignal * 0.4f) : 0.5f;

    for (size_t i = 0; i < static_cast<size_t> (kMaxVoices); ++i)
    {
        // PRNG component ensures each voice drifts uniquely
        float noise = random.nextFloat() * 2.0f - 1.0f;

        // Per-voice seed evolves slowly, modulated by the environmental signal
        float seedPhase = std::sin (driftSeeds[i] + envSignal * 3.0f);

        // Motion creates faster pitch wobble; battery creates slower cutoff wander
        float motionComponent = motionSignal * std::sin (driftSeeds[i] * 7.3f);
        float batteryComponent = batterySignal * std::cos (driftSeeds[i] * 2.1f);

        float pitchDrift = (noise * 0.3f + seedPhase * 0.3f + motionComponent * 0.4f)
                         * kMaxPitchDriftCents * amount;

        float cutoffDrift = (noise * 0.2f + seedPhase * 0.3f + batteryComponent * 0.5f)
                          * kMaxCutoffDriftHz * amount;

        pitchDrifts[i].store (pitchDrift);
        cutoffDrifts[i].store (cutoffDrift);

        // Slowly evolve the seeds — rate influenced by motion for liveliness
        driftSeeds[i] += 0.01f + motionSignal * 0.02f;
    }
}

} // namespace tillysynth
