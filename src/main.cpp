#include "DistrhoPlugin.hpp"
#include "Circuits/FuzzClipper.h"
#include "Circuits/LowPassFilter.h"

#include <algorithm>
#include <cmath>

USE_NAMESPACE_DISTRHO;

START_NAMESPACE_DISTRHO;

namespace
{

    constexpr float kDefaultDoom = 0.5f;
    constexpr float kDefaultTone = 0.75f;

    struct DoomCurve
    {
        float low;
        float high;
        float bend;
    };

    struct DoomSettings
    {
        float thresholdDb;
        float ratio;
        float inputGainDb;
        float outputGainDb;
        float hardClipMix;
    };

    // Tune these to reshape the one-knob sweep without changing the UI or host parameter list.
    constexpr DoomCurve kThresholdCurve = {-6.0f, -30.0f, 1.10f};
    constexpr DoomCurve kRatioCurve = {1.5f, 35.0f, 1.45f};
    constexpr DoomCurve kInputGainCurve = {-3.0f, 30.0f, 1.20f};
    constexpr DoomCurve kOutputGainCurve = {-2.0f, 11.0f, 1.00f};
    constexpr DoomCurve kHardClipCurve = {0.0f, 1.0f, 3.0f};
    constexpr DoomCurve kClipperCutoffCurve = {180.0f, 6500.0f, 1.35f};
    constexpr DoomCurve kToneCutoffCurve = {450.0f, 14000.0f, 1.80f};

    static float clamp01(const float value) noexcept
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    static float mapCurve(const float doom, const DoomCurve &curve) noexcept
    {
        const float shaped = std::pow(clamp01(doom), std::max(0.001f, curve.bend));
        return curve.low + (curve.high - curve.low) * shaped;
    }

    static DoomSettings makeDoomSettings(const float doom) noexcept
    {
        return DoomSettings{
            mapCurve(doom, kThresholdCurve),
            mapCurve(doom, kRatioCurve),
            mapCurve(doom, kInputGainCurve),
            mapCurve(doom, kOutputGainCurve),
            mapCurve(doom, kHardClipCurve),
        };
    }

}

class DistPlugin : public Plugin
{
public:
    DistPlugin()
        : Plugin(kParameterCount, 0, 0),
          fDoom(kDefaultDoom),
          fTone(kDefaultTone)
    {
    }

protected:
    const char *getLabel() const override
    {
        return "dih-stortion";
    }

    const char *getMaker() const override
    {
        return "Evil Plugins";
    }

    const char *getLicense() const override
    {
        return "MIT";
    }

    uint32_t getVersion() const override
    {
        return d_version(1, 0, 0);
    }

    /**
       Get the plugin unique Id.
       This value is used by LADSPA, DSSI, VST2 and VST3 plugin formats.
     */
    int64_t getUniqueId() const override
    {
        return d_cconst('D', 'i', 'h', 'S');
    }

    void initParameter(uint32_t index, Parameter &parameter) override
    {
        parameter.hints = kParameterIsAutomatable;

        switch (index)
        {
        case kParameterDoom:
            parameter.name = "Doom";
            parameter.symbol = "doom";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = kDefaultDoom;
            break;
        case kParameterTone:
            parameter.name = "Tone";
            parameter.symbol = "tone";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = kDefaultTone;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        if (index >= kParameterCount)
            return 0.0f;

        if (index == kParameterDoom)
            return fDoom;

        if (index == kParameterTone)
            return fTone;

        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index == kParameterDoom)
            fDoom = clamp01(value);
        else if (index == kParameterTone)
            fTone = clamp01(value);
    }

    void activate() override
    {
        prepareClippers(getSampleRate());
        resetClippers();
    }

    void sampleRateChanged(double newSampleRate) override
    {
        prepareClippers(newSampleRate);
    }

    void run(const float **inputs, float **outputs, uint32_t frames) override
    {
        const float *const inLeft = inputs[0];
        const float *const inRight = inputs[1];
        float *const outLeft = outputs[0];
        float *const outRight = outputs[1];

        const DoomSettings settings = makeDoomSettings(fDoom);
        const float inputGain = dbToGain(settings.inputGainDb);
        const float outputGain = dbToGain(settings.outputGainDb);
        const float clipperCutoff = mapCurve(fDoom, kClipperCutoffCurve);
        const float toneCutoff = mapCurve(fTone, kToneCutoffCurve);

        fLeftClipper.setCircuitParams(clipperCutoff);
        fRightClipper.setCircuitParams(clipperCutoff);
        fLeftToneFilter.setCircuitParams(toneCutoff);
        fRightToneFilter.setCircuitParams(toneCutoff);

        for (uint32_t i = 0; i < frames; ++i)
        {
            const float left = fLeftClipper.processSample(inLeft[i] * inputGain);
            const float right = fRightClipper.processSample(inRight[i] * inputGain);

            outLeft[i] = fLeftToneFilter.processSample(left) * outputGain;
            outRight[i] = fRightToneFilter.processSample(right) * outputGain;
        }
    }

private:
    void prepareClippers(double sampleRate)
    {
        fLeftClipper.prepare(sampleRate);
        fRightClipper.prepare(sampleRate);
        fLeftToneFilter.prepare(sampleRate);
        fRightToneFilter.prepare(sampleRate);
    }

    void resetClippers()
    {
        fLeftClipper.reset();
        fRightClipper.reset();
        fLeftToneFilter.reset();
        fRightToneFilter.reset();
    }

    static float dbToGain(const float db) noexcept
    {
        return std::pow(10.0f, db / 20.0f);
    }

    float fDoom;
    float fTone;
    FuzzClipper fLeftClipper;
    FuzzClipper fRightClipper;
    LowPassFilter fLeftToneFilter;
    LowPassFilter fRightToneFilter;
};

Plugin *createPlugin()
{
    return new DistPlugin();
}

END_NAMESPACE_DISTRHO;
