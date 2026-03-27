#include "DriftSensorReader.h"

#if JUCE_MAC
    #import <CoreMotion/CoreMotion.h>
    #import <IOKit/ps/IOPowerSources.h>
    #import <IOKit/ps/IOPSKeys.h>
    #include <cmath>
#endif

namespace tillysynth
{

#if JUCE_MAC

struct DriftSensorReader::Impl
{
    CMMotionManager* motionManager = nil;
    NSOperationQueue* motionQueue = nil;

    bool motionRunning = false;

    // Smoothed accelerometer values
    float lastAccelMagnitude = 1.0f;  // gravity = ~1.0
    float smoothedMotionIntensity = 0.0f;

    // Battery tracking
    float lastBatteryLevel = -1.0f;
    float smoothedDrainRate = 0.0f;
    uint64_t lastBatteryReadTime = 0;

    void startMotion()
    {
        motionManager = [[CMMotionManager alloc] init];

        if (! motionManager.accelerometerAvailable)
            return;

        motionManager.accelerometerUpdateInterval = 0.1;  // 10 Hz
        motionQueue = [[NSOperationQueue alloc] init];
        motionQueue.maxConcurrentOperationCount = 1;

        [motionManager startAccelerometerUpdatesToQueue:motionQueue
            withHandler:^(CMAccelerometerData* data, NSError* error)
            {
                if (error != nil || data == nil)
                    return;

                float x = static_cast<float> (data.acceleration.x);
                float y = static_cast<float> (data.acceleration.y);
                float z = static_cast<float> (data.acceleration.z);

                float magnitude = std::sqrt (x * x + y * y + z * z);

                // Delta from gravity baseline captures vibrations and movement
                float delta = std::abs (magnitude - lastAccelMagnitude);
                lastAccelMagnitude = magnitude;

                // Exponential smoothing for organic feel
                smoothedMotionIntensity = smoothedMotionIntensity * 0.85f + delta * 0.15f;
            }];

        motionRunning = true;
    }

    void stopMotion()
    {
        if (motionRunning && motionManager != nil)
        {
            [motionManager stopAccelerometerUpdates];
            motionRunning = false;
        }

        motionManager = nil;
        motionQueue = nil;
    }

    float readMotionIntensity()
    {
        if (! motionRunning)
            return 0.0f;

        // Clamp to useful range: tiny desk vibrations to deliberate shakes
        return std::min (smoothedMotionIntensity * 10.0f, 1.0f);
    }

    float readBatteryDrainRate()
    {
        CFTypeRef info = IOPSCopyPowerSourcesInfo();
        if (info == nullptr)
            return 0.0f;

        CFArrayRef sources = IOPSCopyPowerSourcesList (info);
        if (sources == nullptr)
        {
            CFRelease (info);
            return 0.0f;
        }

        float drainRate = 0.0f;
        bool found = false;

        CFIndex count = CFArrayGetCount (sources);
        for (CFIndex i = 0; i < count; ++i)
        {
            CFDictionaryRef source = IOPSGetPowerSourceDescription (
                info, CFArrayGetValueAtIndex (sources, i));

            if (source == nullptr)
                continue;

            // Read current capacity
            CFNumberRef capacityRef = static_cast<CFNumberRef> (
                CFDictionaryGetValue (source, CFSTR (kIOPSCurrentCapacityKey)));
            CFNumberRef maxCapacityRef = static_cast<CFNumberRef> (
                CFDictionaryGetValue (source, CFSTR (kIOPSMaxCapacityKey)));

            if (capacityRef == nullptr || maxCapacityRef == nullptr)
                continue;

            int capacity = 0, maxCapacity = 0;
            CFNumberGetValue (capacityRef, kCFNumberIntType, &capacity);
            CFNumberGetValue (maxCapacityRef, kCFNumberIntType, &maxCapacity);

            if (maxCapacity <= 0)
                continue;

            float currentLevel = static_cast<float> (capacity) / static_cast<float> (maxCapacity);

            uint64_t now = static_cast<uint64_t> (
                [[NSDate date] timeIntervalSince1970] * 1000.0);

            if (lastBatteryLevel >= 0.0f && lastBatteryReadTime > 0)
            {
                float elapsed = static_cast<float> (now - lastBatteryReadTime) / 1000.0f;

                if (elapsed > 0.5f)
                {
                    // Discharge per second, scaled up for sensitivity
                    float rawRate = (lastBatteryLevel - currentLevel) / elapsed;

                    // Normalise: typical laptop drains ~0.01% per second under load
                    float normRate = std::abs (rawRate) * 5000.0f;
                    normRate = std::min (normRate, 1.0f);

                    // Smooth to avoid jumps
                    smoothedDrainRate = smoothedDrainRate * 0.9f + normRate * 0.1f;
                    drainRate = smoothedDrainRate;
                    found = true;
                }
            }

            lastBatteryLevel = currentLevel;
            lastBatteryReadTime = now;
            break;
        }

        CFRelease (sources);
        CFRelease (info);

        return found ? drainRate : smoothedDrainRate;
    }
};

#else

// Non-macOS fallback: no real sensors
struct DriftSensorReader::Impl
{
    void startMotion() {}
    void stopMotion() {}
    float readMotionIntensity() { return 0.0f; }
    float readBatteryDrainRate() { return 0.0f; }
};

#endif

DriftSensorReader::DriftSensorReader()
    : pimpl (std::make_unique<Impl>())
{
}

DriftSensorReader::~DriftSensorReader() = default;

void DriftSensorReader::start()
{
    pimpl->startMotion();
}

void DriftSensorReader::stop()
{
    pimpl->stopMotion();
}

DriftSensorData DriftSensorReader::read()
{
    DriftSensorData data;

    data.motionIntensity = pimpl->readMotionIntensity();
    data.batteryDrainRate = pimpl->readBatteryDrainRate();

#if JUCE_MAC
    data.motionAvailable = pimpl->motionRunning;
    data.batteryAvailable = (pimpl->lastBatteryLevel >= 0.0f);
#endif

    return data;
}

} // namespace tillysynth
