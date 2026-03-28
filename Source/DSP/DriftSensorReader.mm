#include "DriftSensorReader.h"

#if JUCE_MAC
    #import <Foundation/Foundation.h>
    #import <IOKit/ps/IOPowerSources.h>
    #import <IOKit/ps/IOPSKeys.h>
    #include <mach/mach.h>
    #include <cmath>
#endif

namespace tillysynth
{

#if JUCE_MAC

struct DriftSensorReader::Impl
{
    bool running = false;

    // CPU load tracking
    uint64_t prevUserTicks = 0;
    uint64_t prevSystemTicks = 0;
    uint64_t prevIdleTicks = 0;
    uint64_t prevNiceTicks = 0;
    float smoothedCpuLoad = 0.0f;

    // Thermal tracking
    float smoothedThermal = 0.0f;

    // Battery tracking
    float lastBatteryLevel = -1.0f;
    float smoothedDrainRate = 0.0f;
    uint64_t lastBatteryReadTime = 0;

    float readCpuLoad()
    {
        host_cpu_load_info_data_t cpuInfo;
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

        kern_return_t result = host_statistics (mach_host_self(), HOST_CPU_LOAD_INFO,
                                                 reinterpret_cast<host_info_t> (&cpuInfo), &count);

        if (result != KERN_SUCCESS)
            return smoothedCpuLoad;

        uint64_t user   = cpuInfo.cpu_ticks[CPU_STATE_USER];
        uint64_t system = cpuInfo.cpu_ticks[CPU_STATE_SYSTEM];
        uint64_t idle   = cpuInfo.cpu_ticks[CPU_STATE_IDLE];
        uint64_t nice   = cpuInfo.cpu_ticks[CPU_STATE_NICE];

        uint64_t totalDelta = (user - prevUserTicks) + (system - prevSystemTicks)
                            + (idle - prevIdleTicks) + (nice - prevNiceTicks);

        float load = 0.0f;
        if (totalDelta > 0 && prevUserTicks > 0)
        {
            uint64_t activeDelta = (user - prevUserTicks) + (system - prevSystemTicks)
                                 + (nice - prevNiceTicks);
            load = static_cast<float> (activeDelta) / static_cast<float> (totalDelta);
        }

        prevUserTicks   = user;
        prevSystemTicks = system;
        prevIdleTicks   = idle;
        prevNiceTicks   = nice;

        // Smooth for organic feel
        smoothedCpuLoad = smoothedCpuLoad * 0.8f + load * 0.2f;
        return smoothedCpuLoad;
    }

    float readThermalPressure()
    {
        // NSProcessInfo.thermalState available on macOS 10.10.3+
        NSProcessInfoThermalState state = [[NSProcessInfo processInfo] thermalState];

        float pressure = 0.0f;
        switch (state)
        {
            case NSProcessInfoThermalStateNominal:  pressure = 0.1f;  break;
            case NSProcessInfoThermalStateFair:      pressure = 0.4f;  break;
            case NSProcessInfoThermalStateSerious:   pressure = 0.7f;  break;
            case NSProcessInfoThermalStateCritical:  pressure = 1.0f;  break;
        }

        smoothedThermal = smoothedThermal * 0.9f + pressure * 0.1f;
        return smoothedThermal;
    }

    float readBatteryDrainRate()
    {
        CFTypeRef info = IOPSCopyPowerSourcesInfo();
        if (info == nullptr)
            return smoothedDrainRate;

        CFArrayRef sources = IOPSCopyPowerSourcesList (info);
        if (sources == nullptr)
        {
            CFRelease (info);
            return smoothedDrainRate;
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
                    float rawRate = (lastBatteryLevel - currentLevel) / elapsed;
                    float normRate = std::abs (rawRate) * 5000.0f;
                    normRate = std::min (normRate, 1.0f);

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

// Non-macOS fallback
struct DriftSensorReader::Impl
{
    bool running = false;
    float readCpuLoad() { return 0.0f; }
    float readThermalPressure() { return 0.0f; }
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
    pimpl->running = true;
}

void DriftSensorReader::stop()
{
    pimpl->running = false;
}

DriftSensorData DriftSensorReader::read()
{
    DriftSensorData data;

    if (! pimpl->running)
        return data;

    data.cpuLoad = pimpl->readCpuLoad();
    data.thermalPressure = pimpl->readThermalPressure();
    data.batteryDrainRate = pimpl->readBatteryDrainRate();

#if JUCE_MAC
    // CPU load and thermal are always available on macOS
    data.cpuLoadAvailable = true;
    data.thermalAvailable = true;
    data.batteryAvailable = (pimpl->lastBatteryLevel >= 0.0f);
#endif

    return data;
}

} // namespace tillysynth
