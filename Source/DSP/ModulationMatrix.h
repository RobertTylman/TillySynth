#pragma once
#include <array>

namespace tillysynth
{

// ============================================================
//  Modulation sources & destinations
// ============================================================

enum class ModSource
{
    None = 0,
    LFO1,
    LFO2,
    ModEnv1,
    ModEnv2,
    Velocity,
    Aftertouch,
    ModWheel,
    Count
};

enum class ModDest
{
    None = 0,
    FilterCutoff,
    FilterResonance,
    Pitch,
    Volume,
    PulseWidth,
    Osc1Level,
    Osc2Level,
    NoiseLevel,
    LFO1Rate,
    LFO2Rate,
    Count
};

// ============================================================
//  Per-slot configuration (read from APVTS once per block)
// ============================================================

struct ModSlot
{
    ModSource source = ModSource::None;
    ModDest   dest   = ModDest::None;
    float     amount = 0.0f;   // bipolar -1..+1
};

static constexpr int kModSlotCount = 8;

// ============================================================
//  Aggregated modulation output — one float per destination
// ============================================================

struct ModulationOutput
{
    float filterCutoff    = 0.0f;
    float filterResonance = 0.0f;
    float pitch           = 0.0f;
    float volume          = 0.0f;
    float pulseWidth      = 0.0f;
    float osc1Level       = 0.0f;
    float osc2Level       = 0.0f;
    float noiseLevel      = 0.0f;
    float lfo1Rate        = 0.0f;
    float lfo2Rate        = 0.0f;

    float& getDestRef (ModDest d)
    {
        switch (d)
        {
            case ModDest::FilterCutoff:    return filterCutoff;
            case ModDest::FilterResonance: return filterResonance;
            case ModDest::Pitch:           return pitch;
            case ModDest::Volume:          return volume;
            case ModDest::PulseWidth:      return pulseWidth;
            case ModDest::Osc1Level:       return osc1Level;
            case ModDest::Osc2Level:       return osc2Level;
            case ModDest::NoiseLevel:      return noiseLevel;
            case ModDest::LFO1Rate:        return lfo1Rate;
            case ModDest::LFO2Rate:        return lfo2Rate;
            case ModDest::None:
            case ModDest::Count:
            default:                       return filterCutoff; // unused fallback
        }
    }
};

// ============================================================
//  Source values snapshot — filled once per sample (global)
//  or per voice (velocity, mod envs)
// ============================================================

struct ModSourceValues
{
    float lfo1      = 0.0f;   // raw -1..+1
    float lfo2      = 0.0f;
    float modEnv1   = 0.0f;   // 0..1 (envelope output, pre-amount)
    float modEnv2   = 0.0f;
    float velocity  = 0.0f;   // 0..1
    float aftertouch = 0.0f;  // 0..1
    float modWheel  = 0.0f;   // 0..1

    float getSourceValue (ModSource s) const
    {
        switch (s)
        {
            case ModSource::LFO1:       return lfo1;
            case ModSource::LFO2:       return lfo2;
            case ModSource::ModEnv1:    return modEnv1;
            case ModSource::ModEnv2:    return modEnv2;
            case ModSource::Velocity:   return velocity;
            case ModSource::Aftertouch: return aftertouch;
            case ModSource::ModWheel:   return modWheel;
            case ModSource::None:
            case ModSource::Count:
            default:                    return 0.0f;
        }
    }
};

// ============================================================
//  ModulationMatrix — holds slot configs, computes output
// ============================================================

class ModulationMatrix
{
public:
    void setSlot (int index, ModSource source, ModDest dest, float amount)
    {
        if (index >= 0 && index < kModSlotCount)
        {
            slots[static_cast<size_t> (index)].source = source;
            slots[static_cast<size_t> (index)].dest   = dest;
            slots[static_cast<size_t> (index)].amount  = amount;
        }
    }

    ModulationOutput compute (const ModSourceValues& sources) const
    {
        ModulationOutput output;

        for (const auto& slot : slots)
        {
            if (slot.source == ModSource::None || slot.dest == ModDest::None)
                continue;

            float value = sources.getSourceValue (slot.source) * slot.amount;
            output.getDestRef (slot.dest) += value;
        }

        return output;
    }

    const ModSlot& getSlot (int index) const
    {
        return slots[static_cast<size_t> (index)];
    }

private:
    std::array<ModSlot, kModSlotCount> slots;
};

} // namespace tillysynth
