#pragma once
#include <memory>

namespace tillysynth
{

struct DriftSensorData
{
    // Accelerometer magnitude delta from gravity baseline (0 = still, higher = movement)
    float motionIntensity = 0.0f;

    // Battery discharge rate normalised to 0–1 (0 = charging/full, 1 = draining fast)
    float batteryDrainRate = 0.0f;

    // Whether real sensor data was available
    bool motionAvailable = false;
    bool batteryAvailable = false;
};

class DriftSensorReader
{
public:
    DriftSensorReader();
    ~DriftSensorReader();

    void start();
    void stop();

    DriftSensorData read();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace tillysynth
