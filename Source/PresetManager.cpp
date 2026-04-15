#include "PresetManager.h"
#include <cmath>

namespace tillysynth
{

static void setParamValue (std::vector<std::pair<juce::String, float>>& params,
                           const juce::String& id,
                           float value)
{
    for (auto& p : params)
    {
        if (p.first == id)
        {
            p.second = value;
            return;
        }
    }

    params.push_back ({ id, value });
}

static float getParamValue (const std::vector<std::pair<juce::String, float>>& params,
                            const juce::String& id,
                            float fallback = 0.0f)
{
    for (const auto& p : params)
        if (p.first == id)
            return p.second;

    return fallback;
}

static void setModSlot (std::vector<std::pair<juce::String, float>>& params,
                        int slot,
                        int source,
                        int dest,
                        float amountPercent)
{
    const auto slotStr = juce::String (slot);
    setParamValue (params, "modmatrix_" + slotStr + "_source", static_cast<float> (source));
    setParamValue (params, "modmatrix_" + slotStr + "_dest", static_cast<float> (dest));
    setParamValue (params, "modmatrix_" + slotStr + "_amount", amountPercent);
}

static void applyPresetSpecificModMatrixProfile (const juce::String& name,
                                                 const juce::String& category,
                                                 std::vector<std::pair<juce::String, float>>& params)
{
    enum ModSourceIds
    {
        srcNone = 0, srcLfo1 = 1, srcLfo2 = 2, srcLfo3 = 3,
        srcModEnv1 = 4, srcModEnv2 = 5, srcVelocity = 6,
        srcAftertouch = 7, srcModWheel = 8
    };

    enum ModDestIds
    {
        dstNone = 0, dstCutoff = 1, dstResonance = 2, dstPitch = 3, dstVolume = 4,
        dstPulseWidth = 5, dstOsc1Level = 6, dstOsc2Level = 7, dstNoiseLevel = 8,
        dstLfo1Rate = 9, dstLfo2Rate = 10, dstLfo3Rate = 11
    };

    juce::String key = category + "|" + name;
    const uint32_t hash = static_cast<uint32_t> (key.hashCode());
    auto jitter = [hash] (int salt, float span)
    {
        uint32_t mixed = hash ^ static_cast<uint32_t> (0x9e3779b9u * static_cast<uint32_t> (salt + 1));
        float unit = static_cast<float> (mixed & 0xffffu) / 65535.0f; // 0..1
        return (unit * 2.0f - 1.0f) * span;
    };

    auto amount = [&jitter] (float base, int salt, float span)
    {
        return juce::jlimit (-100.0f, 100.0f, base + jitter (salt, span));
    };

    auto ranged = [&jitter] (float base, float lo, float hi, int salt, float span)
    {
        return juce::jlimit (lo, hi, base + jitter (salt, span));
    };

    auto hasToken = [&name] (std::initializer_list<const char*> tokens)
    {
        for (const auto* token : tokens)
            if (name.containsIgnoreCase (token))
                return true;
        return false;
    };

    for (int i = 1; i <= 8; ++i)
        setModSlot (params, i, srcNone, dstNone, 0.0f);

    const float noiseLevel = getParamValue (params, "noise_level", 0.0f);
    const float filterRes = getParamValue (params, "filter_resonance", 20.0f);
    const float osc2Level = getParamValue (params, "osc2_level", 50.0f);

    setParamValue (params, "modrange_cutoff", ranged (95.0f, 40.0f, 220.0f, 1, 12.0f));
    setParamValue (params, "modrange_resonance", ranged (55.0f, 20.0f, 95.0f, 2, 10.0f));
    setParamValue (params, "modrange_pitch", ranged (2.0f, 0.5f, 6.0f, 3, 0.8f));
    setParamValue (params, "modrange_volume", ranged (35.0f, 10.0f, 80.0f, 4, 8.0f));
    setParamValue (params, "modrange_pw", ranged (35.0f, 10.0f, 80.0f, 5, 7.0f));
    setParamValue (params, "modrange_osc1_level", ranged (95.0f, 40.0f, 200.0f, 6, 12.0f));
    setParamValue (params, "modrange_osc2_level", ranged (95.0f, 40.0f, 200.0f, 7, 12.0f));
    setParamValue (params, "modrange_noise_level", ranged (100.0f, 40.0f, 200.0f, 8, 12.0f));
    setParamValue (params, "modrange_lfo1_rate", ranged (12.0f, 2.0f, 30.0f, 9, 4.0f));
    setParamValue (params, "modrange_lfo2_rate", ranged (12.0f, 2.0f, 30.0f, 10, 4.0f));
    setParamValue (params, "modrange_lfo3_rate", ranged (12.0f, 2.0f, 30.0f, 11, 4.0f));

    if (category.equalsIgnoreCase ("Pads"))
    {
        const int variant = static_cast<int> ((hash >> 3) & 0x3u);

        setModSlot (params, 1, srcVelocity, dstCutoff, amount (20.0f, 12, 6.0f));
        setModSlot (params, 2, srcModWheel, dstCutoff, amount (28.0f, 13, 7.0f));
        setModSlot (params, 3, srcAftertouch, dstVolume, amount (10.0f, 14, 4.0f));
        setModSlot (params, 4, srcLfo3, dstCutoff, amount (12.0f, 15, 5.0f));
        setModSlot (params, 5, srcModEnv1, dstCutoff, amount (10.0f, 16, 4.0f));
        setParamValue (params, "lfo3_rate", ranged (0.18f, 0.05f, 0.8f, 17, 0.08f));

        switch (variant)
        {
            case 0:
                setModSlot (params, 6, srcLfo1, dstCutoff, amount (14.0f, 18, 5.0f));
                setModSlot (params, 7, srcAftertouch, dstVolume, amount (9.0f, 19, 4.0f));
                setModSlot (params, 8, srcModWheel, dstLfo3Rate, amount (10.0f, 20, 4.0f));
                break;
            case 1:
                setModSlot (params, 6, srcModEnv2, dstCutoff, amount (11.0f, 21, 4.0f));
                setModSlot (params, 7, srcVelocity, dstVolume, amount (11.0f, 22, 4.0f));
                setModSlot (params, 8, srcModWheel, dstLfo1Rate, amount (12.0f, 23, 5.0f));
                break;
            case 2:
                setModSlot (params, 6, srcLfo1, dstPulseWidth, amount (16.0f, 24, 5.0f));
                setModSlot (params, 7, srcAftertouch, dstCutoff, amount (9.0f, 25, 4.0f));
                setModSlot (params, 8, srcModWheel, dstResonance, amount (8.0f, 26, 4.0f));
                break;
            default:
                setModSlot (params, 6, srcLfo3, dstVolume, amount (9.0f, 27, 4.0f));
                setModSlot (params, 7, srcModEnv1, dstResonance, amount (10.0f, 28, 4.0f));
                setModSlot (params, 8, srcModWheel, dstCutoff, amount (18.0f, 29, 6.0f));
                break;
        }

        if (hasToken ({ "drift", "wash", "sweep", "wind", "ocean", "space", "cosmic", "nebula", "glacial", "frozen" }))
            setModSlot (params, 6, srcLfo1, dstCutoff, amount (18.0f, 30, 6.0f));

        if (hasToken ({ "shimmer", "crystal", "glassy", "neon", "prism", "star", "sunrise" }))
            setModSlot (params, 7, srcVelocity, dstVolume, amount (12.0f, 31, 4.0f));

        if (noiseLevel > 7.0f)
            setModSlot (params, 8, srcVelocity, dstNoiseLevel, amount (10.0f, 32, 5.0f));

        setParamValue (params, "modrange_cutoff", ranged (110.0f, 60.0f, 180.0f, 33, 12.0f));
        setParamValue (params, "modrange_resonance", ranged (45.0f, 20.0f, 70.0f, 34, 7.0f));
        setParamValue (params, "modrange_pitch", ranged (1.4f, 0.6f, 3.0f, 35, 0.4f));
        setParamValue (params, "modrange_volume", ranged (34.0f, 15.0f, 60.0f, 36, 6.0f));
        return;
    }

    if (category.equalsIgnoreCase ("Leads"))
    {
        const int variant = static_cast<int> ((hash >> 7) & 0x3u);

        setModSlot (params, 1, srcVelocity, dstCutoff, amount (34.0f, 40, 8.0f));
        setModSlot (params, 2, srcModWheel, dstPitch, amount (24.0f, 41, 7.0f));
        setModSlot (params, 3, srcAftertouch, dstCutoff, amount (16.0f, 42, 6.0f));
        setModSlot (params, 4, srcAftertouch, dstResonance, amount (10.0f, 43, 5.0f));
        setModSlot (params, 5, srcModEnv1, dstCutoff, amount (14.0f, 44, 5.0f));

        switch (variant)
        {
            case 0:
                setModSlot (params, 6, srcVelocity, dstResonance, amount (16.0f, 45, 6.0f));
                setModSlot (params, 7, srcLfo1, dstPitch, amount (12.0f, 46, 5.0f));
                setModSlot (params, 8, srcModWheel, dstLfo1Rate, amount (15.0f, 47, 6.0f));
                break;
            case 1:
                setModSlot (params, 6, srcModEnv2, dstCutoff, amount (12.0f, 48, 5.0f));
                setModSlot (params, 7, srcAftertouch, dstPitch, amount (10.0f, 49, 5.0f));
                setModSlot (params, 8, srcModWheel, dstResonance, amount (11.0f, 50, 4.0f));
                break;
            case 2:
                setModSlot (params, 6, srcVelocity, dstOsc2Level, amount (12.0f, 51, 5.0f));
                setModSlot (params, 7, srcLfo3, dstCutoff, amount (14.0f, 52, 6.0f));
                setModSlot (params, 8, srcModWheel, dstPitch, amount (10.0f, 53, 4.0f));
                break;
            default:
                setModSlot (params, 6, srcAftertouch, dstResonance, amount (14.0f, 54, 5.0f));
                setModSlot (params, 7, srcModEnv1, dstPitch, amount (9.0f, 55, 4.0f));
                setModSlot (params, 8, srcModWheel, dstLfo3Rate, amount (14.0f, 56, 5.0f));
                break;
        }

        if (hasToken ({ "acid", "squelch", "screech", "razor", "laser", "bite", "resonant" }) || filterRes > 55.0f)
            setModSlot (params, 6, srcVelocity, dstResonance, amount (22.0f, 57, 7.0f));

        if (hasToken ({ "vibrato", "whistle", "siren", "solo", "flute" }))
            setModSlot (params, 7, srcLfo1, dstPitch, amount (18.0f, 58, 6.0f));

        setParamValue (params, "modrange_cutoff", ranged (125.0f, 70.0f, 200.0f, 59, 12.0f));
        setParamValue (params, "modrange_resonance", ranged (68.0f, 30.0f, 90.0f, 60, 8.0f));
        setParamValue (params, "modrange_pitch", ranged (3.2f, 1.5f, 8.0f, 61, 0.7f));
        setParamValue (params, "modrange_volume", ranged (28.0f, 12.0f, 50.0f, 62, 6.0f));
        setParamValue (params, "modrange_pw", ranged (38.0f, 15.0f, 70.0f, 63, 7.0f));
        return;
    }

    if (category.equalsIgnoreCase ("Bass"))
    {
        const int variant = static_cast<int> ((hash >> 11) & 0x3u);

        setModSlot (params, 1, srcVelocity, dstCutoff, amount (30.0f, 70, 8.0f));
        setModSlot (params, 2, srcModWheel, dstCutoff, amount (22.0f, 71, 7.0f));
        setModSlot (params, 3, srcAftertouch, dstResonance, amount (12.0f, 72, 5.0f));

        switch (variant)
        {
            case 0:
                setModSlot (params, 4, srcLfo1, dstCutoff, amount (24.0f, 73, 8.0f));
                setModSlot (params, 5, srcModWheel, dstLfo1Rate, amount (22.0f, 74, 8.0f));
                setModSlot (params, 6, srcVelocity, dstNoiseLevel, amount (12.0f, 75, 5.0f));
                break;
            case 1:
                setModSlot (params, 4, srcVelocity, dstOsc2Level, amount (10.0f, 76, 5.0f));
                setModSlot (params, 5, srcVelocity, dstOsc1Level, amount (-8.0f, 77, 4.0f));
                setModSlot (params, 6, srcAftertouch, dstResonance, amount (12.0f, 78, 5.0f));
                break;
            case 2:
                setModSlot (params, 4, srcModEnv1, dstCutoff, amount (16.0f, 79, 6.0f));
                setModSlot (params, 5, srcModWheel, dstCutoff, amount (16.0f, 80, 6.0f));
                setModSlot (params, 6, srcModEnv2, dstResonance, amount (10.0f, 81, 4.0f));
                break;
            default:
                setModSlot (params, 4, srcLfo3, dstCutoff, amount (18.0f, 82, 6.0f));
                setModSlot (params, 5, srcModWheel, dstLfo3Rate, amount (18.0f, 83, 6.0f));
                setModSlot (params, 6, srcVelocity, dstOsc2Level, amount (10.0f, 84, 4.0f));
                break;
        }

        if (hasToken ({ "wobble", "growl", "sweep", "step", "acid", "squelch", "screech", "filtered" }))
        {
            setModSlot (params, 4, srcLfo1, dstCutoff, amount (26.0f, 85, 8.0f));
            setModSlot (params, 5, srcModWheel, dstLfo1Rate, amount (22.0f, 86, 8.0f));
        }
        else if (osc2Level > 40.0f)
        {
            setModSlot (params, 5, srcVelocity, dstOsc1Level, amount (-8.0f, 87, 4.0f));
        }

        if (noiseLevel > 7.0f)
            setModSlot (params, 6, srcVelocity, dstNoiseLevel, amount (14.0f, 88, 6.0f));

        setModSlot (params, 8, srcModWheel, dstPitch, amount (8.0f, 89, 4.0f));

        setParamValue (params, "modrange_cutoff", ranged (135.0f, 80.0f, 220.0f, 90, 14.0f));
        setParamValue (params, "modrange_resonance", ranged (72.0f, 30.0f, 95.0f, 91, 9.0f));
        setParamValue (params, "modrange_pitch", ranged (1.3f, 0.4f, 4.0f, 92, 0.5f));
        setParamValue (params, "modrange_volume", ranged (24.0f, 10.0f, 45.0f, 93, 6.0f));
        setParamValue (params, "modrange_pw", ranged (32.0f, 10.0f, 65.0f, 94, 8.0f));
        setParamValue (params, "modrange_noise_level", ranged (125.0f, 60.0f, 200.0f, 95, 12.0f));
        return;
    }

    if (category.equalsIgnoreCase ("Keys") || category.equalsIgnoreCase ("Plucks"))
    {
        const int variant = static_cast<int> ((hash >> 15) & 0x3u);

        setModSlot (params, 1, srcVelocity, dstCutoff, amount (40.0f, 100, 8.0f));
        setModSlot (params, 2, srcVelocity, dstVolume, amount (20.0f, 101, 6.0f));
        setModSlot (params, 3, srcModWheel, dstCutoff, amount (16.0f, 102, 6.0f));
        setModSlot (params, 4, srcAftertouch, dstVolume, amount (10.0f, 103, 5.0f));

        switch (variant)
        {
            case 0:
                setModSlot (params, 5, srcLfo1, dstVolume, amount (10.0f, 104, 4.0f));
                setModSlot (params, 6, srcVelocity, dstResonance, amount (12.0f, 105, 5.0f));
                setModSlot (params, 7, srcModEnv1, dstCutoff, amount (10.0f, 106, 4.0f));
                setModSlot (params, 8, srcModWheel, dstPitch, amount (8.0f, 107, 4.0f));
                break;
            case 1:
                setModSlot (params, 5, srcModEnv2, dstCutoff, amount (10.0f, 108, 4.0f));
                setModSlot (params, 6, srcVelocity, dstOsc2Level, amount (11.0f, 109, 5.0f));
                setModSlot (params, 7, srcLfo3, dstPulseWidth, amount (11.0f, 110, 4.0f));
                setModSlot (params, 8, srcModWheel, dstCutoff, amount (12.0f, 111, 5.0f));
                break;
            case 2:
                setModSlot (params, 5, srcLfo1, dstPulseWidth, amount (12.0f, 112, 5.0f));
                setModSlot (params, 6, srcModEnv1, dstCutoff, amount (12.0f, 113, 5.0f));
                setModSlot (params, 7, srcAftertouch, dstVolume, amount (9.0f, 114, 4.0f));
                setModSlot (params, 8, srcModWheel, dstLfo1Rate, amount (11.0f, 115, 5.0f));
                break;
            default:
                setModSlot (params, 5, srcModWheel, dstResonance, amount (10.0f, 116, 4.0f));
                setModSlot (params, 6, srcVelocity, dstVolume, amount (10.0f, 117, 4.0f));
                setModSlot (params, 7, srcModEnv1, dstPitch, amount (7.0f, 118, 3.0f));
                setModSlot (params, 8, srcAftertouch, dstCutoff, amount (10.0f, 119, 4.0f));
                break;
        }

        if (hasToken ({ "organ", "vibe", "rhodes", "wurl", "accordion", "chorus" }))
            setModSlot (params, 5, srcLfo1, dstVolume, amount (10.0f, 120, 4.0f));

        if (hasToken ({ "pluck", "arp", "pick", "pizz", "banjo", "koto", "kalimba", "marimba", "harp", "mbira" }))
            setModSlot (params, 6, srcVelocity, dstResonance, amount (14.0f, 121, 6.0f));
        else
            setModSlot (params, 6, srcModEnv1, dstCutoff, amount (12.0f, 122, 5.0f));

        setParamValue (params, "modrange_cutoff", ranged (112.0f, 70.0f, 180.0f, 123, 10.0f));
        setParamValue (params, "modrange_resonance", ranged (48.0f, 20.0f, 75.0f, 124, 8.0f));
        setParamValue (params, "modrange_pitch", ranged (1.6f, 0.5f, 4.0f, 125, 0.5f));
        setParamValue (params, "modrange_volume", ranged (42.0f, 20.0f, 70.0f, 126, 8.0f));
        setParamValue (params, "modrange_pw", ranged (32.0f, 10.0f, 60.0f, 127, 7.0f));
        return;
    }

    if (category.equalsIgnoreCase ("Brass"))
    {
        const int variant = static_cast<int> ((hash >> 19) & 0x3u);

        setModSlot (params, 1, srcVelocity, dstCutoff, amount (30.0f, 130, 7.0f));
        setModSlot (params, 2, srcModWheel, dstCutoff, amount (22.0f, 131, 6.0f));
        setModSlot (params, 3, srcAftertouch, dstResonance, amount (10.0f, 132, 5.0f));
        setModSlot (params, 4, srcModWheel, dstPitch, amount (12.0f, 133, 5.0f));
        setModSlot (params, 5, srcModEnv1, dstCutoff, amount (16.0f, 134, 5.0f));

        switch (variant)
        {
            case 0:
                setModSlot (params, 6, srcAftertouch, dstVolume, amount (10.0f, 135, 4.0f));
                setModSlot (params, 7, srcModWheel, dstResonance, amount (10.0f, 136, 4.0f));
                setModSlot (params, 8, srcVelocity, dstPitch, amount (7.0f, 137, 3.0f));
                break;
            case 1:
                setModSlot (params, 6, srcVelocity, dstResonance, amount (11.0f, 138, 4.0f));
                setModSlot (params, 7, srcModEnv2, dstCutoff, amount (12.0f, 139, 4.0f));
                setModSlot (params, 8, srcModWheel, dstPitch, amount (10.0f, 140, 4.0f));
                break;
            case 2:
                setModSlot (params, 6, srcAftertouch, dstCutoff, amount (12.0f, 141, 5.0f));
                setModSlot (params, 7, srcModWheel, dstLfo1Rate, amount (10.0f, 142, 4.0f));
                setModSlot (params, 8, srcVelocity, dstVolume, amount (10.0f, 143, 4.0f));
                break;
            default:
                setModSlot (params, 6, srcModEnv1, dstResonance, amount (10.0f, 144, 4.0f));
                setModSlot (params, 7, srcAftertouch, dstPitch, amount (9.0f, 145, 4.0f));
                setModSlot (params, 8, srcModWheel, dstCutoff, amount (14.0f, 146, 5.0f));
                break;
        }

        if (hasToken ({ "soft", "warm", "mellow", "flugel", "cornet" }))
            setModSlot (params, 6, srcAftertouch, dstVolume, amount (10.0f, 147, 4.0f));

        setParamValue (params, "modrange_cutoff", ranged (122.0f, 70.0f, 190.0f, 148, 10.0f));
        setParamValue (params, "modrange_resonance", ranged (56.0f, 20.0f, 80.0f, 149, 8.0f));
        setParamValue (params, "modrange_pitch", ranged (2.0f, 0.8f, 5.0f, 150, 0.6f));
        setParamValue (params, "modrange_volume", ranged (32.0f, 15.0f, 55.0f, 151, 6.0f));
        return;
    }

    if (category.equalsIgnoreCase ("FX"))
    {
        const int variant = static_cast<int> ((hash >> 23) & 0x3u);

        setModSlot (params, 1, srcLfo2, dstCutoff, amount (48.0f, 160, 10.0f));
        setModSlot (params, 2, srcLfo3, dstPitch, amount (20.0f, 161, 8.0f));
        setModSlot (params, 3, srcModWheel, dstLfo2Rate, amount (30.0f, 162, 10.0f));
        setModSlot (params, 4, srcAftertouch, dstResonance, amount (20.0f, 163, 8.0f));
        setModSlot (params, 5, srcVelocity, dstNoiseLevel, amount (18.0f, 164, 8.0f));
        setModSlot (params, 6, srcModWheel, dstVolume, amount (14.0f, 165, 7.0f));

        switch (variant)
        {
            case 0:
                setModSlot (params, 7, srcLfo1, dstPitch, amount (30.0f, 166, 10.0f));
                setModSlot (params, 8, srcAftertouch, dstNoiseLevel, amount (14.0f, 167, 6.0f));
                break;
            case 1:
                setModSlot (params, 7, srcLfo1, dstCutoff, amount (24.0f, 168, 8.0f));
                setModSlot (params, 8, srcModWheel, dstPitch, amount (18.0f, 169, 7.0f));
                break;
            case 2:
                setModSlot (params, 7, srcModEnv1, dstPitch, amount (22.0f, 170, 8.0f));
                setModSlot (params, 8, srcVelocity, dstNoiseLevel, amount (18.0f, 171, 8.0f));
                break;
            default:
                setModSlot (params, 7, srcLfo3, dstResonance, amount (20.0f, 172, 7.0f));
                setModSlot (params, 8, srcAftertouch, dstLfo2Rate, amount (14.0f, 173, 6.0f));
                break;
        }

        if (hasToken ({ "siren", "laser", "dial", "modem", "glitch", "spike", "voltage", "comm", "radio" }))
            setModSlot (params, 7, srcLfo1, dstPitch, amount (30.0f, 174, 10.0f));

        if (noiseLevel > 10.0f || hasToken ({ "noise", "wind", "static", "thunder", "swarm", "rain" }))
            setModSlot (params, 8, srcAftertouch, dstNoiseLevel, amount (18.0f, 175, 8.0f));

        setParamValue (params, "modrange_cutoff", ranged (172.0f, 90.0f, 260.0f, 176, 16.0f));
        setParamValue (params, "modrange_resonance", ranged (86.0f, 40.0f, 100.0f, 177, 9.0f));
        setParamValue (params, "modrange_pitch", ranged (4.2f, 1.5f, 12.0f, 178, 1.0f));
        setParamValue (params, "modrange_volume", ranged (62.0f, 20.0f, 100.0f, 179, 10.0f));
        setParamValue (params, "modrange_pw", ranged (52.0f, 15.0f, 90.0f, 180, 9.0f));
        setParamValue (params, "modrange_noise_level", ranged (155.0f, 80.0f, 200.0f, 181, 12.0f));
        setParamValue (params, "modrange_lfo2_rate", ranged (18.0f, 4.0f, 35.0f, 182, 5.0f));
        setParamValue (params, "modrange_lfo3_rate", ranged (18.0f, 4.0f, 35.0f, 183, 5.0f));
        return;
    }

    // Utility/fallback profile
    const int variant = static_cast<int> ((hash >> 27) & 0x3u);
    setModSlot (params, 1, srcVelocity, dstCutoff, amount (18.0f, 190, 5.0f));
    setModSlot (params, 2, srcModWheel, dstPitch, amount (10.0f, 191, 4.0f));
    setModSlot (params, 3, srcAftertouch, dstVolume, amount (8.0f, 192, 3.0f));

    switch (variant)
    {
        case 0:
            setModSlot (params, 4, srcLfo1, dstCutoff, amount (12.0f, 193, 4.0f));
            setModSlot (params, 5, srcModWheel, dstLfo1Rate, amount (10.0f, 194, 4.0f));
            break;
        case 1:
            setModSlot (params, 4, srcModEnv1, dstCutoff, amount (10.0f, 195, 4.0f));
            setModSlot (params, 5, srcAftertouch, dstResonance, amount (9.0f, 196, 3.0f));
            break;
        case 2:
            setModSlot (params, 4, srcLfo3, dstPulseWidth, amount (12.0f, 197, 4.0f));
            setModSlot (params, 5, srcModWheel, dstCutoff, amount (12.0f, 198, 4.0f));
            break;
        default:
            setModSlot (params, 4, srcModEnv2, dstPitch, amount (8.0f, 199, 3.0f));
            setModSlot (params, 5, srcVelocity, dstResonance, amount (10.0f, 200, 4.0f));
            break;
    }

    setParamValue (params, "modrange_cutoff", ranged (102.0f, 60.0f, 160.0f, 201, 8.0f));
    setParamValue (params, "modrange_pitch", ranged (1.5f, 0.5f, 4.0f, 202, 0.4f));
}

// Helper to build a preset with defaults, then override specific params.
// Auto-normalizes master_volume based on combined oscillator energy so
// all presets output roughly the same perceived loudness.
static Preset makePreset (const juce::String& name, const juce::String& category,
                           std::initializer_list<std::pair<juce::String, float>> overrides)
{
    // Default parameter values matching the APVTS defaults
    std::vector<std::pair<juce::String, float>> defaults = {
        { "osc1_waveform", 1 }, { "osc1_octave", 0 }, { "osc1_semitone", 0 },
        { "osc1_fine_tune", 0 }, { "osc1_level", 50 }, { "osc1_pulse_width", 50 },
        { "osc1_unison_voices", 1 }, { "osc1_unison_detune", 20 }, { "osc1_unison_blend", 50 },
        { "osc1_attack", 5 }, { "osc1_decay", 200 }, { "osc1_sustain", 70 }, { "osc1_release", 300 },

        { "osc2_waveform", 1 }, { "osc2_octave", 0 }, { "osc2_semitone", 0 },
        { "osc2_fine_tune", 0 }, { "osc2_level", 50 }, { "osc2_pulse_width", 50 },
        { "osc2_unison_voices", 1 }, { "osc2_unison_detune", 20 }, { "osc2_unison_blend", 50 },
        { "osc2_attack", 5 }, { "osc2_decay", 200 }, { "osc2_sustain", 70 }, { "osc2_release", 300 },

        { "filter_model", 0 }, { "filter_mode", 0 }, { "filter_slope", 2 },
        { "filter_target", 4 }, { "filter_cutoff", 8000 },
        { "filter_resonance", 20 }, { "filter_env_amount", 50 }, { "filter_key_tracking", 0 },
        { "filter_velocity", 0 },
        { "filter_attack", 10 }, { "filter_decay", 400 }, { "filter_sustain", 30 },
        { "filter_release", 500 },

        { "lfo1_waveform", 0 }, { "lfo1_rate", 1 }, { "lfo1_depth", 0 },
        { "lfo1_dest_cutoff", 0 }, { "lfo1_dest_pitch", 0 },
        { "lfo1_dest_volume", 0 }, { "lfo1_dest_pw", 0 },

        { "lfo2_waveform", 0 }, { "lfo2_rate", 1 }, { "lfo2_depth", 0 },
        { "lfo2_dest_cutoff", 0 }, { "lfo2_dest_pitch", 0 },
        { "lfo2_dest_volume", 0 }, { "lfo2_dest_pw", 0 },

        { "lfo3_waveform", 0 }, { "lfo3_rate", 1 }, { "lfo3_depth", 0 },
        { "lfo3_dest_cutoff", 0 }, { "lfo3_dest_pitch", 0 },
        { "lfo3_dest_volume", 0 }, { "lfo3_dest_pw", 0 },

        { "modenv1_attack", 5 }, { "modenv1_decay", 200 }, { "modenv1_sustain", 0 },
        { "modenv1_release", 300 }, { "modenv1_amount", 0 },
        { "modenv1_dest_cutoff", 0 }, { "modenv1_dest_resonance", 0 },
        { "modenv1_dest_pitch", 0 }, { "modenv1_dest_volume", 0 },

        { "modenv2_attack", 5 }, { "modenv2_decay", 200 }, { "modenv2_sustain", 0 },
        { "modenv2_release", 300 }, { "modenv2_amount", 0 },
        { "modenv2_dest_cutoff", 0 }, { "modenv2_dest_resonance", 0 },
        { "modenv2_dest_pitch", 0 }, { "modenv2_dest_volume", 0 },

        { "modmatrix_1_source", 0 }, { "modmatrix_1_dest", 0 }, { "modmatrix_1_amount", 0 },
        { "modmatrix_2_source", 0 }, { "modmatrix_2_dest", 0 }, { "modmatrix_2_amount", 0 },
        { "modmatrix_3_source", 0 }, { "modmatrix_3_dest", 0 }, { "modmatrix_3_amount", 0 },
        { "modmatrix_4_source", 0 }, { "modmatrix_4_dest", 0 }, { "modmatrix_4_amount", 0 },
        { "modmatrix_5_source", 0 }, { "modmatrix_5_dest", 0 }, { "modmatrix_5_amount", 0 },
        { "modmatrix_6_source", 0 }, { "modmatrix_6_dest", 0 }, { "modmatrix_6_amount", 0 },
        { "modmatrix_7_source", 0 }, { "modmatrix_7_dest", 0 }, { "modmatrix_7_amount", 0 },
        { "modmatrix_8_source", 0 }, { "modmatrix_8_dest", 0 }, { "modmatrix_8_amount", 0 },

        { "modrange_cutoff", 75 }, { "modrange_resonance", 75 }, { "modrange_pitch", 2 },
        { "modrange_volume", 50 }, { "modrange_pw", 45 },
        { "modrange_osc1_level", 100 }, { "modrange_osc2_level", 100 }, { "modrange_noise_level", 100 },
        { "modrange_lfo1_rate", 10 }, { "modrange_lfo2_rate", 10 }, { "modrange_lfo3_rate", 10 },

        { "chorus_mode", 1 }, { "chorus_depth", 50 },

        { "reverb_size", 50 }, { "reverb_damping", 50 }, { "reverb_mix", 0 }, { "reverb_width", 100 },

        { "noise_type", 0 }, { "noise_level", 0 }, { "noise_sh_rate", 1000 },
        { "noise_attack", 5 }, { "noise_decay", 200 }, { "noise_sustain", 70 }, { "noise_release", 300 },

        { "master_volume", 80 }, { "master_polyphony", 16 }, { "master_glide", 0 },
        { "master_pitch_bend", 2 }, { "master_mono_legato", 0 }, { "master_analog_drift", 0 },
        { "master_unison", 1 }, { "master_unison_detune", 20 },
        { "sidechain_amount", 0 },
        { "output_mode", 0 }, { "output_drive", 25 }, { "output_mix", 100 }
    };

    // Check if master_volume is explicitly overridden by the preset
    bool volumeOverridden = false;
    for (const auto& ov : overrides)
    {
        if (ov.first == "master_volume")
        {
            volumeOverridden = true;
            break;
        }
    }

    // Apply explicit overrides first.
    for (const auto& ov : overrides)
    {
        for (auto& def : defaults)
        {
            if (def.first == ov.first)
            {
                def.second = ov.second;
                break;
            }
        }
    }

    // Assign a preset-specific mod matrix profile from final sound settings.
    applyPresetSpecificModMatrixProfile (name, category, defaults);

    // Auto-normalize volume based on combined oscillator energy.
    // Presets with many unison voices or high osc levels would otherwise
    // be dramatically louder than single-oscillator presets.
    if (! volumeOverridden)
    {
        auto getParam = [&defaults] (const juce::String& id) -> float
        {
            for (const auto& p : defaults)
                if (p.first == id) return p.second;
            return 0.0f;
        };

        float osc1Level = getParam ("osc1_level") / 100.0f;
        float osc2Level = getParam ("osc2_level") / 100.0f;
        float osc1Unison = std::max (1.0f, getParam ("osc1_unison_voices"));
        float osc2Unison = std::max (1.0f, getParam ("osc2_unison_voices"));
        // Noise path is capped to 50% in SynthVoice, so reflect that here.
        float noiseLevel = (getParam ("noise_level") / 100.0f) * 0.5f;

        // Energy estimate: level * sqrt(unison voices) per oscillator + noise
        float energy = osc1Level * std::sqrt (osc1Unison)
                     + osc2Level * std::sqrt (osc2Unison)
                     + noiseLevel;

        // Baseline energy: two oscillators at 50% = 1.0
        constexpr float targetEnergy = 1.0f;

        if (energy > 0.01f)
        {
            float normalizedVolume = 80.0f * targetEnergy / energy;
            normalizedVolume = juce::jlimit (40.0f, 100.0f, normalizedVolume);

            for (auto& def : defaults)
            {
                if (def.first == "master_volume")
                {
                    def.second = std::round (normalizedVolume);
                    break;
                }
            }
        }
    }

    return { name, category, defaults };
}

void PresetManager::buildFactoryPresets()
{
    // ==================== PADS (1-10) ====================

    presets.push_back (makePreset ("Warm Velvet Pad", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 15 },
        { "osc1_attack", 800 }, { "osc1_decay", 2000 }, { "osc1_sustain", 80 }, { "osc1_release", 1500 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 7 }, { "osc2_level", 70 },
        { "osc2_attack", 1000 }, { "osc2_decay", 2000 }, { "osc2_sustain", 80 }, { "osc2_release", 1500 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 10 }, { "filter_env_amount", 20 },
        { "chorus_mode", 3 }, { "master_analog_drift", 15 },
        { "reverb_size", 65 }, { "reverb_mix", 25 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 800 }, { "noise_sustain", 60 }, { "noise_release", 1500 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Crystal Shimmer", "Pads", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 25 },
        { "osc1_attack", 1200 }, { "osc1_sustain", 90 }, { "osc1_release", 3000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 40 },
        { "osc2_attack", 1500 }, { "osc2_sustain", 90 }, { "osc2_release", 3000 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 25 },
        { "lfo1_depth", 15 }, { "lfo1_rate", 0.3f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 20 },
        { "reverb_size", 70 }, { "reverb_mix", 30 },
        { "noise_type", 3 }, { "noise_level", 6 }, { "noise_attack", 1200 }, { "noise_sustain", 30 }, { "noise_release", 3000 }
    }));

    presets.push_back (makePreset ("Dark Atmosphere", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 4 }, { "osc1_unison_detune", 30 },
        { "osc1_attack", 2000 }, { "osc1_sustain", 60 }, { "osc1_release", 4000 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_level", 50 }, { "osc2_pulse_width", 30 },
        { "osc2_attack", 2500 }, { "osc2_sustain", 60 }, { "osc2_release", 4000 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 30 }, { "filter_env_amount", 30 },
        { "filter_decay", 3000 }, { "filter_sustain", 20 },
        { "chorus_mode", 3 }, { "master_analog_drift", 30 },
        { "reverb_size", 80 }, { "reverb_mix", 35 },
        { "noise_type", 2 }, { "noise_level", 12 }, { "noise_attack", 2000 }, { "noise_sustain", 50 }, { "noise_release", 4000 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Juno Strings", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 12 },
        { "osc1_attack", 400 }, { "osc1_sustain", 85 }, { "osc1_release", 800 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", -8 }, { "osc2_level", 80 },
        { "osc2_attack", 500 }, { "osc2_sustain", 85 }, { "osc2_release", 800 },
        { "filter_cutoff", 4500 }, { "filter_resonance", 5 }, { "filter_env_amount", 15 },
        { "chorus_mode", 1 }, { "master_analog_drift", 10 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_type", 1 }, { "noise_level", 5 }, { "noise_attack", 400 }, { "noise_sustain", 70 }, { "noise_release", 800 }
    }));

    presets.push_back (makePreset ("Floating Clouds", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 20 },
        { "osc1_attack", 3000 }, { "osc1_sustain", 90 }, { "osc1_release", 5000 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 4000 }, { "osc2_sustain", 90 }, { "osc2_release", 5000 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 15 },
        { "lfo1_depth", 20 }, { "lfo1_rate", 0.15f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 25 },
        { "reverb_size", 85 }, { "reverb_mix", 35 },
        { "noise_type", 1 }, { "noise_level", 10 }, { "noise_attack", 3000 }, { "noise_sustain", 80 }, { "noise_release", 5000 }
    }));

    presets.push_back (makePreset ("Analog Heaven", "Pads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 40 }, { "osc1_unison_voices", 3 },
        { "osc1_attack", 600 }, { "osc1_sustain", 75 }, { "osc1_release", 2000 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 5 }, { "osc2_level", 60 },
        { "osc2_attack", 800 }, { "osc2_sustain", 75 }, { "osc2_release", 2000 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 20 }, { "filter_env_amount", 35 },
        { "lfo1_depth", 10 }, { "lfo1_rate", 0.5f }, { "lfo1_dest_pw", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 40 },
        { "reverb_size", 60 }, { "reverb_mix", 20 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Choir of Angels", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 1500 }, { "osc1_sustain", 95 }, { "osc1_release", 3000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 50 },
        { "osc2_unison_voices", 5 }, { "osc2_unison_detune", 12 },
        { "osc2_attack", 2000 }, { "osc2_sustain", 95 }, { "osc2_release", 3000 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 8 },
        { "chorus_mode", 2 }, { "master_analog_drift", 15 },
        { "reverb_size", 75 }, { "reverb_mix", 30 },
        { "noise_level", 4 }, { "noise_attack", 1500 }, { "noise_decay", 3000 }, { "noise_sustain", 30 }, { "noise_release", 3000 }
    }));

    presets.push_back (makePreset ("Glacial Drift", "Pads", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 4 }, { "osc1_unison_detune", 35 },
        { "osc1_attack", 4000 }, { "osc1_sustain", 70 }, { "osc1_release", 6000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 25 },
        { "osc2_attack", 5000 }, { "osc2_sustain", 70 }, { "osc2_release", 6000 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 35 }, { "filter_env_amount", 40 },
        { "filter_decay", 5000 }, { "filter_sustain", 10 },
        { "lfo1_depth", 25 }, { "lfo1_rate", 0.08f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 50 },
        { "reverb_size", 90 }, { "reverb_mix", 40 },
        { "noise_type", 2 }, { "noise_level", 10 }, { "noise_attack", 4000 }, { "noise_sustain", 60 }, { "noise_release", 6000 }
    }));

    presets.push_back (makePreset ("Warm Blanket", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 8 },
        { "osc1_attack", 500 }, { "osc1_sustain", 90 }, { "osc1_release", 1200 },
        { "osc2_waveform", 3 }, { "osc2_fine_tune", -3 }, { "osc2_level", 90 },
        { "osc2_attack", 600 }, { "osc2_sustain", 90 }, { "osc2_release", 1200 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 5 }, { "filter_env_amount", 10 },
        { "chorus_mode", 1 }, { "master_analog_drift", 12 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_type", 1 }, { "noise_level", 6 }, { "noise_attack", 500 }, { "noise_sustain", 80 }, { "noise_release", 1200 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Neon Horizons", "Pads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 35 }, { "osc1_unison_voices", 5 },
        { "osc1_unison_detune", 22 },
        { "osc1_attack", 1000 }, { "osc1_sustain", 80 }, { "osc1_release", 2500 },
        { "osc2_waveform", 1 }, { "osc2_octave", 1 }, { "osc2_level", 45 },
        { "osc2_attack", 1200 }, { "osc2_sustain", 80 }, { "osc2_release", 2500 },
        { "filter_cutoff", 5500 }, { "filter_resonance", 30 }, { "filter_env_amount", 45 },
        { "filter_decay", 2000 },
        { "lfo1_depth", 12 }, { "lfo1_rate", 0.4f }, { "lfo1_dest_pw", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 20 },
        { "reverb_size", 60 }, { "reverb_mix", 20 },
        { "master_unison", 2 }
    }));

    // ==================== LEADS (11-20) ====================

    presets.push_back (makePreset ("Classic Mono Lead", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_level", 100 },
        { "osc1_attack", 2 }, { "osc1_decay", 300 }, { "osc1_sustain", 60 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_octave", 1 }, { "osc2_level", 50 }, { "osc2_pulse_width", 40 },
        { "osc2_attack", 2 }, { "osc2_decay", 300 }, { "osc2_sustain", 60 }, { "osc2_release", 100 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 40 }, { "filter_env_amount", 70 },
        { "filter_decay", 500 }, { "filter_sustain", 20 },
        { "master_mono_legato", 1 }, { "master_glide", 60 }
    }));

    presets.push_back (makePreset ("Screaming Saw", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 150 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 5 }, { "osc2_level", 90 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 150 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 35 }, { "filter_env_amount", 50 },
        { "filter_decay", 400 },
        { "master_mono_legato", 1 }, { "master_glide", 30 },
        { "reverb_size", 40 }, { "reverb_mix", 10 }
    }));

    presets.push_back (makePreset ("Soft Sine Lead", "Leads", {
        { "osc1_waveform", 0 }, { "osc1_attack", 10 }, { "osc1_sustain", 90 }, { "osc1_release", 200 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 10 }, { "osc2_sustain", 90 }, { "osc2_release", 200 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 10 },
        { "lfo1_depth", 8 }, { "lfo1_rate", 5.5f }, { "lfo1_dest_pitch", 1 },
        { "master_mono_legato", 1 }, { "master_glide", 80 },
        { "reverb_size", 50 }, { "reverb_mix", 15 }
    }));

    presets.push_back (makePreset ("PWM Lead", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 30 },
        { "osc1_attack", 3 }, { "osc1_sustain", 75 }, { "osc1_release", 120 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 60 }, { "osc2_fine_tune", 8 },
        { "osc2_attack", 3 }, { "osc2_sustain", 75 }, { "osc2_release", 120 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 25 }, { "filter_env_amount", 40 },
        { "lfo1_depth", 30 }, { "lfo1_rate", 1.2f }, { "lfo1_dest_pw", 1 },
        { "chorus_mode", 1 },
        { "reverb_size", 45 }, { "reverb_mix", 12 }
    }));

    presets.push_back (makePreset ("Acid Squelch", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_attack", 1 }, { "osc1_sustain", 50 }, { "osc1_release", 80 },
        { "osc2_level", 0 },
        { "filter_cutoff", 500 }, { "filter_resonance", 80 }, { "filter_env_amount", 90 },
        { "filter_slope", 2 }, { "filter_decay", 300 }, { "filter_sustain", 5 },
        { "master_mono_legato", 1 }, { "master_glide", 40 }, { "master_analog_drift", 10 }
    }));

    presets.push_back (makePreset ("Thick Unison Lead", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 18 },
        { "osc1_attack", 2 }, { "osc1_sustain", 85 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_level", 60 }, { "osc2_pulse_width", 45 },
        { "osc2_attack", 2 }, { "osc2_sustain", 85 }, { "osc2_release", 100 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 15 }, { "filter_env_amount", 30 },
        { "chorus_mode", 1 }, { "master_analog_drift", 8 },
        { "reverb_size", 40 }, { "reverb_mix", 10 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Ethereal Whistle", "Leads", {
        { "osc1_waveform", 0 }, { "osc1_octave", 1 },
        { "osc1_attack", 200 }, { "osc1_sustain", 95 }, { "osc1_release", 500 },
        { "osc2_waveform", 3 }, { "osc2_octave", 2 }, { "osc2_level", 20 },
        { "osc2_attack", 300 }, { "osc2_sustain", 95 }, { "osc2_release", 500 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 10 },
        { "lfo1_depth", 6 }, { "lfo1_rate", 4.0f }, { "lfo1_dest_pitch", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 5 },
        { "reverb_size", 65 }, { "reverb_mix", 25 },
        { "noise_level", 5 }, { "noise_attack", 200 }, { "noise_decay", 500 }, { "noise_sustain", 20 }, { "noise_release", 500 }
    }));

    presets.push_back (makePreset ("Detuned Aggression", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 40 },
        { "osc1_attack", 1 }, { "osc1_sustain", 70 }, { "osc1_release", 80 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 70 },
        { "osc2_attack", 1 }, { "osc2_sustain", 70 }, { "osc2_release", 80 },
        { "filter_cutoff", 7000 }, { "filter_resonance", 20 },
        { "filter_env_amount", 60 }, { "filter_decay", 250 },
        { "master_analog_drift", 15 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Vibrato Solo", "Leads", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 5 }, { "osc1_sustain", 80 }, { "osc1_release", 150 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 40 },
        { "osc2_attack", 5 }, { "osc2_sustain", 80 }, { "osc2_release", 150 },
        { "filter_cutoff", 4500 }, { "filter_resonance", 20 }, { "filter_env_amount", 35 },
        { "lfo1_depth", 15 }, { "lfo1_rate", 5.0f }, { "lfo1_dest_pitch", 1 },
        { "master_mono_legato", 1 }, { "master_glide", 50 },
        { "reverb_size", 45 }, { "reverb_mix", 12 }
    }));

    presets.push_back (makePreset ("Retro Poly Lead", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 45 },
        { "osc1_attack", 5 }, { "osc1_sustain", 70 }, { "osc1_release", 200 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 3 }, { "osc2_level", 80 },
        { "osc2_attack", 5 }, { "osc2_sustain", 70 }, { "osc2_release", 200 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 30 }, { "filter_env_amount", 45 },
        { "filter_decay", 500 }, { "filter_sustain", 25 },
        { "chorus_mode", 1 }, { "master_analog_drift", 12 },
        { "reverb_size", 45 }, { "reverb_mix", 12 }
    }));

    // ==================== BASS (21-30) ====================

    presets.push_back (makePreset ("Sub Bass", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -2 },
        { "osc1_attack", 2 }, { "osc1_decay", 100 }, { "osc1_sustain", 90 }, { "osc1_release", 50 },
        { "osc2_level", 0 },
        { "filter_cutoff", 500 }, { "filter_resonance", 5 },
        { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("Analog Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 2 }, { "osc1_decay", 300 }, { "osc1_sustain", 60 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_level", 70 }, { "osc2_pulse_width", 40 },
        { "osc2_attack", 2 }, { "osc2_decay", 300 }, { "osc2_sustain", 60 }, { "osc2_release", 80 },
        { "filter_cutoff", 1200 }, { "filter_resonance", 30 }, { "filter_env_amount", 60 },
        { "filter_decay", 400 }, { "filter_sustain", 15 },
        { "master_mono_legato", 1 }, { "master_analog_drift", 8 },
        { "noise_level", 4 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Rubber Bass", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_decay", 200 }, { "osc1_sustain", 50 }, { "osc1_release", 60 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 50 },
        { "osc2_attack", 1 }, { "osc2_decay", 200 }, { "osc2_sustain", 50 }, { "osc2_release", 60 },
        { "filter_cutoff", 800 }, { "filter_resonance", 45 }, { "filter_env_amount", 80 },
        { "filter_decay", 250 }, { "filter_sustain", 10 },
        { "master_mono_legato", 1 }, { "master_glide", 20 }
    }));

    presets.push_back (makePreset ("Acid Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_sustain", 40 }, { "osc1_release", 50 },
        { "osc2_level", 0 },
        { "filter_cutoff", 400 }, { "filter_resonance", 75 }, { "filter_env_amount", 85 },
        { "filter_slope", 2 }, { "filter_decay", 200 }, { "filter_sustain", 5 },
        { "master_mono_legato", 1 }, { "master_glide", 30 }, { "master_analog_drift", 5 }
    }));

    presets.push_back (makePreset ("Warm Pluck Bass", "Bass", {
        { "osc1_waveform", 3 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 0 }, { "osc1_release", 100 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 80 },
        { "osc2_attack", 1 }, { "osc2_decay", 500 }, { "osc2_sustain", 0 }, { "osc2_release", 100 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 15 }, { "filter_env_amount", 50 },
        { "filter_decay", 300 }, { "filter_sustain", 5 },
        { "noise_level", 6 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Wobble Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_level", 60 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 80 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 50 },
        { "lfo1_depth", 60 }, { "lfo1_rate", 3.0f }, { "lfo1_dest_cutoff", 1 },
        { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("808 Style", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -2 },
        { "osc1_attack", 1 }, { "osc1_decay", 800 }, { "osc1_sustain", 0 }, { "osc1_release", 300 },
        { "osc2_level", 0 },
        { "filter_cutoff", 1000 }, { "filter_resonance", 5 }, { "filter_env_amount", 40 },
        { "filter_decay", 600 }, { "filter_sustain", 0 },
        { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("Reese Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 2 }, { "osc1_sustain", 90 }, { "osc1_release", 100 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_fine_tune", 10 }, { "osc2_level", 100 },
        { "osc2_attack", 2 }, { "osc2_sustain", 90 }, { "osc2_release", 100 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 15 },
        { "master_mono_legato", 1 }, { "master_analog_drift", 10 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Funky Slap", "Bass", {
        { "osc1_waveform", 2 }, { "osc1_octave", -1 }, { "osc1_pulse_width", 30 },
        { "osc1_attack", 1 }, { "osc1_decay", 150 }, { "osc1_sustain", 40 }, { "osc1_release", 60 },
        { "osc2_level", 0 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 35 }, { "filter_env_amount", 70 },
        { "filter_decay", 120 }, { "filter_sustain", 10 }, { "filter_velocity", 60 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Deep Dub", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -2 },
        { "osc1_attack", 5 }, { "osc1_decay", 600 }, { "osc1_sustain", 70 }, { "osc1_release", 200 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 40 },
        { "osc2_attack", 5 }, { "osc2_decay", 600 }, { "osc2_sustain", 70 }, { "osc2_release", 200 },
        { "filter_cutoff", 600 }, { "filter_resonance", 20 }, { "filter_env_amount", 30 },
        { "filter_decay", 500 },
        { "chorus_mode", 1 }, { "master_mono_legato", 1 }, { "master_analog_drift", 15 },
        { "reverb_size", 50 }, { "reverb_mix", 12 },
        { "noise_type", 2 }, { "noise_level", 6 }, { "noise_attack", 5 }, { "noise_decay", 600 }, { "noise_sustain", 50 }, { "noise_release", 200 }
    }));

    // ==================== KEYS (31-38) ====================

    presets.push_back (makePreset ("Electric Piano", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 2 }, { "osc1_decay", 600 }, { "osc1_sustain", 40 }, { "osc1_release", 200 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 50 },
        { "osc2_attack", 2 }, { "osc2_decay", 400 }, { "osc2_sustain", 20 }, { "osc2_release", 200 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 10 },
        { "filter_env_amount", 30 }, { "filter_decay", 300 },
        { "filter_velocity", 50 }, { "chorus_mode", 1 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_level", 3 }, { "noise_attack", 1 }, { "noise_decay", 80 }, { "noise_sustain", 0 }, { "noise_release", 50 }
    }));

    presets.push_back (makePreset ("Bright Pluck", "Keys", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 300 }, { "osc1_sustain", 0 }, { "osc1_release", 150 },
        { "osc2_waveform", 3 }, { "osc2_level", 60 },
        { "osc2_attack", 1 }, { "osc2_decay", 250 }, { "osc2_sustain", 0 }, { "osc2_release", 150 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 15 },
        { "filter_env_amount", 60 }, { "filter_decay", 200 }, { "filter_sustain", 0 },
        { "filter_velocity", 40 },
        { "reverb_size", 50 }, { "reverb_mix", 18 }
    }));

    presets.push_back (makePreset ("Clav", "Keys", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 25 },
        { "osc1_attack", 1 }, { "osc1_decay", 200 }, { "osc1_sustain", 30 }, { "osc1_release", 80 },
        { "osc2_level", 0 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 25 },
        { "filter_env_amount", 50 }, { "filter_decay", 150 }, { "filter_sustain", 10 },
        { "filter_velocity", 70 },
        { "reverb_size", 40 }, { "reverb_mix", 12 },
        { "noise_level", 3 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Vibraphone", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 5 }, { "osc1_decay", 1500 }, { "osc1_sustain", 0 }, { "osc1_release", 800 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 25 },
        { "osc2_attack", 5 }, { "osc2_decay", 800 }, { "osc2_sustain", 0 }, { "osc2_release", 800 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 8 },
        { "lfo1_depth", 8 }, { "lfo1_rate", 5.0f }, { "lfo1_dest_volume", 1 },
        { "reverb_size", 65 }, { "reverb_mix", 25 }
    }));

    presets.push_back (makePreset ("Organ", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 10 }, { "osc1_sustain", 100 }, { "osc1_release", 30 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 70 },
        { "osc2_attack", 10 }, { "osc2_sustain", 100 }, { "osc2_release", 30 },
        { "filter_cutoff", 10000 }, { "filter_resonance", 5 },
        { "lfo1_depth", 5 }, { "lfo1_rate", 6.0f }, { "lfo1_dest_volume", 1 },
        { "chorus_mode", 2 },
        { "reverb_size", 50 }, { "reverb_mix", 15 }
    }));

    presets.push_back (makePreset ("Bell Tone", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 1 }, { "osc1_decay", 2000 }, { "osc1_sustain", 0 }, { "osc1_release", 1000 },
        { "osc2_waveform", 0 }, { "osc2_semitone", 7 }, { "osc2_level", 60 },
        { "osc2_attack", 1 }, { "osc2_decay", 1500 }, { "osc2_sustain", 0 }, { "osc2_release", 1000 },
        { "filter_cutoff", 12000 }, { "filter_resonance", 5 },
        { "reverb_size", 60 }, { "reverb_mix", 25 }
    }));

    presets.push_back (makePreset ("Harpsichord", "Keys", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 15 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 10 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_octave", 1 }, { "osc2_pulse_width", 20 }, { "osc2_level", 50 },
        { "osc2_attack", 1 }, { "osc2_decay", 300 }, { "osc2_sustain", 5 }, { "osc2_release", 100 },
        { "filter_cutoff", 9000 }, { "filter_resonance", 15 },
        { "filter_env_amount", 40 }, { "filter_decay", 200 }, { "filter_sustain", 5 },
        { "reverb_size", 50 }, { "reverb_mix", 18 },
        { "noise_level", 4 }, { "noise_attack", 1 }, { "noise_decay", 60 }, { "noise_sustain", 0 }, { "noise_release", 40 }
    }));

    presets.push_back (makePreset ("Muted Keys", "Keys", {
        { "osc1_waveform", 3 },
        { "osc1_attack", 3 }, { "osc1_decay", 400 }, { "osc1_sustain", 40 }, { "osc1_release", 150 },
        { "osc2_waveform", 0 }, { "osc2_level", 60 },
        { "osc2_attack", 3 }, { "osc2_decay", 400 }, { "osc2_sustain", 40 }, { "osc2_release", 150 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 10 },
        { "filter_env_amount", 30 }, { "filter_decay", 200 },
        { "chorus_mode", 1 },
        { "reverb_size", 45 }, { "reverb_mix", 15 }
    }));

    // ==================== FX / TEXTURES (39-45) ====================

    presets.push_back (makePreset ("Sci-Fi Sweep", "FX", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 50 },
        { "osc1_attack", 3000 }, { "osc1_sustain", 50 }, { "osc1_release", 4000 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 20 }, { "osc2_level", 60 },
        { "osc2_attack", 3000 }, { "osc2_sustain", 50 }, { "osc2_release", 4000 },
        { "filter_cutoff", 300 }, { "filter_resonance", 60 }, { "filter_env_amount", 90 },
        { "filter_attack", 3000 }, { "filter_decay", 5000 }, { "filter_sustain", 10 },
        { "master_analog_drift", 40 },
        { "reverb_size", 70 }, { "reverb_mix", 30 },
        { "noise_type", 3 }, { "noise_level", 8 }, { "noise_attack", 3000 }, { "noise_sustain", 40 }, { "noise_release", 4000 },
        { "master_unison", 3 }
    }));

    presets.push_back (makePreset ("Laser Zap", "FX", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 100 }, { "osc1_sustain", 0 }, { "osc1_release", 50 },
        { "osc2_level", 0 },
        { "filter_cutoff", 15000 }, { "filter_resonance", 70 }, { "filter_env_amount", -90 },
        { "filter_decay", 80 }, { "filter_sustain", 0 },
        { "master_mono_legato", 1 },
        { "reverb_size", 40 }, { "reverb_mix", 15 }
    }));

    presets.push_back (makePreset ("Wind Noise", "FX", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 100 },
        { "osc1_attack", 2000 }, { "osc1_sustain", 60 }, { "osc1_release", 3000 },
        { "osc2_waveform", 3 }, { "osc2_unison_voices", 7 }, { "osc2_unison_detune", 80 },
        { "osc2_level", 80 },
        { "osc2_attack", 2500 }, { "osc2_sustain", 60 }, { "osc2_release", 3000 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 20 },
        { "lfo1_depth", 40 }, { "lfo1_rate", 0.2f }, { "lfo1_dest_cutoff", 1 },
        { "lfo2_depth", 15 }, { "lfo2_rate", 0.05f }, { "lfo2_dest_volume", 1 },
        { "master_analog_drift", 60 },
        { "reverb_size", 70 }, { "reverb_mix", 30 },
        { "noise_type", 1 }, { "noise_level", 45 }, { "noise_attack", 2000 }, { "noise_sustain", 60 }, { "noise_release", 3000 }
    }));

    presets.push_back (makePreset ("Metallic Ring", "FX", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 10 },
        { "osc1_attack", 1 }, { "osc1_decay", 3000 }, { "osc1_sustain", 0 }, { "osc1_release", 2000 },
        { "osc2_waveform", 2 }, { "osc2_semitone", 5 }, { "osc2_pulse_width", 15 }, { "osc2_level", 80 },
        { "osc2_attack", 1 }, { "osc2_decay", 2500 }, { "osc2_sustain", 0 }, { "osc2_release", 2000 },
        { "filter_cutoff", 10000 }, { "filter_resonance", 40 },
        { "chorus_mode", 3 },
        { "reverb_size", 60 }, { "reverb_mix", 25 }
    }));

    presets.push_back (makePreset ("Underwater", "FX", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 3 },
        { "osc1_attack", 500 }, { "osc1_sustain", 80 }, { "osc1_release", 2000 },
        { "osc2_waveform", 0 }, { "osc2_octave", -1 }, { "osc2_level", 60 },
        { "osc2_attack", 600 }, { "osc2_sustain", 80 }, { "osc2_release", 2000 },
        { "filter_cutoff", 800 }, { "filter_resonance", 50 },
        { "lfo1_depth", 50 }, { "lfo1_rate", 0.4f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 35 },
        { "reverb_size", 75 }, { "reverb_mix", 35 },
        { "noise_type", 2 }, { "noise_level", 10 }, { "noise_attack", 500 }, { "noise_sustain", 70 }, { "noise_release", 2000 }
    }));

    presets.push_back (makePreset ("Bit Crusher Tone", "FX", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 5 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 50 },
        { "osc2_waveform", 2 }, { "osc2_semitone", 7 }, { "osc2_pulse_width", 8 }, { "osc2_level", 90 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 50 },
        { "filter_cutoff", 12000 }, { "filter_resonance", 10 },
        { "master_analog_drift", 20 },
        { "reverb_size", 40 }, { "reverb_mix", 15 },
        { "noise_type", 4 }, { "noise_level", 12 }, { "noise_sh_rate", 3000 }, { "noise_sustain", 60 }, { "noise_release", 50 }
    }));

    presets.push_back (makePreset ("Ghost Whisper", "FX", {
        { "osc1_waveform", 0 }, { "osc1_octave", 2 },
        { "osc1_attack", 1000 }, { "osc1_decay", 3000 }, { "osc1_sustain", 20 }, { "osc1_release", 5000 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 1500 }, { "osc2_decay", 3000 }, { "osc2_sustain", 20 }, { "osc2_release", 5000 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 30 },
        { "filter_env_amount", 50 }, { "filter_decay", 2000 }, { "filter_sustain", 5 },
        { "lfo1_depth", 10 }, { "lfo1_rate", 0.1f }, { "lfo1_dest_pitch", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 45 },
        { "reverb_size", 85 }, { "reverb_mix", 40 },
        { "noise_type", 1 }, { "noise_level", 15 }, { "noise_attack", 1000 }, { "noise_decay", 3000 }, { "noise_sustain", 15 }, { "noise_release", 5000 }
    }));

    // ==================== INIT / UTILITY (46-50) ====================

    presets.push_back (makePreset ("Init Saw", "Utility", {
        { "osc1_waveform", 1 }, { "osc2_level", 0 }
    }));

    presets.push_back (makePreset ("Init Square", "Utility", {
        { "osc1_waveform", 2 }, { "osc2_level", 0 }
    }));

    presets.push_back (makePreset ("Init Dual Osc", "Utility", {
        { "osc1_waveform", 1 }, { "osc2_waveform", 2 }, { "osc2_fine_tune", 5 }
    }));

    presets.push_back (makePreset ("Full Drift Demo", "Utility", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 },
        { "osc1_attack", 200 }, { "osc1_sustain", 80 }, { "osc1_release", 500 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 6 }, { "osc2_level", 80 },
        { "osc2_attack", 200 }, { "osc2_sustain", 80 }, { "osc2_release", 500 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 20 },
        { "chorus_mode", 3 }, { "master_analog_drift", 100 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 200 }, { "noise_sustain", 50 }, { "noise_release", 500 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Chorus Showcase", "Utility", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 100 }, { "osc1_sustain", 85 }, { "osc1_release", 400 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 40 }, { "osc2_level", 60 },
        { "osc2_attack", 100 }, { "osc2_sustain", 85 }, { "osc2_release", 400 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 15 },
        { "chorus_mode", 3 }, { "chorus_depth", 80 },
        { "master_analog_drift", 15 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_level", 5 }, { "noise_attack", 100 }, { "noise_sustain", 40 }, { "noise_release", 400 }
    }));

    // ==================== ADDITIONAL PRESETS (51-100) ====================

    // --- PADS (51-60) ---

    presets.push_back (makePreset ("Slow Motion", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 12 },
        { "osc1_attack", 2000 }, { "osc1_decay", 3000 }, { "osc1_sustain", 85 }, { "osc1_release", 4000 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 2500 }, { "osc2_sustain", 85 }, { "osc2_release", 4000 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 15 },
        { "lfo1_depth", 10 }, { "lfo1_rate", 0.15f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 25 },
        { "reverb_size", 70 }, { "reverb_mix", 30 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 2000 }, { "noise_sustain", 70 }, { "noise_release", 4000 }
    }));

    presets.push_back (makePreset ("Cosmic Drift", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 30 },
        { "osc1_attack", 1500 }, { "osc1_sustain", 90 }, { "osc1_release", 5000 },
        { "osc2_waveform", 0 }, { "osc2_octave", -1 }, { "osc2_level", 45 },
        { "osc2_attack", 2000 }, { "osc2_sustain", 90 }, { "osc2_release", 5000 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 30 }, { "filter_env_amount", 25 },
        { "lfo1_depth", 20 }, { "lfo1_rate", 0.08f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 50 },
        { "reverb_size", 85 }, { "reverb_mix", 35 },
        { "noise_type", 2 }, { "noise_level", 10 }, { "noise_attack", 1500 }, { "noise_sustain", 60 }, { "noise_release", 5000 }
    }));

    presets.push_back (makePreset ("Glassy Pad", "Pads", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 8 },
        { "osc1_attack", 500 }, { "osc1_decay", 1000 }, { "osc1_sustain", 75 }, { "osc1_release", 2000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 35 },
        { "osc2_attack", 600 }, { "osc2_sustain", 75 }, { "osc2_release", 2000 },
        { "filter_cutoff", 7000 }, { "filter_resonance", 20 },
        { "chorus_mode", 1 }, { "master_analog_drift", 10 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Dark Matter", "Pads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 30 }, { "osc1_unison_voices", 5 },
        { "osc1_unison_detune", 20 },
        { "osc1_attack", 1000 }, { "osc1_sustain", 70 }, { "osc1_release", 3000 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 60 },
        { "osc2_attack", 1200 }, { "osc2_sustain", 70 }, { "osc2_release", 3000 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 35 }, { "filter_env_amount", 30 },
        { "filter_decay", 800 }, { "filter_sustain", 20 },
        { "chorus_mode", 2 }, { "master_analog_drift", 30 },
        { "reverb_size", 75 }, { "reverb_mix", 30 },
        { "noise_type", 2 }, { "noise_level", 12 }, { "noise_attack", 1000 }, { "noise_sustain", 50 }, { "noise_release", 3000 }
    }));

    presets.push_back (makePreset ("Cathedral", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 18 },
        { "osc1_attack", 3000 }, { "osc1_sustain", 95 }, { "osc1_release", 6000 },
        { "osc2_waveform", 0 }, { "osc2_semitone", 7 }, { "osc2_level", 40 },
        { "osc2_attack", 3500 }, { "osc2_sustain", 95 }, { "osc2_release", 6000 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 10 },
        { "chorus_mode", 3 }, { "master_analog_drift", 20 },
        { "reverb_size", 90 }, { "reverb_mix", 40 },
        { "noise_type", 1 }, { "noise_level", 6 }, { "noise_attack", 3000 }, { "noise_sustain", 80 }, { "noise_release", 6000 }
    }));

    presets.push_back (makePreset ("Underwater", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 1500 }, { "osc1_sustain", 80 }, { "osc1_release", 3000 },
        { "osc2_waveform", 3 }, { "osc2_level", 50 }, { "osc2_fine_tune", -8 },
        { "osc2_attack", 1800 }, { "osc2_sustain", 80 }, { "osc2_release", 3000 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 40 }, { "filter_env_amount", 35 },
        { "filter_attack", 500 }, { "filter_decay", 1500 },
        { "lfo1_depth", 25 }, { "lfo1_rate", 0.2f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 35 },
        { "reverb_size", 70 }, { "reverb_mix", 30 },
        { "noise_type", 2 }, { "noise_level", 10 }, { "noise_attack", 1500 }, { "noise_sustain", 70 }, { "noise_release", 3000 }
    }));

    presets.push_back (makePreset ("Sunrise Strings", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 22 },
        { "osc1_unison_blend", 70 },
        { "osc1_attack", 2000 }, { "osc1_decay", 1500 }, { "osc1_sustain", 85 }, { "osc1_release", 2500 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 5 }, { "osc2_level", 65 },
        { "osc2_unison_voices", 3 }, { "osc2_unison_detune", 15 },
        { "osc2_attack", 2200 }, { "osc2_sustain", 85 }, { "osc2_release", 2500 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 12 }, { "filter_env_amount", 15 },
        { "chorus_mode", 3 }, { "master_analog_drift", 18 },
        { "reverb_size", 60 }, { "reverb_mix", 22 },
        { "noise_type", 1 }, { "noise_level", 5 }, { "noise_attack", 2000 }, { "noise_sustain", 75 }, { "noise_release", 2500 }
    }));

    presets.push_back (makePreset ("Analog Heaven", "Pads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 35 }, { "osc1_unison_voices", 5 },
        { "osc1_unison_detune", 18 },
        { "osc1_attack", 1000 }, { "osc1_sustain", 80 }, { "osc1_release", 2000 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 65 }, { "osc2_level", 50 },
        { "osc2_fine_tune", 4 },
        { "osc2_attack", 1200 }, { "osc2_sustain", 80 }, { "osc2_release", 2000 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 20 },
        { "lfo1_depth", 15 }, { "lfo1_rate", 0.5f }, { "lfo1_dest_pw", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 40 },
        { "reverb_size", 60 }, { "reverb_mix", 22 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Nebula", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 35 },
        { "osc1_attack", 4000 }, { "osc1_sustain", 90 }, { "osc1_release", 8000 },
        { "osc2_waveform", 3 }, { "osc2_octave", -1 }, { "osc2_level", 40 },
        { "osc2_attack", 4500 }, { "osc2_sustain", 90 }, { "osc2_release", 8000 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 15 },
        { "lfo1_depth", 12 }, { "lfo1_rate", 0.05f }, { "lfo1_dest_cutoff", 1 },
        { "lfo2_depth", 8 }, { "lfo2_rate", 0.12f }, { "lfo2_dest_volume", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 60 },
        { "reverb_size", 90 }, { "reverb_mix", 40 },
        { "noise_type", 1 }, { "noise_level", 10 }, { "noise_attack", 4000 }, { "noise_sustain", 70 }, { "noise_release", 8000 }
    }));

    presets.push_back (makePreset ("Frozen Lake", "Pads", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 15 },
        { "osc1_attack", 2500 }, { "osc1_sustain", 85 }, { "osc1_release", 4000 },
        { "osc2_waveform", 0 }, { "osc2_level", 55 }, { "osc2_fine_tune", -3 },
        { "osc2_attack", 3000 }, { "osc2_sustain", 85 }, { "osc2_release", 4000 },
        { "filter_cutoff", 4500 }, { "filter_resonance", 25 }, { "filter_env_amount", 10 },
        { "chorus_mode", 2 }, { "master_analog_drift", 22 },
        { "reverb_size", 75 }, { "reverb_mix", 30 },
        { "noise_type", 3 }, { "noise_level", 5 }, { "noise_attack", 2500 }, { "noise_sustain", 70 }, { "noise_release", 4000 }
    }));

    // --- LEADS (61-70) ---

    presets.push_back (makePreset ("Screamer", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_level", 80 },
        { "osc1_attack", 2 }, { "osc1_decay", 100 }, { "osc1_sustain", 90 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_level", 70 }, { "osc2_semitone", 7 },
        { "osc2_attack", 2 }, { "osc2_sustain", 90 }, { "osc2_release", 100 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 50 }, { "filter_env_amount", 60 },
        { "filter_attack", 2 }, { "filter_decay", 200 }, { "filter_sustain", 50 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_glide", 50 }
    }));

    presets.push_back (makePreset ("Neon Nights", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 40 },
        { "osc1_attack", 5 }, { "osc1_sustain", 85 }, { "osc1_release", 200 },
        { "osc2_waveform", 1 }, { "osc2_level", 60 }, { "osc2_fine_tune", 8 },
        { "osc2_attack", 5 }, { "osc2_sustain", 85 }, { "osc2_release", 200 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 35 }, { "filter_env_amount", 40 },
        { "filter_decay", 300 },
        { "lfo1_depth", 20 }, { "lfo1_rate", 5.0f }, { "lfo1_waveform", 0 }, { "lfo1_dest_pitch", 1 },
        { "chorus_mode", 1 }, { "master_analog_drift", 10 },
        { "reverb_size", 45 }, { "reverb_mix", 12 }
    }));

    presets.push_back (makePreset ("Laser Tag", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_level", 80 },
        { "osc1_attack", 1 }, { "osc1_decay", 80 }, { "osc1_sustain", 70 }, { "osc1_release", 60 },
        { "osc2_waveform", 1 }, { "osc2_octave", 1 }, { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_sustain", 70 }, { "osc2_release", 60 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 60 }, { "filter_env_amount", 70 },
        { "filter_attack", 1 }, { "filter_decay", 150 }, { "filter_sustain", 30 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_glide", 30 }
    }));

    presets.push_back (makePreset ("Soft Whistle", "Leads", {
        { "osc1_waveform", 0 }, { "osc1_level", 70 },
        { "osc1_attack", 50 }, { "osc1_decay", 300 }, { "osc1_sustain", 60 }, { "osc1_release", 400 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 25 },
        { "osc2_attack", 80 }, { "osc2_sustain", 60 }, { "osc2_release", 400 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 20 },
        { "lfo1_depth", 8 }, { "lfo1_rate", 6.0f }, { "lfo1_dest_pitch", 1 },
        { "chorus_mode", 1 }, { "master_analog_drift", 12 },
        { "reverb_size", 55 }, { "reverb_mix", 18 },
        { "noise_level", 5 }, { "noise_attack", 50 }, { "noise_decay", 400 }, { "noise_sustain", 15 }, { "noise_release", 400 }
    }));

    presets.push_back (makePreset ("PWM Lead", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 25 },
        { "osc1_attack", 5 }, { "osc1_sustain", 80 }, { "osc1_release", 150 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 75 }, { "osc2_level", 50 },
        { "osc2_attack", 5 }, { "osc2_sustain", 80 }, { "osc2_release", 150 },
        { "filter_cutoff", 5500 }, { "filter_resonance", 25 }, { "filter_env_amount", 35 },
        { "lfo1_depth", 40 }, { "lfo1_rate", 1.5f }, { "lfo1_dest_pw", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 15 },
        { "reverb_size", 45 }, { "reverb_mix", 12 }
    }));

    presets.push_back (makePreset ("Acid Bite", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_level", 90 },
        { "osc1_attack", 1 }, { "osc1_decay", 150 }, { "osc1_sustain", 50 }, { "osc1_release", 80 },
        { "osc2_level", 0 },
        { "filter_cutoff", 800 }, { "filter_resonance", 80 }, { "filter_env_amount", 90 },
        { "filter_attack", 1 }, { "filter_decay", 200 }, { "filter_sustain", 10 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_glide", 40 },
        { "master_analog_drift", 8 }
    }));

    presets.push_back (makePreset ("Detuned Stack", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 35 },
        { "osc1_attack", 3 }, { "osc1_sustain", 85 }, { "osc1_release", 200 },
        { "osc2_waveform", 2 }, { "osc2_level", 60 }, { "osc2_unison_voices", 3 },
        { "osc2_unison_detune", 25 },
        { "osc2_attack", 3 }, { "osc2_sustain", 85 }, { "osc2_release", 200 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 20 },
        { "chorus_mode", 1 }, { "master_analog_drift", 20 },
        { "reverb_size", 40 }, { "reverb_mix", 10 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Vintage Solo", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_level", 80 },
        { "osc1_attack", 10 }, { "osc1_decay", 200 }, { "osc1_sustain", 70 }, { "osc1_release", 150 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 45 }, { "osc2_level", 50 },
        { "osc2_fine_tune", 6 },
        { "osc2_attack", 10 }, { "osc2_sustain", 70 }, { "osc2_release", 150 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 30 }, { "filter_env_amount", 40 },
        { "filter_decay", 250 }, { "filter_sustain", 40 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_glide", 60 },
        { "master_analog_drift", 35 },
        { "reverb_size", 45 }, { "reverb_mix", 12 }
    }));

    presets.push_back (makePreset ("Synth Brass", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 12 },
        { "osc1_attack", 20 }, { "osc1_decay", 150 }, { "osc1_sustain", 75 }, { "osc1_release", 100 },
        { "osc2_waveform", 1 }, { "osc2_level", 70 }, { "osc2_fine_tune", 3 },
        { "osc2_attack", 20 }, { "osc2_sustain", 75 }, { "osc2_release", 100 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 20 }, { "filter_env_amount", 50 },
        { "filter_attack", 15 }, { "filter_decay", 200 }, { "filter_sustain", 50 },
        { "master_analog_drift", 12 },
        { "reverb_size", 40 }, { "reverb_mix", 10 },
        { "noise_level", 4 }, { "noise_attack", 20 }, { "noise_decay", 200 }, { "noise_sustain", 10 }, { "noise_release", 100 }
    }));

    presets.push_back (makePreset ("Talkbox", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 50 },
        { "osc1_attack", 5 }, { "osc1_sustain", 80 }, { "osc1_release", 120 },
        { "osc2_level", 0 },
        { "filter_cutoff", 1200 }, { "filter_resonance", 70 }, { "filter_env_amount", 60 },
        { "filter_attack", 50 }, { "filter_decay", 300 }, { "filter_sustain", 40 },
        { "lfo1_depth", 30 }, { "lfo1_rate", 3.0f }, { "lfo1_dest_cutoff", 1 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_glide", 80 }
    }));

    // --- BASS (71-78) ---

    presets.push_back (makePreset ("Rubber Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 }, { "osc1_level", 90 },
        { "osc1_attack", 1 }, { "osc1_decay", 300 }, { "osc1_sustain", 60 }, { "osc1_release", 60 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 50 },
        { "osc2_attack", 1 }, { "osc2_sustain", 60 }, { "osc2_release", 60 },
        { "filter_cutoff", 1000 }, { "filter_resonance", 35 }, { "filter_env_amount", 50 },
        { "filter_decay", 200 }, { "filter_sustain", 20 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_glide", 25 },
        { "noise_level", 5 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Wobble Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 }, { "osc1_level", 85 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 50 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_pulse_width", 40 },
        { "osc2_level", 60 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 50 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 55 }, { "filter_env_amount", 40 },
        { "lfo1_depth", 50 }, { "lfo1_rate", 4.0f }, { "lfo1_dest_cutoff", 1 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("Reese Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 }, { "osc1_level", 80 },
        { "osc1_attack", 1 }, { "osc1_sustain", 85 }, { "osc1_release", 80 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 80 },
        { "osc2_fine_tune", 10 },
        { "osc2_attack", 1 }, { "osc2_sustain", 85 }, { "osc2_release", 80 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 20 }, { "filter_env_amount", 30 },
        { "filter_decay", 500 }, { "filter_sustain", 40 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_analog_drift", 8 },
        { "master_unison", 2 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Organ Bass", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -1 }, { "osc1_level", 80 },
        { "osc1_attack", 3 }, { "osc1_decay", 100 }, { "osc1_sustain", 90 }, { "osc1_release", 50 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 60 },
        { "osc2_attack", 3 }, { "osc2_sustain", 90 }, { "osc2_release", 50 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 10 },
        { "master_polyphony", 1 }
    }));

    presets.push_back (makePreset ("Squelch", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 }, { "osc1_level", 90 },
        { "osc1_attack", 1 }, { "osc1_decay", 200 }, { "osc1_sustain", 40 }, { "osc1_release", 40 },
        { "osc2_level", 0 },
        { "filter_cutoff", 500 }, { "filter_resonance", 85 }, { "filter_env_amount", 95 },
        { "filter_attack", 1 }, { "filter_decay", 250 }, { "filter_sustain", 5 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_glide", 20 }
    }));

    presets.push_back (makePreset ("Pluck Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_decay", 250 }, { "osc1_sustain", 0 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_level", 50 },
        { "osc2_attack", 1 }, { "osc2_decay", 250 }, { "osc2_sustain", 0 }, { "osc2_release", 100 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 25 }, { "filter_env_amount", 60 },
        { "filter_attack", 1 }, { "filter_decay", 200 }, { "filter_sustain", 0 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 35 }, { "noise_sustain", 0 }, { "noise_release", 25 }
    }));

    presets.push_back (makePreset ("808 Sub", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -2 }, { "osc1_level", 90 },
        { "osc1_attack", 1 }, { "osc1_decay", 500 }, { "osc1_sustain", 50 }, { "osc1_release", 200 },
        { "osc2_level", 0 },
        { "filter_cutoff", 800 }, { "filter_resonance", 10 },
        { "master_polyphony", 1 }
    }));

    presets.push_back (makePreset ("Distorted Bass", "Bass", {
        { "osc1_waveform", 2 }, { "osc1_octave", -1 }, { "osc1_pulse_width", 30 },
        { "osc1_level", 95 },
        { "osc1_attack", 1 }, { "osc1_decay", 150 }, { "osc1_sustain", 70 }, { "osc1_release", 50 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 80 },
        { "osc2_attack", 1 }, { "osc2_sustain", 70 }, { "osc2_release", 50 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 40 }, { "filter_env_amount", 50 },
        { "filter_decay", 180 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 },
        { "noise_level", 5 }, { "noise_attack", 1 }, { "noise_decay", 60 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    // --- KEYS (79-86) ---

    presets.push_back (makePreset ("Soft EP", "Keys", {
        { "osc1_waveform", 0 }, { "osc1_level", 70 },
        { "osc1_attack", 5 }, { "osc1_decay", 500 }, { "osc1_sustain", 40 }, { "osc1_release", 300 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 20 },
        { "osc2_attack", 5 }, { "osc2_decay", 400 }, { "osc2_sustain", 20 }, { "osc2_release", 300 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 10 }, { "filter_env_amount", 20 },
        { "filter_decay", 400 },
        { "chorus_mode", 1 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_level", 2 }, { "noise_attack", 1 }, { "noise_decay", 80 }, { "noise_sustain", 0 }, { "noise_release", 50 }
    }));

    presets.push_back (makePreset ("Bright Clav", "Keys", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 25 },
        { "osc1_attack", 1 }, { "osc1_decay", 200 }, { "osc1_sustain", 30 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 75 }, { "osc2_level", 50 },
        { "osc2_attack", 1 }, { "osc2_decay", 200 }, { "osc2_sustain", 30 }, { "osc2_release", 80 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 30 }, { "filter_env_amount", 40 },
        { "filter_decay", 150 }, { "filter_sustain", 20 },
        { "reverb_size", 40 }, { "reverb_mix", 12 },
        { "noise_level", 3 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Toy Piano", "Keys", {
        { "osc1_waveform", 3 }, { "osc1_octave", 1 }, { "osc1_level", 60 },
        { "osc1_attack", 1 }, { "osc1_decay", 300 }, { "osc1_sustain", 10 }, { "osc1_release", 200 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 25 },
        { "osc2_attack", 1 }, { "osc2_decay", 200 }, { "osc2_sustain", 0 }, { "osc2_release", 150 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 15 },
        { "reverb_size", 55 }, { "reverb_mix", 20 }
    }));

    presets.push_back (makePreset ("Wurlitzer", "Keys", {
        { "osc1_waveform", 0 }, { "osc1_level", 75 },
        { "osc1_attack", 3 }, { "osc1_decay", 400 }, { "osc1_sustain", 35 }, { "osc1_release", 200 },
        { "osc2_waveform", 3 }, { "osc2_level", 30 }, { "osc2_fine_tune", 4 },
        { "osc2_attack", 3 }, { "osc2_decay", 350 }, { "osc2_sustain", 30 }, { "osc2_release", 200 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 18 }, { "filter_env_amount", 30 },
        { "filter_decay", 350 }, { "filter_sustain", 25 },
        { "lfo1_depth", 5 }, { "lfo1_rate", 5.5f }, { "lfo1_dest_volume", 1 },
        { "chorus_mode", 1 }, { "master_analog_drift", 15 },
        { "reverb_size", 50 }, { "reverb_mix", 18 },
        { "noise_level", 3 }, { "noise_attack", 1 }, { "noise_decay", 70 }, { "noise_sustain", 0 }, { "noise_release", 40 }
    }));

    presets.push_back (makePreset ("Ice Keys", "Keys", {
        { "osc1_waveform", 3 }, { "osc1_level", 65 },
        { "osc1_attack", 2 }, { "osc1_decay", 400 }, { "osc1_sustain", 30 }, { "osc1_release", 600 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 2 }, { "osc2_decay", 300 }, { "osc2_sustain", 15 }, { "osc2_release", 600 },
        { "filter_cutoff", 7000 }, { "filter_resonance", 25 },
        { "chorus_mode", 2 }, { "master_analog_drift", 8 },
        { "reverb_size", 60 }, { "reverb_mix", 22 }
    }));

    presets.push_back (makePreset ("FM-ish Bell", "Keys", {
        { "osc1_waveform", 0 }, { "osc1_level", 70 },
        { "osc1_attack", 1 }, { "osc1_decay", 600 }, { "osc1_sustain", 5 }, { "osc1_release", 500 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_semitone", 7 }, { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_decay", 400 }, { "osc2_sustain", 0 }, { "osc2_release", 400 },
        { "filter_cutoff", 10000 }, { "filter_resonance", 10 },
        { "reverb_size", 65 }, { "reverb_mix", 25 }
    }));

    presets.push_back (makePreset ("Harpsichord", "Keys", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 15 },
        { "osc1_attack", 1 }, { "osc1_decay", 250 }, { "osc1_sustain", 0 }, { "osc1_release", 120 },
        { "osc2_waveform", 2 }, { "osc2_octave", 1 }, { "osc2_pulse_width", 20 },
        { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_decay", 200 }, { "osc2_sustain", 0 }, { "osc2_release", 100 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 15 }, { "filter_env_amount", 30 },
        { "filter_decay", 200 }, { "filter_sustain", 0 },
        { "reverb_size", 50 }, { "reverb_mix", 18 },
        { "noise_level", 4 }, { "noise_attack", 1 }, { "noise_decay", 60 }, { "noise_sustain", 0 }, { "noise_release", 40 }
    }));

    presets.push_back (makePreset ("Mallet Synth", "Keys", {
        { "osc1_waveform", 3 }, { "osc1_level", 65 },
        { "osc1_attack", 1 }, { "osc1_decay", 350 }, { "osc1_sustain", 15 }, { "osc1_release", 250 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 35 },
        { "osc2_attack", 1 }, { "osc2_decay", 250 }, { "osc2_sustain", 5 }, { "osc2_release", 200 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 20 }, { "filter_env_amount", 25 },
        { "filter_decay", 300 },
        { "chorus_mode", 1 },
        { "reverb_size", 55 }, { "reverb_mix", 20 }
    }));

    // --- FX (87-94) ---

    presets.push_back (makePreset ("Alien Comm", "FX", {
        { "osc1_waveform", 1 }, { "osc1_level", 60 },
        { "osc1_attack", 200 }, { "osc1_decay", 500 }, { "osc1_sustain", 40 }, { "osc1_release", 1000 },
        { "osc2_waveform", 2 }, { "osc2_semitone", 5 }, { "osc2_level", 50 },
        { "osc2_attack", 300 }, { "osc2_sustain", 40 }, { "osc2_release", 1000 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 70 }, { "filter_env_amount", 80 },
        { "filter_attack", 100 }, { "filter_decay", 600 }, { "filter_sustain", 15 },
        { "lfo1_depth", 40 }, { "lfo1_rate", 8.0f }, { "lfo1_dest_pitch", 1 },
        { "lfo2_depth", 30 }, { "lfo2_rate", 0.5f }, { "lfo2_dest_cutoff", 1 },
        { "master_analog_drift", 50 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "noise_type", 4 }, { "noise_level", 10 }, { "noise_sh_rate", 5000 }, { "noise_attack", 200 }, { "noise_sustain", 30 }, { "noise_release", 1000 }
    }));

    presets.push_back (makePreset ("Wind Howl", "FX", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 50 },
        { "osc1_level", 40 },
        { "osc1_attack", 3000 }, { "osc1_sustain", 60 }, { "osc1_release", 5000 },
        { "osc2_waveform", 1 }, { "osc2_unison_voices", 7 }, { "osc2_unison_detune", 55 },
        { "osc2_level", 40 }, { "osc2_fine_tune", 15 },
        { "osc2_attack", 3500 }, { "osc2_sustain", 60 }, { "osc2_release", 5000 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 50 },
        { "lfo1_depth", 60 }, { "lfo1_rate", 0.3f }, { "lfo1_dest_cutoff", 1 },
        { "master_analog_drift", 80 },
        { "reverb_size", 70 }, { "reverb_mix", 30 },
        { "noise_type", 1 }, { "noise_level", 30 }, { "noise_attack", 3000 }, { "noise_sustain", 60 }, { "noise_release", 5000 },
        { "master_unison", 3 }
    }));

    presets.push_back (makePreset ("Siren", "FX", {
        { "osc1_waveform", 0 }, { "osc1_level", 70 },
        { "osc1_attack", 5 }, { "osc1_sustain", 90 }, { "osc1_release", 200 },
        { "osc2_level", 0 },
        { "filter_cutoff", 10000 },
        { "lfo1_depth", 80 }, { "lfo1_rate", 2.0f }, { "lfo1_waveform", 3 }, { "lfo1_dest_pitch", 1 },
        { "master_polyphony", 1 },
        { "reverb_size", 50 }, { "reverb_mix", 20 }
    }));

    presets.push_back (makePreset ("Laser Sweep", "FX", {
        { "osc1_waveform", 1 }, { "osc1_level", 80 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 0 }, { "osc1_release", 100 },
        { "osc2_level", 0 },
        { "filter_cutoff", 200 }, { "filter_resonance", 90 }, { "filter_env_amount", 100 },
        { "filter_attack", 1 }, { "filter_decay", 500 }, { "filter_sustain", 0 },
        { "reverb_size", 45 }, { "reverb_mix", 18 }
    }));

    presets.push_back (makePreset ("Metallic Ring", "FX", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 10 },
        { "osc1_attack", 1 }, { "osc1_decay", 800 }, { "osc1_sustain", 0 }, { "osc1_release", 600 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 90 }, { "osc2_semitone", 7 },
        { "osc2_level", 60 },
        { "osc2_attack", 1 }, { "osc2_decay", 700 }, { "osc2_sustain", 0 }, { "osc2_release", 500 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 40 },
        { "reverb_size", 55 }, { "reverb_mix", 22 }
    }));

    presets.push_back (makePreset ("Droplets", "FX", {
        { "osc1_waveform", 0 }, { "osc1_octave", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 150 }, { "osc1_sustain", 0 }, { "osc1_release", 300 },
        { "osc2_waveform", 3 }, { "osc2_octave", 2 }, { "osc2_level", 30 },
        { "osc2_attack", 1 }, { "osc2_decay", 100 }, { "osc2_sustain", 0 }, { "osc2_release", 200 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 30 }, { "filter_env_amount", 40 },
        { "filter_decay", 100 }, { "filter_sustain", 0 },
        { "lfo1_depth", 15 }, { "lfo1_rate", 12.0f }, { "lfo1_dest_pitch", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 25 },
        { "reverb_size", 70 }, { "reverb_mix", 35 },
        { "noise_level", 5 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 100 }
    }));

    presets.push_back (makePreset ("Space Radio", "FX", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 50 }, { "osc1_level", 50 },
        { "osc1_attack", 100 }, { "osc1_sustain", 70 }, { "osc1_release", 500 },
        { "osc2_waveform", 1 }, { "osc2_level", 40 }, { "osc2_fine_tune", 20 },
        { "osc2_attack", 150 }, { "osc2_sustain", 70 }, { "osc2_release", 500 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 60 },
        { "lfo1_depth", 50 }, { "lfo1_rate", 0.1f }, { "lfo1_dest_cutoff", 1 },
        { "lfo2_depth", 20 }, { "lfo2_rate", 7.0f }, { "lfo2_dest_pw", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 70 },
        { "reverb_size", 65 }, { "reverb_mix", 30 },
        { "noise_type", 4 }, { "noise_level", 15 }, { "noise_sh_rate", 8000 }, { "noise_sustain", 50 }, { "noise_release", 500 },
        { "master_unison", 3 }
    }));

    presets.push_back (makePreset ("Thunder Roll", "FX", {
        { "osc1_waveform", 1 }, { "osc1_octave", -2 }, { "osc1_unison_voices", 7 },
        { "osc1_unison_detune", 60 }, { "osc1_level", 60 },
        { "osc1_attack", 500 }, { "osc1_decay", 2000 }, { "osc1_sustain", 20 }, { "osc1_release", 3000 },
        { "osc2_waveform", 1 }, { "osc2_octave", -2 }, { "osc2_unison_voices", 7 },
        { "osc2_unison_detune", 65 }, { "osc2_level", 60 }, { "osc2_fine_tune", 10 },
        { "osc2_attack", 600 }, { "osc2_sustain", 20 }, { "osc2_release", 3000 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 30 }, { "filter_env_amount", 40 },
        { "filter_attack", 200 }, { "filter_decay", 1500 },
        { "master_analog_drift", 90 },
        { "reverb_size", 80 }, { "reverb_mix", 35 },
        { "noise_type", 2 }, { "noise_level", 20 }, { "noise_attack", 500 }, { "noise_decay", 2000 }, { "noise_sustain", 15 }, { "noise_release", 3000 }
    }));

    // --- ARPS / PLUCKS (95-100) ---

    presets.push_back (makePreset ("Arp Pluck", "Plucks", {
        { "osc1_waveform", 1 }, { "osc1_level", 70 },
        { "osc1_attack", 1 }, { "osc1_decay", 180 }, { "osc1_sustain", 0 }, { "osc1_release", 150 },
        { "osc2_waveform", 2 }, { "osc2_level", 40 }, { "osc2_fine_tune", 5 },
        { "osc2_attack", 1 }, { "osc2_decay", 150 }, { "osc2_sustain", 0 }, { "osc2_release", 120 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 20 }, { "filter_env_amount", 50 },
        { "filter_decay", 150 }, { "filter_sustain", 0 },
        { "chorus_mode", 1 },
        { "reverb_size", 50 }, { "reverb_mix", 20 },
        { "noise_level", 10 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Crystal Pluck", "Plucks", {
        { "osc1_waveform", 3 }, { "osc1_octave", 1 }, { "osc1_level", 65 },
        { "osc1_attack", 1 }, { "osc1_decay", 250 }, { "osc1_sustain", 5 }, { "osc1_release", 300 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 25 },
        { "osc2_attack", 1 }, { "osc2_decay", 200 }, { "osc2_sustain", 0 }, { "osc2_release", 250 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 25 }, { "filter_env_amount", 30 },
        { "filter_decay", 200 }, { "filter_sustain", 5 },
        { "chorus_mode", 2 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 40 }
    }));

    presets.push_back (makePreset ("Synth Marimba", "Plucks", {
        { "osc1_waveform", 3 }, { "osc1_level", 70 },
        { "osc1_attack", 1 }, { "osc1_decay", 300 }, { "osc1_sustain", 0 }, { "osc1_release", 200 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 1 }, { "osc2_decay", 200 }, { "osc2_sustain", 0 }, { "osc2_release", 150 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 15 }, { "filter_env_amount", 35 },
        { "filter_decay", 250 }, { "filter_sustain", 0 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 30 }, { "noise_sustain", 0 }, { "noise_release", 25 }
    }));

    presets.push_back (makePreset ("Kalimba", "Plucks", {
        { "osc1_waveform", 0 }, { "osc1_octave", 1 }, { "osc1_level", 60 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 5 }, { "osc1_release", 350 },
        { "osc2_waveform", 3 }, { "osc2_octave", 2 }, { "osc2_level", 20 },
        { "osc2_attack", 1 }, { "osc2_decay", 300 }, { "osc2_sustain", 0 }, { "osc2_release", 300 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 10 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "noise_level", 6 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Pizzicato", "Plucks", {
        { "osc1_waveform", 1 }, { "osc1_level", 75 },
        { "osc1_attack", 1 }, { "osc1_decay", 120 }, { "osc1_sustain", 0 }, { "osc1_release", 80 },
        { "osc2_waveform", 3 }, { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_decay", 100 }, { "osc2_sustain", 0 }, { "osc2_release", 60 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 20 }, { "filter_env_amount", 50 },
        { "filter_decay", 100 }, { "filter_sustain", 0 },
        { "master_analog_drift", 10 },
        { "reverb_size", 50 }, { "reverb_mix", 20 },
        { "noise_level", 10 }, { "noise_attack", 1 }, { "noise_decay", 25 }, { "noise_sustain", 0 }, { "noise_release", 15 }
    }));

    presets.push_back (makePreset ("Gameboy", "Plucks", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 50 }, { "osc1_octave", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 100 }, { "osc1_sustain", 0 }, { "osc1_release", 50 },
        { "osc2_level", 0 },
        { "filter_cutoff", 12000 }, { "filter_resonance", 5 },
        { "filter_slope", 1 }
    }));

    // ==================== PRESETS 101-200 ====================

    // --- BRASS (101-115) ---

    presets.push_back (makePreset ("Classic Brass", "Brass", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 8 },
        { "osc1_attack", 30 }, { "osc1_decay", 200 }, { "osc1_sustain", 80 }, { "osc1_release", 120 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 40 }, { "osc2_level", 60 },
        { "osc2_attack", 25 }, { "osc2_sustain", 80 }, { "osc2_release", 120 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 20 }, { "filter_env_amount", 55 },
        { "filter_attack", 20 }, { "filter_decay", 250 }, { "filter_sustain", 50 },
        { "master_analog_drift", 8 },
        { "reverb_size", 50 }, { "reverb_mix", 15 },
        { "noise_level", 8 }, { "noise_attack", 30 }, { "noise_decay", 200 }, { "noise_sustain", 40 }, { "noise_release", 120 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Fanfare Trumpets", "Brass", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 15 }, { "osc1_decay", 150 }, { "osc1_sustain", 85 }, { "osc1_release", 100 },
        { "osc2_waveform", 1 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 20 }, { "osc2_sustain", 85 }, { "osc2_release", 100 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 25 }, { "filter_env_amount", 60 },
        { "filter_attack", 15 }, { "filter_decay", 200 }, { "filter_sustain", 55 },
        { "master_analog_drift", 5 },
        { "reverb_size", 55 }, { "reverb_mix", 18 },
        { "noise_level", 6 }, { "noise_attack", 15 }, { "noise_decay", 150 }, { "noise_sustain", 35 }, { "noise_release", 100 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Mellow Horn", "Brass", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 60 }, { "osc1_decay", 300 }, { "osc1_sustain", 70 }, { "osc1_release", 200 },
        { "osc2_waveform", 0 }, { "osc2_level", 40 },
        { "osc2_attack", 80 }, { "osc2_sustain", 70 }, { "osc2_release", 200 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 15 }, { "filter_env_amount", 30 },
        { "filter_attack", 50 }, { "filter_decay", 400 }, { "filter_sustain", 40 },
        { "master_analog_drift", 12 },
        { "reverb_size", 55 }, { "reverb_mix", 18 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 60 }, { "noise_decay", 300 }, { "noise_sustain", 40 }, { "noise_release", 200 }
    }));

    presets.push_back (makePreset ("French Horn", "Brass", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 80 }, { "osc1_decay", 400 }, { "osc1_sustain", 75 }, { "osc1_release", 250 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 3 }, { "osc2_level", 45 },
        { "osc2_attack", 100 }, { "osc2_sustain", 75 }, { "osc2_release", 250 },
        { "filter_cutoff", 1800 }, { "filter_resonance", 18 }, { "filter_env_amount", 35 },
        { "filter_attack", 60 }, { "filter_decay", 500 }, { "filter_sustain", 45 },
        { "master_mono_legato", 1 }, { "master_glide", 40 }, { "master_analog_drift", 15 },
        { "reverb_size", 60 }, { "reverb_mix", 20 },
        { "noise_type", 1 }, { "noise_level", 7 }, { "noise_attack", 80 }, { "noise_decay", 400 }, { "noise_sustain", 35 }, { "noise_release", 250 }
    }));

    presets.push_back (makePreset ("Trombone Growl", "Brass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 20 }, { "osc1_decay", 200 }, { "osc1_sustain", 80 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_pulse_width", 35 }, { "osc2_level", 55 },
        { "osc2_attack", 15 }, { "osc2_sustain", 80 }, { "osc2_release", 100 },
        { "filter_cutoff", 2200 }, { "filter_resonance", 30 }, { "filter_env_amount", 50 },
        { "filter_attack", 10 }, { "filter_decay", 300 }, { "filter_sustain", 45 },
        { "master_mono_legato", 1 }, { "master_glide", 50 }, { "master_analog_drift", 10 },
        { "noise_level", 10 }, { "noise_attack", 20 }, { "noise_decay", 200 }, { "noise_sustain", 45 }, { "noise_release", 100 }
    }));

    presets.push_back (makePreset ("Stab Brass", "Brass", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 12 },
        { "osc1_attack", 5 }, { "osc1_decay", 250 }, { "osc1_sustain", 40 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 45 }, { "osc2_level", 50 },
        { "osc2_attack", 5 }, { "osc2_decay", 250 }, { "osc2_sustain", 40 }, { "osc2_release", 80 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 20 }, { "filter_env_amount", 65 },
        { "filter_attack", 3 }, { "filter_decay", 300 }, { "filter_sustain", 20 },
        { "noise_level", 8 }, { "noise_attack", 5 }, { "noise_decay", 250 }, { "noise_sustain", 15 }, { "noise_release", 80 }
    }));

    presets.push_back (makePreset ("Soft Flugelhorn", "Brass", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 100 }, { "osc1_decay", 500 }, { "osc1_sustain", 65 }, { "osc1_release", 300 },
        { "osc2_waveform", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 120 }, { "osc2_sustain", 60 }, { "osc2_release", 300 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 12 }, { "filter_env_amount", 25 },
        { "filter_attack", 80 }, { "filter_decay", 600 }, { "filter_sustain", 35 },
        { "lfo1_depth", 5 }, { "lfo1_rate", 5.0f }, { "lfo1_dest_pitch", 1 },
        { "master_mono_legato", 1 }, { "master_glide", 60 }, { "master_analog_drift", 18 },
        { "reverb_size", 55 }, { "reverb_mix", 18 },
        { "noise_type", 1 }, { "noise_level", 10 }, { "noise_attack", 100 }, { "noise_decay", 500 }, { "noise_sustain", 40 }, { "noise_release", 300 }
    }));

    presets.push_back (makePreset ("Power Brass", "Brass", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 15 },
        { "osc1_attack", 10 }, { "osc1_decay", 150 }, { "osc1_sustain", 90 }, { "osc1_release", 100 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 5 }, { "osc2_level", 60 },
        { "osc2_attack", 10 }, { "osc2_sustain", 90 }, { "osc2_release", 100 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 22 }, { "filter_env_amount", 50 },
        { "filter_attack", 8 }, { "filter_decay", 200 }, { "filter_sustain", 60 },
        { "master_analog_drift", 6 },
        { "reverb_size", 45 }, { "reverb_mix", 12 },
        { "noise_level", 6 }, { "noise_attack", 10 }, { "noise_decay", 150 }, { "noise_sustain", 35 }, { "noise_release", 100 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Muted Trumpet", "Brass", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 30 },
        { "osc1_attack", 15 }, { "osc1_decay", 200 }, { "osc1_sustain", 60 }, { "osc1_release", 100 },
        { "osc2_level", 0 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 40 }, { "filter_env_amount", 45 },
        { "filter_attack", 10 }, { "filter_decay", 250 }, { "filter_sustain", 30 },
        { "master_mono_legato", 1 }, { "master_glide", 30 }, { "master_analog_drift", 10 },
        { "noise_level", 8 }, { "noise_attack", 15 }, { "noise_decay", 200 }, { "noise_sustain", 30 }, { "noise_release", 100 }
    }));

    presets.push_back (makePreset ("Brass Ensemble", "Brass", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 40 }, { "osc1_decay", 300 }, { "osc1_sustain", 80 }, { "osc1_release", 200 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 45 }, { "osc2_level", 45 },
        { "osc2_unison_voices", 3 }, { "osc2_unison_detune", 8 },
        { "osc2_attack", 50 }, { "osc2_sustain", 80 }, { "osc2_release", 200 },
        { "filter_cutoff", 2800 }, { "filter_resonance", 18 }, { "filter_env_amount", 40 },
        { "filter_attack", 30 }, { "filter_decay", 400 }, { "filter_sustain", 50 },
        { "chorus_mode", 1 }, { "master_analog_drift", 12 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_level", 7 }, { "noise_attack", 40 }, { "noise_decay", 300 }, { "noise_sustain", 40 }, { "noise_release", 200 },
        { "master_unison", 3 }
    }));

    presets.push_back (makePreset ("Sax-ish Lead", "Brass", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 20 }, { "osc1_decay", 150 }, { "osc1_sustain", 75 }, { "osc1_release", 120 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 35 }, { "osc2_level", 40 },
        { "osc2_attack", 25 }, { "osc2_sustain", 75 }, { "osc2_release", 120 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 35 }, { "filter_env_amount", 45 },
        { "filter_attack", 15 }, { "filter_decay", 300 }, { "filter_sustain", 40 },
        { "lfo1_depth", 6 }, { "lfo1_rate", 5.5f }, { "lfo1_dest_pitch", 1 },
        { "master_mono_legato", 1 }, { "master_glide", 50 }, { "master_analog_drift", 15 },
        { "reverb_size", 50 }, { "reverb_mix", 15 },
        { "noise_level", 12 }, { "noise_attack", 20 }, { "noise_decay", 150 }, { "noise_sustain", 45 }, { "noise_release", 120 }
    }));

    presets.push_back (makePreset ("Tuba", "Brass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -2 },
        { "osc1_attack", 40 }, { "osc1_decay", 300 }, { "osc1_sustain", 70 }, { "osc1_release", 150 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 50 },
        { "osc2_attack", 50 }, { "osc2_sustain", 70 }, { "osc2_release", 150 },
        { "filter_cutoff", 1200 }, { "filter_resonance", 20 }, { "filter_env_amount", 30 },
        { "filter_attack", 30 }, { "filter_decay", 400 }, { "filter_sustain", 35 },
        { "master_mono_legato", 1 }, { "master_analog_drift", 10 },
        { "noise_type", 1 }, { "noise_level", 6 }, { "noise_attack", 40 }, { "noise_decay", 300 }, { "noise_sustain", 30 }, { "noise_release", 150 }
    }));

    presets.push_back (makePreset ("80s Brass Hit", "Brass", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 18 },
        { "osc1_attack", 3 }, { "osc1_decay", 300 }, { "osc1_sustain", 30 }, { "osc1_release", 100 },
        { "osc2_waveform", 1 }, { "osc2_level", 50 }, { "osc2_fine_tune", 6 },
        { "osc2_attack", 3 }, { "osc2_decay", 300 }, { "osc2_sustain", 30 }, { "osc2_release", 100 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 15 }, { "filter_env_amount", 70 },
        { "filter_attack", 2 }, { "filter_decay", 350 }, { "filter_sustain", 15 },
        { "chorus_mode", 2 },
        { "reverb_size", 45 }, { "reverb_mix", 15 },
        { "noise_level", 6 }, { "noise_attack", 3 }, { "noise_decay", 300 }, { "noise_sustain", 10 }, { "noise_release", 100 }
    }));

    presets.push_back (makePreset ("Warm Cornet", "Brass", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 30 }, { "osc1_decay", 250 }, { "osc1_sustain", 70 }, { "osc1_release", 150 },
        { "osc2_waveform", 0 }, { "osc2_level", 35 }, { "osc2_fine_tune", 2 },
        { "osc2_attack", 40 }, { "osc2_sustain", 70 }, { "osc2_release", 150 },
        { "filter_cutoff", 2200 }, { "filter_resonance", 15 }, { "filter_env_amount", 35 },
        { "filter_attack", 25 }, { "filter_decay", 350 }, { "filter_sustain", 40 },
        { "lfo1_depth", 4 }, { "lfo1_rate", 5.0f }, { "lfo1_dest_pitch", 1 },
        { "master_mono_legato", 1 }, { "master_glide", 35 }, { "master_analog_drift", 12 },
        { "reverb_size", 50 }, { "reverb_mix", 18 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 30 }, { "noise_decay", 250 }, { "noise_sustain", 35 }, { "noise_release", 150 }
    }));

    presets.push_back (makePreset ("Synth Horns", "Brass", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 12 },
        { "osc1_attack", 25 }, { "osc1_decay", 200 }, { "osc1_sustain", 80 }, { "osc1_release", 120 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 40 }, { "osc2_level", 55 },
        { "osc2_attack", 30 }, { "osc2_sustain", 80 }, { "osc2_release", 120 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 22 }, { "filter_env_amount", 45 },
        { "filter_attack", 20 }, { "filter_decay", 300 }, { "filter_sustain", 50 },
        { "chorus_mode", 1 }, { "master_analog_drift", 10 },
        { "reverb_size", 50 }, { "reverb_mix", 15 },
        { "noise_level", 6 }, { "noise_attack", 25 }, { "noise_decay", 200 }, { "noise_sustain", 35 }, { "noise_release", 120 },
        { "master_unison", 2 }
    }));

    // --- PADS (116-130) ---

    presets.push_back (makePreset ("Vintage Choir", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 14 },
        { "osc1_attack", 1500 }, { "osc1_sustain", 90 }, { "osc1_release", 3000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 35 },
        { "osc2_unison_voices", 3 }, { "osc2_unison_detune", 10 },
        { "osc2_attack", 1800 }, { "osc2_sustain", 90 }, { "osc2_release", 3000 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 12 },
        { "lfo1_depth", 8 }, { "lfo1_rate", 0.3f }, { "lfo1_dest_volume", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 20 },
        { "reverb_size", 65 }, { "reverb_mix", 25 },
        { "noise_level", 5 }, { "noise_attack", 1500 }, { "noise_sustain", 30 }, { "noise_release", 3000 }
    }));

    presets.push_back (makePreset ("Ocean Wash", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 25 },
        { "osc1_attack", 3000 }, { "osc1_sustain", 85 }, { "osc1_release", 6000 },
        { "osc2_waveform", 3 }, { "osc2_level", 40 }, { "osc2_fine_tune", -5 },
        { "osc2_attack", 3500 }, { "osc2_sustain", 85 }, { "osc2_release", 6000 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 20 },
        { "lfo1_depth", 30 }, { "lfo1_rate", 0.1f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 40 },
        { "reverb_size", 85 }, { "reverb_mix", 35 },
        { "noise_type", 1 }, { "noise_level", 12 }, { "noise_attack", 3000 }, { "noise_sustain", 70 }, { "noise_release", 6000 }
    }));

    presets.push_back (makePreset ("Twilight Zone", "Pads", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 20 },
        { "osc1_attack", 2000 }, { "osc1_sustain", 75 }, { "osc1_release", 4000 },
        { "osc2_waveform", 0 }, { "osc2_octave", -1 }, { "osc2_level", 50 },
        { "osc2_attack", 2500 }, { "osc2_sustain", 75 }, { "osc2_release", 4000 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 25 }, { "filter_env_amount", 20 },
        { "lfo1_depth", 15 }, { "lfo1_rate", 0.08f }, { "lfo1_dest_pitch", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 35 },
        { "reverb_size", 75 }, { "reverb_mix", 30 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 2000 }, { "noise_sustain", 60 }, { "noise_release", 4000 }
    }));

    presets.push_back (makePreset ("Molten Core", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 30 },
        { "osc1_attack", 1000 }, { "osc1_sustain", 70 }, { "osc1_release", 2500 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 55 },
        { "osc2_attack", 1200 }, { "osc2_sustain", 70 }, { "osc2_release", 2500 },
        { "filter_cutoff", 1800 }, { "filter_resonance", 40 }, { "filter_env_amount", 35 },
        { "filter_decay", 1000 }, { "filter_sustain", 25 },
        { "lfo1_depth", 20 }, { "lfo1_rate", 0.15f }, { "lfo1_dest_cutoff", 1 },
        { "master_analog_drift", 30 },
        { "reverb_size", 65 }, { "reverb_mix", 25 },
        { "noise_type", 2 }, { "noise_level", 10 }, { "noise_attack", 1000 }, { "noise_sustain", 50 }, { "noise_release", 2500 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Starfield", "Pads", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 18 },
        { "osc1_attack", 4000 }, { "osc1_sustain", 90 }, { "osc1_release", 7000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 20 },
        { "osc2_attack", 4500 }, { "osc2_sustain", 90 }, { "osc2_release", 7000 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 10 },
        { "lfo1_depth", 8 }, { "lfo1_rate", 0.05f }, { "lfo1_dest_volume", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 25 },
        { "reverb_size", 85 }, { "reverb_mix", 35 },
        { "noise_type", 3 }, { "noise_level", 6 }, { "noise_attack", 4000 }, { "noise_sustain", 80 }, { "noise_release", 7000 }
    }));

    presets.push_back (makePreset ("Midnight Velvet", "Pads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 35 }, { "osc1_unison_voices", 3 },
        { "osc1_unison_detune", 10 },
        { "osc1_attack", 800 }, { "osc1_sustain", 85 }, { "osc1_release", 2000 },
        { "osc2_waveform", 0 }, { "osc2_level", 45 }, { "osc2_fine_tune", -4 },
        { "osc2_attack", 1000 }, { "osc2_sustain", 85 }, { "osc2_release", 2000 },
        { "filter_cutoff", 2200 }, { "filter_resonance", 15 },
        { "lfo1_depth", 12 }, { "lfo1_rate", 0.4f }, { "lfo1_dest_pw", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 18 },
        { "reverb_size", 60 }, { "reverb_mix", 22 },
        { "noise_type", 1 }, { "noise_level", 6 }, { "noise_attack", 800 }, { "noise_sustain", 70 }, { "noise_release", 2000 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Prism", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 15 },
        { "osc1_attack", 1200 }, { "osc1_sustain", 80 }, { "osc1_release", 2500 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 1500 }, { "osc2_sustain", 80 }, { "osc2_release", 2500 },
        { "filter_cutoff", 4500 }, { "filter_resonance", 20 }, { "filter_env_amount", 15 },
        { "chorus_mode", 3 }, { "master_analog_drift", 15 },
        { "reverb_size", 65 }, { "reverb_mix", 25 }
    }));

    presets.push_back (makePreset ("Solar Wind", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 40 },
        { "osc1_attack", 5000 }, { "osc1_sustain", 70 }, { "osc1_release", 8000 },
        { "osc2_waveform", 0 }, { "osc2_level", 35 },
        { "osc2_attack", 5500 }, { "osc2_sustain", 70 }, { "osc2_release", 8000 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 25 },
        { "lfo1_depth", 20 }, { "lfo1_rate", 0.06f }, { "lfo1_dest_cutoff", 1 },
        { "lfo2_depth", 10 }, { "lfo2_rate", 0.1f }, { "lfo2_dest_volume", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 55 },
        { "reverb_size", 85 }, { "reverb_mix", 35 },
        { "noise_type", 1 }, { "noise_level", 10 }, { "noise_attack", 5000 }, { "noise_sustain", 60 }, { "noise_release", 8000 }
    }));

    presets.push_back (makePreset ("Ethereal Strings", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 12 },
        { "osc1_unison_blend", 60 },
        { "osc1_attack", 1500 }, { "osc1_sustain", 85 }, { "osc1_release", 3000 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", -6 }, { "osc2_level", 55 },
        { "osc2_unison_voices", 3 }, { "osc2_unison_detune", 8 },
        { "osc2_attack", 1800 }, { "osc2_sustain", 85 }, { "osc2_release", 3000 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 10 }, { "filter_env_amount", 10 },
        { "chorus_mode", 2 }, { "master_analog_drift", 15 },
        { "reverb_size", 60 }, { "reverb_mix", 22 },
        { "noise_type", 1 }, { "noise_level", 5 }, { "noise_attack", 1500 }, { "noise_sustain", 70 }, { "noise_release", 3000 }
    }));

    presets.push_back (makePreset ("Morning Mist", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 8 },
        { "osc1_attack", 2500 }, { "osc1_sustain", 90 }, { "osc1_release", 4000 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 25 },
        { "osc2_attack", 3000 }, { "osc2_sustain", 90 }, { "osc2_release", 4000 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 8 },
        { "chorus_mode", 1 }, { "reverb_size", 70 }, { "reverb_mix", 30 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 2500 }, { "noise_sustain", 80 }, { "noise_release", 4000 },
        { "master_analog_drift", 10 }
    }));

    presets.push_back (makePreset ("Deep Space", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 30 },
        { "osc1_attack", 3000 }, { "osc1_sustain", 60 }, { "osc1_release", 6000 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 40 },
        { "osc2_attack", 3500 }, { "osc2_sustain", 60 }, { "osc2_release", 6000 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 30 }, { "filter_env_amount", 20 },
        { "filter_decay", 2000 },
        { "lfo1_depth", 15 }, { "lfo1_rate", 0.05f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 3 }, { "reverb_size", 80 }, { "reverb_mix", 40 },
        { "noise_type", 2 }, { "noise_level", 10 }, { "noise_attack", 3000 }, { "noise_sustain", 50 }, { "noise_release", 6000 },
        { "master_analog_drift", 45 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Arctic Shimmer", "Pads", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 12 },
        { "osc1_attack", 1000 }, { "osc1_sustain", 85 }, { "osc1_release", 3000 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_fine_tune", 5 },
        { "osc2_attack", 1200 }, { "osc2_sustain", 85 }, { "osc2_release", 3000 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 18 },
        { "chorus_mode", 2 }, { "reverb_size", 65 }, { "reverb_mix", 25 },
        { "noise_type", 3 }, { "noise_level", 5 }, { "noise_attack", 1000 }, { "noise_sustain", 70 }, { "noise_release", 3000 },
        { "master_analog_drift", 12 }
    }));

    presets.push_back (makePreset ("Haunted Organ", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 3 },
        { "osc1_attack", 500 }, { "osc1_sustain", 95 }, { "osc1_release", 1500 },
        { "osc2_waveform", 0 }, { "osc2_octave", -1 }, { "osc2_level", 60 },
        { "osc2_attack", 600 }, { "osc2_sustain", 95 }, { "osc2_release", 1500 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 20 },
        { "lfo1_depth", 5 }, { "lfo1_rate", 6.0f }, { "lfo1_dest_volume", 1 },
        { "chorus_mode", 3 }, { "reverb_size", 85 }, { "reverb_mix", 35 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 500 }, { "noise_sustain", 80 }, { "noise_release", 1500 },
        { "master_analog_drift", 25 }
    }));

    presets.push_back (makePreset ("Synthwave Wash", "Pads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 20 },
        { "osc1_attack", 800 }, { "osc1_sustain", 80 }, { "osc1_release", 2000 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 40 }, { "osc2_level", 40 },
        { "osc2_fine_tune", 3 },
        { "osc2_attack", 1000 }, { "osc2_sustain", 80 }, { "osc2_release", 2000 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 25 }, { "filter_env_amount", 20 },
        { "chorus_mode", 3 }, { "master_analog_drift", 15 },
        { "reverb_size", 60 }, { "reverb_mix", 22 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Phantom Breath", "Pads", {
        { "osc1_waveform", 0 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 22 },
        { "osc1_attack", 4000 }, { "osc1_sustain", 80 }, { "osc1_release", 6000 },
        { "osc2_waveform", 3 }, { "osc2_level", 30 },
        { "osc2_attack", 4500 }, { "osc2_sustain", 80 }, { "osc2_release", 6000 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 15 },
        { "lfo1_depth", 10 }, { "lfo1_rate", 0.12f }, { "lfo1_dest_cutoff", 1 },
        { "lfo2_depth", 5 }, { "lfo2_rate", 0.08f }, { "lfo2_dest_volume", 1 },
        { "chorus_mode", 3 }, { "reverb_size", 75 }, { "reverb_mix", 30 },
        { "noise_type", 1 }, { "noise_level", 10 }, { "noise_attack", 4000 }, { "noise_sustain", 70 }, { "noise_release", 6000 },
        { "master_analog_drift", 30 }
    }));

    // --- LEADS (131-145) ---

    presets.push_back (makePreset ("Razor Lead", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 8 },
        { "osc1_attack", 2 }, { "osc1_sustain", 85 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 30 }, { "osc2_level", 50 },
        { "osc2_attack", 2 }, { "osc2_sustain", 85 }, { "osc2_release", 80 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 30 }, { "filter_env_amount", 40 },
        { "filter_decay", 200 },
        { "master_mono_legato", 1 }, { "master_glide", 25 }
    }));

    presets.push_back (makePreset ("Warm Analog Solo", "Leads", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 10 }, { "osc1_decay", 250 }, { "osc1_sustain", 70 }, { "osc1_release", 150 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 8 }, { "osc2_level", 45 },
        { "osc2_attack", 10 }, { "osc2_sustain", 70 }, { "osc2_release", 150 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 25 }, { "filter_env_amount", 35 },
        { "filter_decay", 300 }, { "filter_sustain", 40 },
        { "lfo1_depth", 5 }, { "lfo1_rate", 5.0f }, { "lfo1_dest_pitch", 1 },
        { "master_mono_legato", 1 }, { "master_glide", 60 }, { "master_analog_drift", 20 },
        { "reverb_size", 50 }, { "reverb_mix", 15 }
    }));

    presets.push_back (makePreset ("Distortion Lead", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 15 },
        { "osc1_attack", 2 }, { "osc1_sustain", 80 }, { "osc1_release", 60 },
        { "osc2_waveform", 1 }, { "osc2_level", 60 }, { "osc2_fine_tune", 12 },
        { "osc2_attack", 2 }, { "osc2_sustain", 80 }, { "osc2_release", 60 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 15 },
        { "master_mono_legato", 1 }, { "master_glide", 20 }
    }));

    presets.push_back (makePreset ("Crystal Flute", "Leads", {
        { "osc1_waveform", 0 }, { "osc1_octave", 1 },
        { "osc1_attack", 50 }, { "osc1_decay", 300 }, { "osc1_sustain", 60 }, { "osc1_release", 400 },
        { "osc2_waveform", 3 }, { "osc2_octave", 2 }, { "osc2_level", 20 },
        { "osc2_attack", 60 }, { "osc2_sustain", 60 }, { "osc2_release", 400 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 10 },
        { "lfo1_depth", 6 }, { "lfo1_rate", 5.5f }, { "lfo1_dest_pitch", 1 },
        { "chorus_mode", 1 }, { "reverb_size", 60 }, { "reverb_mix", 20 },
        { "master_mono_legato", 1 }, { "master_glide", 70 }, { "master_analog_drift", 8 },
        { "noise_level", 4 }, { "noise_attack", 50 }, { "noise_decay", 400 }, { "noise_sustain", 15 }, { "noise_release", 400 }
    }));

    presets.push_back (makePreset ("Gritty Mono", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_level", 70 },
        { "osc1_attack", 1 }, { "osc1_sustain", 75 }, { "osc1_release", 60 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 25 }, { "osc2_level", 50 },
        { "osc2_fine_tune", 15 },
        { "osc2_attack", 1 }, { "osc2_sustain", 75 }, { "osc2_release", 60 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 35 }, { "filter_env_amount", 50 },
        { "filter_decay", 200 }, { "filter_sustain", 30 },
        { "master_mono_legato", 1 }, { "master_glide", 30 }, { "master_analog_drift", 12 }
    }));

    presets.push_back (makePreset ("Octave Lead", "Leads", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 3 }, { "osc1_sustain", 80 }, { "osc1_release", 100 },
        { "osc2_waveform", 1 }, { "osc2_octave", 1 }, { "osc2_level", 40 },
        { "osc2_attack", 3 }, { "osc2_sustain", 80 }, { "osc2_release", 100 },
        { "filter_cutoff", 4500 }, { "filter_resonance", 20 }, { "filter_env_amount", 45 },
        { "filter_decay", 200 }, { "filter_sustain", 30 },
        { "master_mono_legato", 1 }, { "master_glide", 40 }
    }));

    presets.push_back (makePreset ("Resonant Sweep Lead", "Leads", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 5 }, { "osc1_sustain", 80 }, { "osc1_release", 120 },
        { "osc2_level", 0 },
        { "filter_cutoff", 800 }, { "filter_resonance", 65 }, { "filter_env_amount", 80 },
        { "filter_attack", 5 }, { "filter_decay", 400 }, { "filter_sustain", 20 },
        { "master_mono_legato", 1 }, { "master_glide", 50 }, { "master_analog_drift", 8 },
        { "reverb_size", 45 }, { "reverb_mix", 12 }
    }));

    presets.push_back (makePreset ("Portamento Sine", "Leads", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 15 }, { "osc1_sustain", 90 }, { "osc1_release", 200 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 25 },
        { "osc2_attack", 20 }, { "osc2_sustain", 90 }, { "osc2_release", 200 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 8 },
        { "master_mono_legato", 1 }, { "master_glide", 100 }, { "master_analog_drift", 5 },
        { "reverb_size", 50 }, { "reverb_mix", 15 }
    }));

    presets.push_back (makePreset ("Fifth Lead", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 45 },
        { "osc1_attack", 5 }, { "osc1_sustain", 75 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 45 }, { "osc2_semitone", 7 },
        { "osc2_level", 45 },
        { "osc2_attack", 5 }, { "osc2_sustain", 75 }, { "osc2_release", 100 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 20 }, { "filter_env_amount", 40 },
        { "filter_decay", 300 }, { "filter_sustain", 30 },
        { "chorus_mode", 1 },
        { "reverb_size", 45 }, { "reverb_mix", 12 }
    }));

    presets.push_back (makePreset ("Supersaw Lead", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 20 },
        { "osc1_attack", 2 }, { "osc1_sustain", 85 }, { "osc1_release", 100 },
        { "osc2_level", 0 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 15 }, { "filter_env_amount", 30 },
        { "filter_decay", 300 },
        { "master_analog_drift", 5 },
        { "reverb_size", 40 }, { "reverb_mix", 10 },
        { "master_unison", 3 }
    }));

    presets.push_back (makePreset ("Pitch Drift Lead", "Leads", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 5 }, { "osc1_sustain", 80 }, { "osc1_release", 120 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 12 }, { "osc2_level", 45 },
        { "osc2_attack", 5 }, { "osc2_sustain", 80 }, { "osc2_release", 120 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 25 }, { "filter_env_amount", 35 },
        { "filter_decay", 250 },
        { "master_mono_legato", 1 }, { "master_glide", 40 }, { "master_analog_drift", 40 }
    }));

    presets.push_back (makePreset ("Chiptune Lead", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 50 },
        { "osc1_attack", 1 }, { "osc1_sustain", 90 }, { "osc1_release", 30 },
        { "osc2_level", 0 },
        { "filter_cutoff", 15000 }, { "filter_resonance", 5 }, { "filter_slope", 1 }
    }));

    presets.push_back (makePreset ("Nasal Lead", "Leads", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 15 },
        { "osc1_attack", 3 }, { "osc1_sustain", 80 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 85 }, { "osc2_level", 40 },
        { "osc2_attack", 3 }, { "osc2_sustain", 80 }, { "osc2_release", 80 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 50 }, { "filter_env_amount", 40 },
        { "filter_decay", 200 }, { "filter_sustain", 35 },
        { "master_mono_legato", 1 }, { "master_glide", 25 }
    }));

    presets.push_back (makePreset ("Glide Swell", "Leads", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 10 },
        { "osc1_attack", 200 }, { "osc1_sustain", 85 }, { "osc1_release", 300 },
        { "osc2_waveform", 0 }, { "osc2_level", 35 },
        { "osc2_attack", 250 }, { "osc2_sustain", 85 }, { "osc2_release", 300 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 20 }, { "filter_env_amount", 40 },
        { "filter_attack", 150 }, { "filter_decay", 400 }, { "filter_sustain", 45 },
        { "master_mono_legato", 1 }, { "master_glide", 80 }, { "master_analog_drift", 12 },
        { "reverb_size", 50 }, { "reverb_mix", 15 }
    }));

    // --- BASS (146-160) ---

    presets.push_back (makePreset ("Mono Sub", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -2 },
        { "osc1_attack", 1 }, { "osc1_decay", 150 }, { "osc1_sustain", 80 }, { "osc1_release", 40 },
        { "osc2_waveform", 0 }, { "osc2_octave", -1 }, { "osc2_level", 30 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 40 },
        { "filter_cutoff", 400 }, { "filter_resonance", 10 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("Growl Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_decay", 200 }, { "osc1_sustain", 60 }, { "osc1_release", 50 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_pulse_width", 30 },
        { "osc2_level", 55 },
        { "osc2_attack", 1 }, { "osc2_sustain", 60 }, { "osc2_release", 50 },
        { "filter_cutoff", 1000 }, { "filter_resonance", 45 }, { "filter_env_amount", 60 },
        { "filter_decay", 200 }, { "filter_sustain", 15 },
        { "lfo1_depth", 30 }, { "lfo1_rate", 5.0f }, { "lfo1_dest_cutoff", 1 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("Sine Bass", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -1 },
        { "osc1_attack", 2 }, { "osc1_decay", 300 }, { "osc1_sustain", 70 }, { "osc1_release", 60 },
        { "osc2_level", 0 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 8 },
        { "master_polyphony", 1 }
    }));

    presets.push_back (makePreset ("FM Bass", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_decay", 250 }, { "osc1_sustain", 40 }, { "osc1_release", 60 },
        { "osc2_waveform", 0 }, { "osc2_semitone", 7 }, { "osc2_level", 45 },
        { "osc2_attack", 1 }, { "osc2_decay", 150 }, { "osc2_sustain", 10 }, { "osc2_release", 40 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 15 }, { "filter_env_amount", 40 },
        { "filter_decay", 200 }, { "filter_sustain", 10 },
        { "master_polyphony", 1 }
    }));

    presets.push_back (makePreset ("Detune Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 60 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_fine_tune", 12 },
        { "osc2_level", 45 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 60 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 20 }, { "filter_env_amount", 30 },
        { "filter_decay", 300 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_analog_drift", 10 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Percussive Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_decay", 100 }, { "osc1_sustain", 20 }, { "osc1_release", 40 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 50 },
        { "osc2_attack", 1 }, { "osc2_decay", 80 }, { "osc2_sustain", 10 }, { "osc2_release", 30 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 20 }, { "filter_env_amount", 60 },
        { "filter_decay", 80 }, { "filter_sustain", 5 },
        { "filter_velocity", 50 },
        { "noise_level", 10 }, { "noise_attack", 1 }, { "noise_decay", 30 }, { "noise_sustain", 0 }, { "noise_release", 20 }
    }));

    presets.push_back (makePreset ("Pulse Bass", "Bass", {
        { "osc1_waveform", 2 }, { "osc1_octave", -1 }, { "osc1_pulse_width", 25 },
        { "osc1_attack", 1 }, { "osc1_sustain", 75 }, { "osc1_release", 50 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_pulse_width", 75 },
        { "osc2_level", 45 },
        { "osc2_attack", 1 }, { "osc2_sustain", 75 }, { "osc2_release", 50 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 30 }, { "filter_env_amount", 45 },
        { "filter_decay", 250 }, { "filter_sustain", 20 },
        { "master_polyphony", 1 }
    }));

    presets.push_back (makePreset ("Acid Screech", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_sustain", 50 }, { "osc1_release", 40 },
        { "osc2_level", 0 },
        { "filter_cutoff", 300 }, { "filter_resonance", 90 }, { "filter_env_amount", 100 },
        { "filter_decay", 350 }, { "filter_sustain", 5 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_glide", 30 }
    }));

    presets.push_back (makePreset ("Unison Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 }, { "osc1_unison_voices", 3 },
        { "osc1_unison_detune", 8 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 60 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 60 },
        { "filter_cutoff", 1800 }, { "filter_resonance", 25 }, { "filter_env_amount", 40 },
        { "filter_decay", 250 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Tape Saturated", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 2 }, { "osc1_decay", 200 }, { "osc1_sustain", 65 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_octave", -1 }, { "osc2_pulse_width", 40 },
        { "osc2_level", 50 },
        { "osc2_attack", 2 }, { "osc2_sustain", 65 }, { "osc2_release", 80 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 18 }, { "filter_env_amount", 35 },
        { "filter_decay", 300 },
        { "chorus_mode", 1 }, { "master_polyphony", 1 }, { "master_analog_drift", 15 },
        { "noise_type", 1 }, { "noise_level", 3 }, { "noise_attack", 2 }, { "noise_sustain", 50 }, { "noise_release", 80 }
    }));

    presets.push_back (makePreset ("Filtered Sweep Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 60 },
        { "osc2_level", 0 },
        { "filter_cutoff", 400 }, { "filter_resonance", 50 }, { "filter_env_amount", 70 },
        { "filter_attack", 1 }, { "filter_decay", 500 }, { "filter_sustain", 10 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }, { "master_glide", 20 }
    }));

    presets.push_back (makePreset ("Octave Stack Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -2 },
        { "osc1_attack", 1 }, { "osc1_sustain", 75 }, { "osc1_release", 50 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_sustain", 75 }, { "osc2_release", 50 },
        { "filter_cutoff", 1500 }, { "filter_resonance", 20 }, { "filter_env_amount", 40 },
        { "filter_decay", 200 },
        { "master_polyphony", 1 }, { "master_mono_legato", 1 }
    }));

    presets.push_back (makePreset ("Warm Finger Bass", "Bass", {
        { "osc1_waveform", 0 }, { "osc1_octave", -1 },
        { "osc1_attack", 3 }, { "osc1_decay", 400 }, { "osc1_sustain", 50 }, { "osc1_release", 100 },
        { "osc2_waveform", 3 }, { "osc2_octave", -1 }, { "osc2_level", 35 },
        { "osc2_attack", 3 }, { "osc2_decay", 350 }, { "osc2_sustain", 40 }, { "osc2_release", 80 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 12 }, { "filter_env_amount", 30 },
        { "filter_decay", 300 },
        { "master_analog_drift", 8 }
    }));

    presets.push_back (makePreset ("Stepper Bass", "Bass", {
        { "osc1_waveform", 2 }, { "osc1_octave", -1 }, { "osc1_pulse_width", 50 },
        { "osc1_attack", 1 }, { "osc1_decay", 80 }, { "osc1_sustain", 60 }, { "osc1_release", 30 },
        { "osc2_level", 0 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 30 }, { "filter_env_amount", 50 },
        { "filter_decay", 100 }, { "filter_sustain", 10 },
        { "master_polyphony", 1 }
    }));

    presets.push_back (makePreset ("Chorus Bass", "Bass", {
        { "osc1_waveform", 1 }, { "osc1_octave", -1 },
        { "osc1_attack", 2 }, { "osc1_sustain", 80 }, { "osc1_release", 80 },
        { "osc2_waveform", 1 }, { "osc2_octave", -1 }, { "osc2_fine_tune", 6 },
        { "osc2_level", 45 },
        { "osc2_attack", 2 }, { "osc2_sustain", 80 }, { "osc2_release", 80 },
        { "filter_cutoff", 1800 }, { "filter_resonance", 15 },
        { "chorus_mode", 2 }, { "master_polyphony", 1 }, { "master_analog_drift", 12 },
        { "master_unison", 2 }
    }));

    // --- KEYS (161-172) ---

    presets.push_back (makePreset ("Grand Piano", "Keys", {
        { "osc1_waveform", 3 },
        { "osc1_attack", 2 }, { "osc1_decay", 800 }, { "osc1_sustain", 30 }, { "osc1_release", 400 },
        { "osc2_waveform", 0 }, { "osc2_level", 45 },
        { "osc2_attack", 2 }, { "osc2_decay", 600 }, { "osc2_sustain", 20 }, { "osc2_release", 350 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 10 }, { "filter_env_amount", 25 },
        { "filter_decay", 500 }, { "filter_sustain", 15 },
        { "filter_velocity", 50 },
        { "reverb_size", 60 }, { "reverb_mix", 22 },
        { "noise_level", 3 }, { "noise_attack", 1 }, { "noise_decay", 60 }, { "noise_sustain", 0 }, { "noise_release", 40 }
    }));

    presets.push_back (makePreset ("Celesta", "Keys", {
        { "osc1_waveform", 0 }, { "osc1_octave", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 500 }, { "osc1_sustain", 5 }, { "osc1_release", 400 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 30 },
        { "osc2_attack", 1 }, { "osc2_decay", 350 }, { "osc2_sustain", 0 }, { "osc2_release", 300 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 8 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "noise_level", 2 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Rhodes Warm", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 3 }, { "osc1_decay", 500 }, { "osc1_sustain", 35 }, { "osc1_release", 250 },
        { "osc2_waveform", 3 }, { "osc2_octave", 1 }, { "osc2_level", 35 },
        { "osc2_attack", 3 }, { "osc2_decay", 350 }, { "osc2_sustain", 15 }, { "osc2_release", 200 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 12 }, { "filter_env_amount", 25 },
        { "filter_decay", 400 },
        { "filter_velocity", 45 }, { "chorus_mode", 1 }, { "master_analog_drift", 10 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_level", 3 }, { "noise_attack", 1 }, { "noise_decay", 80 }, { "noise_sustain", 0 }, { "noise_release", 50 }
    }));

    presets.push_back (makePreset ("Music Box", "Keys", {
        { "osc1_waveform", 3 }, { "osc1_octave", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 600 }, { "osc1_sustain", 0 }, { "osc1_release", 500 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 20 },
        { "osc2_attack", 1 }, { "osc2_decay", 400 }, { "osc2_sustain", 0 }, { "osc2_release", 400 },
        { "filter_cutoff", 7000 }, { "filter_resonance", 12 },
        { "reverb_size", 65 }, { "reverb_mix", 30 },
        { "noise_level", 2 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Tine Piano", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 25 }, { "osc1_release", 200 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 25 },
        { "osc2_attack", 1 }, { "osc2_decay", 200 }, { "osc2_sustain", 5 }, { "osc2_release", 150 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 15 }, { "filter_env_amount", 30 },
        { "filter_decay", 250 }, { "filter_sustain", 10 },
        { "filter_velocity", 60 }, { "chorus_mode", 1 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_level", 3 }, { "noise_attack", 1 }, { "noise_decay", 60 }, { "noise_sustain", 0 }, { "noise_release", 40 }
    }));

    presets.push_back (makePreset ("Clavinet Funk", "Keys", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 20 },
        { "osc1_attack", 1 }, { "osc1_decay", 150 }, { "osc1_sustain", 40 }, { "osc1_release", 60 },
        { "osc2_level", 0 },
        { "filter_cutoff", 7000 }, { "filter_resonance", 30 }, { "filter_env_amount", 40 },
        { "filter_decay", 120 }, { "filter_sustain", 15 },
        { "filter_velocity", 70 },
        { "reverb_size", 35 }, { "reverb_mix", 10 },
        { "noise_level", 4 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 25 }
    }));

    presets.push_back (makePreset ("Dulcimer", "Keys", {
        { "osc1_waveform", 3 },
        { "osc1_attack", 1 }, { "osc1_decay", 500 }, { "osc1_sustain", 10 }, { "osc1_release", 300 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 30 },
        { "osc2_attack", 1 }, { "osc2_decay", 400 }, { "osc2_sustain", 5 }, { "osc2_release", 250 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 10 }, { "filter_env_amount", 20 },
        { "filter_decay", 300 },
        { "reverb_size", 60 }, { "reverb_mix", 22 }
    }));

    presets.push_back (makePreset ("Accordion", "Keys", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 40 },
        { "osc1_attack", 20 }, { "osc1_sustain", 90 }, { "osc1_release", 50 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 60 }, { "osc2_octave", 1 },
        { "osc2_level", 40 },
        { "osc2_attack", 25 }, { "osc2_sustain", 90 }, { "osc2_release", 50 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 15 },
        { "lfo1_depth", 4 }, { "lfo1_rate", 6.0f }, { "lfo1_dest_volume", 1 },
        { "master_analog_drift", 10 },
        { "reverb_size", 45 }, { "reverb_mix", 15 },
        { "noise_level", 3 }, { "noise_attack", 20 }, { "noise_sustain", 60 }, { "noise_release", 50 }
    }));

    presets.push_back (makePreset ("Chime Keys", "Keys", {
        { "osc1_waveform", 0 }, { "osc1_octave", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 1000 }, { "osc1_sustain", 0 }, { "osc1_release", 800 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_semitone", 7 },
        { "osc2_level", 25 },
        { "osc2_attack", 1 }, { "osc2_decay", 700 }, { "osc2_sustain", 0 }, { "osc2_release", 600 },
        { "filter_cutoff", 10000 }, { "filter_resonance", 8 },
        { "reverb_size", 70 }, { "reverb_mix", 30 },
        { "noise_level", 2 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Synth Organ", "Keys", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 5 }, { "osc1_sustain", 100 }, { "osc1_release", 20 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 50 },
        { "osc2_attack", 5 }, { "osc2_sustain", 100 }, { "osc2_release", 20 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 5 },
        { "chorus_mode", 2 },
        { "reverb_size", 50 }, { "reverb_mix", 15 }
    }));

    presets.push_back (makePreset ("Glockenspiel", "Keys", {
        { "osc1_waveform", 0 }, { "osc1_octave", 2 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 0 }, { "osc1_release", 300 },
        { "osc2_waveform", 0 }, { "osc2_octave", 3 }, { "osc2_level", 20 },
        { "osc2_attack", 1 }, { "osc2_decay", 250 }, { "osc2_sustain", 0 }, { "osc2_release", 200 },
        { "filter_cutoff", 12000 }, { "filter_resonance", 5 },
        { "reverb_size", 65 }, { "reverb_mix", 30 }
    }));

    presets.push_back (makePreset ("Broken Piano", "Keys", {
        { "osc1_waveform", 3 },
        { "osc1_attack", 2 }, { "osc1_decay", 500 }, { "osc1_sustain", 20 }, { "osc1_release", 200 },
        { "osc2_waveform", 3 }, { "osc2_fine_tune", 15 }, { "osc2_level", 40 },
        { "osc2_attack", 2 }, { "osc2_decay", 450 }, { "osc2_sustain", 15 }, { "osc2_release", 180 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 20 }, { "filter_env_amount", 30 },
        { "filter_decay", 300 },
        { "master_analog_drift", 30 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_level", 5 }, { "noise_attack", 1 }, { "noise_decay", 80 }, { "noise_sustain", 5 }, { "noise_release", 50 }
    }));

    // --- FX (173-185) ---

    presets.push_back (makePreset ("Phaser Sweep", "FX", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 15 },
        { "osc1_attack", 500 }, { "osc1_sustain", 70 }, { "osc1_release", 1000 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 10 }, { "osc2_level", 45 },
        { "osc2_attack", 600 }, { "osc2_sustain", 70 }, { "osc2_release", 1000 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 40 },
        { "lfo1_depth", 50 }, { "lfo1_rate", 0.3f }, { "lfo1_dest_cutoff", 1 },
        { "chorus_mode", 2 }, { "master_analog_drift", 15 },
        { "reverb_size", 55 }, { "reverb_mix", 22 },
        { "master_unison", 2 }
    }));

    presets.push_back (makePreset ("Dial-up Modem", "FX", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 50 },
        { "osc1_attack", 1 }, { "osc1_sustain", 80 }, { "osc1_release", 50 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 10 }, { "osc2_semitone", 5 },
        { "osc2_level", 45 },
        { "osc2_attack", 1 }, { "osc2_sustain", 80 }, { "osc2_release", 50 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 60 },
        { "lfo1_depth", 60 }, { "lfo1_rate", 10.0f }, { "lfo1_dest_pitch", 1 },
        { "lfo2_depth", 40 }, { "lfo2_rate", 8.0f }, { "lfo2_dest_cutoff", 1 },
        { "reverb_size", 35 }, { "reverb_mix", 12 },
        { "noise_type", 4 }, { "noise_level", 20 }, { "noise_sh_rate", 12000 }, { "noise_sustain", 60 }, { "noise_release", 50 }
    }));

    presets.push_back (makePreset ("Cosmic Rain", "FX", {
        { "osc1_waveform", 3 }, { "osc1_unison_voices", 5 }, { "osc1_unison_detune", 40 },
        { "osc1_attack", 1000 }, { "osc1_decay", 2000 }, { "osc1_sustain", 30 },
        { "osc1_release", 3000 },
        { "osc2_waveform", 0 }, { "osc2_octave", 2 }, { "osc2_level", 20 },
        { "osc2_attack", 1200 }, { "osc2_decay", 1500 }, { "osc2_sustain", 10 },
        { "osc2_release", 2500 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 25 }, { "filter_env_amount", 30 },
        { "filter_decay", 1500 },
        { "chorus_mode", 3 }, { "reverb_size", 80 }, { "reverb_mix", 40 },
        { "master_analog_drift", 35 },
        { "noise_type", 3 }, { "noise_level", 8 }, { "noise_attack", 1000 }, { "noise_decay", 2000 }, { "noise_sustain", 20 }, { "noise_release", 3000 },
        { "master_unison", 3 }
    }));

    presets.push_back (makePreset ("Error Glitch", "FX", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 5 },
        { "osc1_attack", 1 }, { "osc1_decay", 60 }, { "osc1_sustain", 0 }, { "osc1_release", 30 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 95 }, { "osc2_semitone", 11 },
        { "osc2_level", 45 },
        { "osc2_attack", 1 }, { "osc2_decay", 50 }, { "osc2_sustain", 0 }, { "osc2_release", 25 },
        { "filter_cutoff", 10000 }, { "filter_resonance", 30 },
        { "lfo1_depth", 40 }, { "lfo1_rate", 15.0f }, { "lfo1_dest_pw", 1 },
        { "reverb_size", 30 }, { "reverb_mix", 10 },
        { "noise_type", 4 }, { "noise_level", 15 }, { "noise_sh_rate", 15000 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 20 }
    }));

    presets.push_back (makePreset ("Haunted House", "FX", {
        { "osc1_waveform", 0 }, { "osc1_octave", -1 },
        { "osc1_attack", 2000 }, { "osc1_sustain", 50 }, { "osc1_release", 4000 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 40 },
        { "osc2_fine_tune", -20 },
        { "osc2_attack", 2500 }, { "osc2_sustain", 50 }, { "osc2_release", 4000 },
        { "filter_cutoff", 1000 }, { "filter_resonance", 40 },
        { "lfo1_depth", 15 }, { "lfo1_rate", 0.05f }, { "lfo1_dest_pitch", 1 },
        { "reverb_size", 90 }, { "reverb_mix", 45 }, { "master_analog_drift", 50 },
        { "noise_type", 1 }, { "noise_level", 10 }, { "noise_attack", 2000 }, { "noise_sustain", 40 }, { "noise_release", 4000 }
    }));

    presets.push_back (makePreset ("Rising Tension", "FX", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 },
        { "osc1_attack", 5000 }, { "osc1_sustain", 80 }, { "osc1_release", 2000 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 8 }, { "osc2_level", 45 },
        { "osc2_attack", 5500 }, { "osc2_sustain", 80 }, { "osc2_release", 2000 },
        { "filter_cutoff", 500 }, { "filter_resonance", 30 }, { "filter_env_amount", 80 },
        { "filter_attack", 5000 }, { "filter_decay", 2000 }, { "filter_sustain", 60 },
        { "master_analog_drift", 25 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 5000 }, { "noise_sustain", 60 }, { "noise_release", 2000 }
    }));

    presets.push_back (makePreset ("Warped Vinyl", "FX", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 100 }, { "osc1_sustain", 70 }, { "osc1_release", 500 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 15 }, { "osc2_level", 45 },
        { "osc2_attack", 120 }, { "osc2_sustain", 70 }, { "osc2_release", 500 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 20 },
        { "lfo1_depth", 10 }, { "lfo1_rate", 0.3f }, { "lfo1_dest_pitch", 1 },
        { "lfo2_depth", 15 }, { "lfo2_rate", 4.0f }, { "lfo2_dest_volume", 1 },
        { "chorus_mode", 3 }, { "master_analog_drift", 60 },
        { "reverb_size", 50 }, { "reverb_mix", 20 },
        { "noise_type", 1 }, { "noise_level", 5 }, { "noise_sustain", 50 }, { "noise_release", 500 }
    }));

    presets.push_back (makePreset ("Sonar Ping", "FX", {
        { "osc1_waveform", 0 }, { "osc1_octave", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 200 }, { "osc1_sustain", 0 }, { "osc1_release", 600 },
        { "osc2_level", 0 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 50 }, { "filter_env_amount", 40 },
        { "filter_decay", 300 }, { "filter_sustain", 0 },
        { "reverb_size", 80 }, { "reverb_mix", 50 },
        { "noise_level", 4 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 100 }
    }));

    presets.push_back (makePreset ("Radio Static", "FX", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 100 },
        { "osc1_level", 40 },
        { "osc1_attack", 200 }, { "osc1_sustain", 60 }, { "osc1_release", 500 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 5 }, { "osc2_unison_voices", 7 },
        { "osc2_unison_detune", 90 }, { "osc2_level", 35 },
        { "osc2_attack", 250 }, { "osc2_sustain", 60 }, { "osc2_release", 500 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 25 },
        { "lfo1_depth", 40 }, { "lfo1_rate", 0.15f }, { "lfo1_dest_volume", 1 },
        { "master_analog_drift", 80 },
        { "reverb_size", 40 }, { "reverb_mix", 15 },
        { "noise_type", 4 }, { "noise_level", 50 }, { "noise_sh_rate", 10000 }, { "noise_sustain", 60 }, { "noise_release", 500 }
    }));

    presets.push_back (makePreset ("Ticking Clock", "FX", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 10 }, { "osc1_octave", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 30 }, { "osc1_sustain", 0 }, { "osc1_release", 20 },
        { "osc2_level", 0 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 20 },
        { "reverb_size", 50 }, { "reverb_mix", 20 },
        { "noise_level", 3 }, { "noise_attack", 1 }, { "noise_decay", 20 }, { "noise_sustain", 0 }, { "noise_release", 15 }
    }));

    presets.push_back (makePreset ("Swarm", "FX", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 7 }, { "osc1_unison_detune", 80 },
        { "osc1_level", 35 },
        { "osc1_attack", 1000 }, { "osc1_sustain", 70 }, { "osc1_release", 2000 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 20 }, { "osc2_unison_voices", 5 },
        { "osc2_unison_detune", 60 }, { "osc2_level", 30 },
        { "osc2_attack", 1200 }, { "osc2_sustain", 70 }, { "osc2_release", 2000 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 30 },
        { "lfo1_depth", 25 }, { "lfo1_rate", 0.2f }, { "lfo1_dest_cutoff", 1 },
        { "master_analog_drift", 70 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "noise_type", 1 }, { "noise_level", 8 }, { "noise_attack", 1000 }, { "noise_sustain", 50 }, { "noise_release", 2000 },
        { "master_unison", 4 }
    }));

    presets.push_back (makePreset ("Voltage Spike", "FX", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 50 }, { "osc1_sustain", 0 }, { "osc1_release", 30 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 10 }, { "osc2_octave", 1 },
        { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_decay", 40 }, { "osc2_sustain", 0 }, { "osc2_release", 25 },
        { "filter_cutoff", 12000 }, { "filter_resonance", 50 }, { "filter_env_amount", -80 },
        { "filter_decay", 60 }, { "filter_sustain", 0 },
        { "reverb_size", 40 }, { "reverb_mix", 15 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 30 }, { "noise_sustain", 0 }, { "noise_release", 20 }
    }));

    presets.push_back (makePreset ("Deep Rumble", "FX", {
        { "osc1_waveform", 1 }, { "osc1_octave", -2 }, { "osc1_unison_voices", 5 },
        { "osc1_unison_detune", 50 },
        { "osc1_attack", 1000 }, { "osc1_sustain", 60 }, { "osc1_release", 3000 },
        { "osc2_waveform", 0 }, { "osc2_octave", -2 }, { "osc2_level", 50 },
        { "osc2_attack", 1200 }, { "osc2_sustain", 60 }, { "osc2_release", 3000 },
        { "filter_cutoff", 800 }, { "filter_resonance", 20 },
        { "lfo1_depth", 20 }, { "lfo1_rate", 0.1f }, { "lfo1_dest_cutoff", 1 },
        { "master_analog_drift", 45 },
        { "reverb_size", 75 }, { "reverb_mix", 30 },
        { "noise_type", 2 }, { "noise_level", 15 }, { "noise_attack", 1000 }, { "noise_sustain", 50 }, { "noise_release", 3000 }
    }));

    // --- PLUCKS (186-200) ---

    presets.push_back (makePreset ("Steel String", "Plucks", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 300 }, { "osc1_sustain", 10 }, { "osc1_release", 200 },
        { "osc2_waveform", 3 }, { "osc2_level", 40 }, { "osc2_fine_tune", 3 },
        { "osc2_attack", 1 }, { "osc2_decay", 250 }, { "osc2_sustain", 5 }, { "osc2_release", 180 },
        { "filter_cutoff", 6000 }, { "filter_resonance", 20 }, { "filter_env_amount", 40 },
        { "filter_decay", 200 }, { "filter_sustain", 5 },
        { "filter_velocity", 40 }, { "master_analog_drift", 5 },
        { "reverb_size", 50 }, { "reverb_mix", 18 },
        { "noise_level", 12 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Nylon Pluck", "Plucks", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 5 }, { "osc1_release", 250 },
        { "osc2_waveform", 3 }, { "osc2_level", 30 },
        { "osc2_attack", 1 }, { "osc2_decay", 300 }, { "osc2_sustain", 0 }, { "osc2_release", 200 },
        { "filter_cutoff", 3500 }, { "filter_resonance", 10 }, { "filter_env_amount", 20 },
        { "filter_decay", 300 },
        { "reverb_size", 55 }, { "reverb_mix", 22 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 60 }, { "noise_sustain", 0 }, { "noise_release", 40 }
    }));

    presets.push_back (makePreset ("Harp", "Plucks", {
        { "osc1_waveform", 3 },
        { "osc1_attack", 1 }, { "osc1_decay", 600 }, { "osc1_sustain", 10 }, { "osc1_release", 500 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 25 },
        { "osc2_attack", 1 }, { "osc2_decay", 400 }, { "osc2_sustain", 0 }, { "osc2_release", 400 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 12 }, { "filter_env_amount", 25 },
        { "filter_decay", 400 }, { "filter_sustain", 5 },
        { "reverb_size", 55 }, { "reverb_mix", 20 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 40 }
    }));

    presets.push_back (makePreset ("Banjo Twang", "Plucks", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 20 },
        { "osc1_attack", 1 }, { "osc1_decay", 150 }, { "osc1_sustain", 5 }, { "osc1_release", 80 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 80 }, { "osc2_level", 35 },
        { "osc2_attack", 1 }, { "osc2_decay", 120 }, { "osc2_sustain", 0 }, { "osc2_release", 60 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 25 }, { "filter_env_amount", 50 },
        { "filter_decay", 100 }, { "filter_sustain", 5 },
        { "filter_velocity", 50 },
        { "reverb_size", 40 }, { "reverb_mix", 15 },
        { "noise_level", 12 }, { "noise_attack", 1 }, { "noise_decay", 30 }, { "noise_sustain", 0 }, { "noise_release", 20 }
    }));

    presets.push_back (makePreset ("Koto", "Plucks", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 500 }, { "osc1_sustain", 5 }, { "osc1_release", 300 },
        { "osc2_waveform", 0 }, { "osc2_octave", 1 }, { "osc2_level", 25 },
        { "osc2_attack", 1 }, { "osc2_decay", 350 }, { "osc2_sustain", 0 }, { "osc2_release", 250 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 30 }, { "filter_env_amount", 40 },
        { "filter_decay", 400 }, { "filter_sustain", 5 },
        { "reverb_size", 50 }, { "reverb_mix", 15 },
        { "noise_level", 10 }, { "noise_attack", 1 }, { "noise_decay", 45 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Resonant Pluck", "Plucks", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 200 }, { "osc1_sustain", 0 }, { "osc1_release", 150 },
        { "osc2_level", 0 },
        { "filter_cutoff", 2000 }, { "filter_resonance", 70 }, { "filter_env_amount", 60 },
        { "filter_decay", 150 }, { "filter_sustain", 0 },
        { "reverb_size", 55 }, { "reverb_mix", 22 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 35 }, { "noise_sustain", 0 }, { "noise_release", 25 }
    }));

    presets.push_back (makePreset ("Bell Pluck", "Plucks", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 1 }, { "osc1_decay", 300 }, { "osc1_sustain", 0 }, { "osc1_release", 250 },
        { "osc2_waveform", 0 }, { "osc2_semitone", 7 }, { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_decay", 250 }, { "osc2_sustain", 0 }, { "osc2_release", 200 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 8 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "noise_level", 6 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Sitar-ish", "Plucks", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 400 }, { "osc1_sustain", 10 }, { "osc1_release", 300 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", 15 }, { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_decay", 350 }, { "osc2_sustain", 5 }, { "osc2_release", 250 },
        { "filter_cutoff", 3000 }, { "filter_resonance", 45 }, { "filter_env_amount", 40 },
        { "filter_decay", 300 }, { "filter_sustain", 10 },
        { "master_analog_drift", 15 },
        { "reverb_size", 50 }, { "reverb_mix", 18 },
        { "noise_level", 10 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 35 }
    }));

    presets.push_back (makePreset ("Chorus Pluck", "Plucks", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 250 }, { "osc1_sustain", 5 }, { "osc1_release", 200 },
        { "osc2_waveform", 3 }, { "osc2_level", 35 },
        { "osc2_attack", 1 }, { "osc2_decay", 200 }, { "osc2_sustain", 0 }, { "osc2_release", 180 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 15 }, { "filter_env_amount", 35 },
        { "filter_decay", 200 }, { "filter_sustain", 5 },
        { "chorus_mode", 2 },
        { "reverb_size", 55 }, { "reverb_mix", 22 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 40 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Metallic Pick", "Plucks", {
        { "osc1_waveform", 2 }, { "osc1_pulse_width", 10 },
        { "osc1_attack", 1 }, { "osc1_decay", 150 }, { "osc1_sustain", 0 }, { "osc1_release", 100 },
        { "osc2_waveform", 2 }, { "osc2_pulse_width", 90 }, { "osc2_semitone", 5 },
        { "osc2_level", 40 },
        { "osc2_attack", 1 }, { "osc2_decay", 120 }, { "osc2_sustain", 0 }, { "osc2_release", 80 },
        { "filter_cutoff", 10000 }, { "filter_resonance", 30 },
        { "reverb_size", 45 }, { "reverb_mix", 15 },
        { "noise_level", 10 }, { "noise_attack", 1 }, { "noise_decay", 25 }, { "noise_sustain", 0 }, { "noise_release", 15 }
    }));

    presets.push_back (makePreset ("Soft Pluck", "Plucks", {
        { "osc1_waveform", 0 },
        { "osc1_attack", 1 }, { "osc1_decay", 350 }, { "osc1_sustain", 0 }, { "osc1_release", 250 },
        { "osc2_waveform", 0 }, { "osc2_level", 40 }, { "osc2_fine_tune", -5 },
        { "osc2_attack", 1 }, { "osc2_decay", 300 }, { "osc2_sustain", 0 }, { "osc2_release", 200 },
        { "filter_cutoff", 2500 }, { "filter_resonance", 10 }, { "filter_env_amount", 15 },
        { "filter_decay", 250 },
        { "chorus_mode", 1 },
        { "reverb_size", 60 }, { "reverb_mix", 25 },
        { "noise_level", 6 }, { "noise_attack", 1 }, { "noise_decay", 50 }, { "noise_sustain", 0 }, { "noise_release", 35 }
    }));

    presets.push_back (makePreset ("Bright Pick", "Plucks", {
        { "osc1_waveform", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 180 }, { "osc1_sustain", 0 }, { "osc1_release", 120 },
        { "osc2_waveform", 1 }, { "osc2_octave", 1 }, { "osc2_level", 25 },
        { "osc2_attack", 1 }, { "osc2_decay", 120 }, { "osc2_sustain", 0 }, { "osc2_release", 80 },
        { "filter_cutoff", 8000 }, { "filter_resonance", 15 }, { "filter_env_amount", 50 },
        { "filter_decay", 150 }, { "filter_sustain", 0 },
        { "filter_velocity", 50 },
        { "reverb_size", 45 }, { "reverb_mix", 18 },
        { "noise_level", 10 }, { "noise_attack", 1 }, { "noise_decay", 35 }, { "noise_sustain", 0 }, { "noise_release", 20 }
    }));

    presets.push_back (makePreset ("Mbira", "Plucks", {
        { "osc1_waveform", 0 }, { "osc1_octave", 1 },
        { "osc1_attack", 1 }, { "osc1_decay", 500 }, { "osc1_sustain", 10 }, { "osc1_release", 400 },
        { "osc2_waveform", 3 }, { "osc2_octave", 2 }, { "osc2_level", 20 },
        { "osc2_attack", 1 }, { "osc2_decay", 300 }, { "osc2_sustain", 0 }, { "osc2_release", 300 },
        { "filter_cutoff", 5000 }, { "filter_resonance", 12 },
        { "reverb_size", 50 }, { "reverb_mix", 15 }, { "master_analog_drift", 10 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 45 }, { "noise_sustain", 0 }, { "noise_release", 30 }
    }));

    presets.push_back (makePreset ("Staccato Strings", "Plucks", {
        { "osc1_waveform", 1 }, { "osc1_unison_voices", 3 }, { "osc1_unison_detune", 8 },
        { "osc1_attack", 2 }, { "osc1_decay", 200 }, { "osc1_sustain", 10 }, { "osc1_release", 100 },
        { "osc2_waveform", 1 }, { "osc2_fine_tune", -4 }, { "osc2_level", 45 },
        { "osc2_attack", 2 }, { "osc2_decay", 180 }, { "osc2_sustain", 5 }, { "osc2_release", 80 },
        { "filter_cutoff", 4000 }, { "filter_resonance", 15 }, { "filter_env_amount", 30 },
        { "filter_decay", 150 }, { "filter_sustain", 5 },
        { "chorus_mode", 1 }, { "master_analog_drift", 8 },
        { "reverb_size", 50 }, { "reverb_mix", 18 },
        { "noise_level", 8 }, { "noise_attack", 1 }, { "noise_decay", 30 }, { "noise_sustain", 0 }, { "noise_release", 20 }
    }));
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& a)
    : apvts (a)
{
    buildFactoryPresets();
    factoryPresetCount = static_cast<int> (presets.size());
    loadUserPresets();
}

int PresetManager::getNumPresets() const
{
    return static_cast<int> (presets.size());
}

juce::String PresetManager::getPresetName (int index) const
{
    if (index < 0 || index >= getNumPresets())
        return {};

    return presets[static_cast<size_t> (index)].name;
}

juce::String PresetManager::getPresetCategory (int index) const
{
    if (index < 0 || index >= getNumPresets())
        return {};

    return presets[static_cast<size_t> (index)].category;
}

juce::String PresetManager::getPresetKey (int index) const
{
    if (index < 0 || index >= getNumPresets())
        return {};

    if (isUserPreset (index))
        return "user:" + presets[static_cast<size_t> (index)].name;

    return "factory:" + juce::String (index) + ":" + presets[static_cast<size_t> (index)].name;
}

const Preset* PresetManager::getPreset (int index) const
{
    if (index < 0 || index >= getNumPresets())
        return nullptr;

    return &presets[static_cast<size_t> (index)];
}

void PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= getNumPresets())
        return;

    currentPreset = index;
    applyPreset (presets[static_cast<size_t> (index)]);
}

void PresetManager::applyPreset (const Preset& preset)
{
    for (const auto& [paramId, value] : preset.parameters)
        if (auto* p = apvts.getParameter (paramId))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : presets)
        names.add (preset.name);
    return names;
}

juce::File PresetManager::getUserPresetDirectory() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("TillySynth")
                   .getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

juce::File PresetManager::getUserPresetFile (const juce::String& name) const
{
    return getUserPresetDirectory().getChildFile (sanitisePresetFileName (name) + ".xml");
}

juce::String PresetManager::sanitisePresetFileName (const juce::String& name)
{
    auto trimmed = name.trim();
    auto safe = trimmed.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_ ()");
    return safe.isNotEmpty() ? safe : "Preset";
}

void PresetManager::loadUserPresets()
{
    // Remove any previously loaded user presets
    if (static_cast<int> (presets.size()) > factoryPresetCount)
        presets.erase (presets.begin() + factoryPresetCount, presets.end());

    auto dir = getUserPresetDirectory();
    auto files = dir.findChildFiles (juce::File::findFiles, false, "*.xml");
    files.sort();

    for (const auto& file : files)
    {
        auto xml = juce::XmlDocument::parse (file);
        if (xml == nullptr || ! xml->hasTagName ("TillySynthPreset"))
            continue;

        Preset preset;
        preset.name = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
        preset.category = xml->getStringAttribute ("category", "User");

        bool hasAnyModMatrixParam = false;

        for (auto* paramEl : xml->getChildIterator())
        {
            if (paramEl->hasTagName ("Param"))
            {
                auto id = paramEl->getStringAttribute ("id");
                auto val = static_cast<float> (paramEl->getDoubleAttribute ("value"));
                preset.parameters.push_back ({ id, val });

                if (id.startsWith ("modmatrix_"))
                    hasAnyModMatrixParam = true;
            }
        }

        auto ensureParam = [&preset] (const juce::String& id, float defaultValue)
        {
            for (const auto& [paramId, value] : preset.parameters)
            {
                juce::ignoreUnused (value);
                if (paramId == id)
                    return;
            }

            preset.parameters.push_back ({ id, defaultValue });
        };

        ensureParam ("filter_target", 4.0f);

        ensureParam ("modenv1_attack", 5.0f);
        ensureParam ("modenv1_decay", 200.0f);
        ensureParam ("modenv1_sustain", 0.0f);
        ensureParam ("modenv1_release", 300.0f);
        ensureParam ("modenv1_amount", 0.0f);
        ensureParam ("modenv1_dest_cutoff", 0.0f);
        ensureParam ("modenv1_dest_resonance", 0.0f);
        ensureParam ("modenv1_dest_pitch", 0.0f);
        ensureParam ("modenv1_dest_volume", 0.0f);

        ensureParam ("modenv2_attack", 5.0f);
        ensureParam ("modenv2_decay", 200.0f);
        ensureParam ("modenv2_sustain", 0.0f);
        ensureParam ("modenv2_release", 300.0f);
        ensureParam ("modenv2_amount", 0.0f);
        ensureParam ("modenv2_dest_cutoff", 0.0f);
        ensureParam ("modenv2_dest_resonance", 0.0f);
        ensureParam ("modenv2_dest_pitch", 0.0f);
        ensureParam ("modenv2_dest_volume", 0.0f);

        ensureParam ("filter_model", 0.0f);
        ensureParam ("lfo3_waveform", 0.0f);
        ensureParam ("lfo3_rate", 1.0f);
        ensureParam ("lfo3_depth", 0.0f);
        ensureParam ("lfo3_dest_cutoff", 0.0f);
        ensureParam ("lfo3_dest_pitch", 0.0f);
        ensureParam ("lfo3_dest_volume", 0.0f);
        ensureParam ("lfo3_dest_pw", 0.0f);

        for (int i = 1; i <= 8; ++i)
        {
            auto slot = juce::String (i);
            ensureParam ("modmatrix_" + slot + "_source", 0.0f);
            ensureParam ("modmatrix_" + slot + "_dest", 0.0f);
            ensureParam ("modmatrix_" + slot + "_amount", 0.0f);
        }

        ensureParam ("modrange_cutoff", 75.0f);
        ensureParam ("modrange_resonance", 75.0f);
        ensureParam ("modrange_pitch", 2.0f);
        ensureParam ("modrange_volume", 50.0f);
        ensureParam ("modrange_pw", 45.0f);
        ensureParam ("modrange_osc1_level", 100.0f);
        ensureParam ("modrange_osc2_level", 100.0f);
        ensureParam ("modrange_noise_level", 100.0f);
        ensureParam ("modrange_lfo1_rate", 10.0f);
        ensureParam ("modrange_lfo2_rate", 10.0f);
        ensureParam ("modrange_lfo3_rate", 10.0f);

        ensureParam ("sidechain_amount", 0.0f);
        ensureParam ("output_mode", 0.0f);
        ensureParam ("output_drive", 25.0f);
        ensureParam ("output_mix", 100.0f);

        // Legacy user presets created before the matrix existed get a deterministic,
        // per-preset matrix profile (name/category based), not a shared global one.
        if (! hasAnyModMatrixParam)
            applyPresetSpecificModMatrixProfile (preset.name, preset.category, preset.parameters);

        presets.push_back (std::move (preset));
    }
}

void PresetManager::saveUserPreset (const juce::String& name)
{
    saveUserPreset (captureCurrentPreset (name));
}

Preset PresetManager::captureCurrentPreset (const juce::String& name, const juce::String& category) const
{
    Preset preset;
    preset.name = name;
    preset.category = category;

    for (auto* param : apvts.processor.getParameters())
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
            preset.parameters.push_back ({ p->getParameterID(), p->convertFrom0to1 (p->getValue()) });

    return preset;
}

void PresetManager::writePresetToFile (const Preset& preset, const juce::File& file) const
{
    juce::XmlElement xml ("TillySynthPreset");
    xml.setAttribute ("name", preset.name);
    xml.setAttribute ("category", preset.category);

    for (const auto& [paramId, value] : preset.parameters)
    {
        auto* paramEl = xml.createNewChildElement ("Param");
        paramEl->setAttribute ("id", paramId);
        paramEl->setAttribute ("value", static_cast<double> (value));
    }

    xml.writeTo (file);
}

void PresetManager::saveUserPreset (const Preset& preset)
{
    auto userPreset = preset;
    if (userPreset.category.isEmpty())
        userPreset.category = "User";

    writePresetToFile (userPreset, getUserPresetFile (userPreset.name));
    refreshUserPresets();
}

void PresetManager::deleteUserPreset (int index)
{
    if (index < factoryPresetCount || index >= getNumPresets())
        return;

    auto name = presets[static_cast<size_t> (index)].name;
    auto file = getUserPresetFile (name);
    file.deleteFile();
    refreshUserPresets();
}

bool PresetManager::isUserPreset (int index) const
{
    return index >= factoryPresetCount && index < getNumPresets();
}

void PresetManager::refreshUserPresets()
{
    loadUserPresets();
}

} // namespace tillysynth
