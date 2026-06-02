#include "DistrhoPlugin.hpp"

#include <algorithm>
#include <cmath>

USE_NAMESPACE_DISTRHO;

START_NAMESPACE_DISTRHO;

namespace
{

    constexpr float kDefaultDoom = 0.5f;

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
          fDoom(kDefaultDoom)
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
        return d_version(0, 0, 1);
    }

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
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        if (index >= kParameterCount)
            return 0.0f;

        return index == kParameterDoom ? fDoom : 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index == kParameterDoom)
            fDoom = clamp01(value);
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
        const float threshold = dbToGain(settings.thresholdDb);
        const float ratio = std::max(1.0f, settings.ratio);

        for (uint32_t i = 0; i < frames; ++i)
        {
            outLeft[i] = distort(inLeft[i] * inputGain, threshold, ratio, settings.hardClipMix) * outputGain;
            outRight[i] = distort(inRight[i] * inputGain, threshold, ratio, settings.hardClipMix) * outputGain;
        }
    }

private:
    static float dbToGain(const float db) noexcept
    {
        return std::pow(10.0f, db / 20.0f);
    }

    static float distort(const float x,
                         const float threshold,
                         const float ratio,
                         const float hardClipMix) noexcept
    {
        float magnitude = std::fabs(x);

        if (magnitude <= threshold)
            return x;

        const float sign = x < 0.0f ? -1.0f : 1.0f;
        const float soft = sign * (threshold + (magnitude - threshold) / ratio);

        const float hard = sign * std::tanh(magnitude);

        const float mix = clamp01(hardClipMix);
        return soft + (hard - soft) * mix;
    }

    float fDoom;
};

Plugin *createPlugin()
{
    return new DistPlugin();
}

END_NAMESPACE_DISTRHO;
