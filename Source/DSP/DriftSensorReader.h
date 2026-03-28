#pragma once
#include <juce_core/juce_core.h>
#include <memory>

namespace tillysynth
{

struct DriftSensorData
{
    // System CPU load normalised 0–1 (provides fast, variable drift signal)
    float cpuLoad = 0.0f;

    // Thermal pressure normalised 0–1 (slow drift correlated with system heat)
    float thermalPressure = 0.0f;

    // Battery discharge rate normalised 0–1 (0 = charging/full, 1 = draining fast)
    float batteryDrainRate = 0.0f;

    // Whether real sensor data was available
    bool cpuLoadAvailable = false;
    bool thermalAvailable = false;
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
