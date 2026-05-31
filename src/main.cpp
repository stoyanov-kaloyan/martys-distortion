/* DPF plugin include */
#include "DistrhoPlugin.hpp"

#include <algorithm>
#include <cmath>

/* Make DPF related classes available for us to use without any extra namespace references */
USE_NAMESPACE_DISTRHO;

START_NAMESPACE_DISTRHO;

namespace
{

constexpr float kDefaultThresholdDB = -27.0f;
constexpr float kDefaultRatio = 10.0f;
constexpr float kDefaultInputGainDB = 0.0f;
constexpr float kDefaultOutputGainDB = 0.0f;
constexpr float kDefaultFlip = 0.0f;
constexpr float kDefaultRect = 0.0f;

} // namespace

/**
   Our custom plugin class.
   Subclassing `Plugin` from DPF is how this all works.

   By default, only information-related functions and `run` are pure virtual (that is, must be reimplemented).
   When enabling certain features (such as programs or states, more on that below), a few extra functions also need to be reimplemented.
 */
class MutePlugin : public Plugin
{
public:
    /**
       Plugin class constructor.
     */
    MutePlugin()
        : Plugin(kParameterCount, 0, 0), // 6 parameters, 0 programs and 0 states
          fParams{kDefaultThresholdDB,
                  kDefaultRatio,
                  kDefaultInputGainDB,
                  kDefaultOutputGainDB,
                  kDefaultFlip,
                  kDefaultRect}
    {
    }

protected:
    /* ----------------------------------------------------------------------------------------
     * Information */

    /**
       Get the plugin label.
       This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
     */
    const char *getLabel() const override
    {
        return "DihStortion";
    }

    /**
       Get the plugin author/maker.
     */
    const char *getMaker() const override
    {
        return "Kaloyanchester";
    }

    /**
       Get the plugin license name (a single line of text).
       For commercial plugins this should return some short copyright information.
     */
    const char *getLicense() const override
    {
        return "MIT";
    }

    /**
       Get the plugin version, in hexadecimal.
     */
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
        case kParameterThreshold:
            parameter.name = "Threshold";
            parameter.symbol = "threshold";
            parameter.ranges.min = -100.0f;
            parameter.ranges.max = 0.0f;
            parameter.ranges.def = kDefaultThresholdDB;
            break;

        case kParameterRatio:
            parameter.name = "Ratio";
            parameter.symbol = "ratio";
            parameter.ranges.min = 1.0f;
            parameter.ranges.max = 50.0f;
            parameter.ranges.def = kDefaultRatio;
            break;

        case kParameterInputGain:
            parameter.name = "Input Gain";
            parameter.symbol = "input_gain";
            parameter.unit = "dB";
            parameter.ranges.min = -30.0f;
            parameter.ranges.max = 6.0f;
            parameter.ranges.def = kDefaultInputGainDB;
            break;

        case kParameterOutputGain:
            parameter.name = "Output Gain";
            parameter.symbol = "output_gain";
            parameter.unit = "dB";
            parameter.ranges.min = -30.0f;
            parameter.ranges.max = 6.0f;
            parameter.ranges.def = kDefaultOutputGainDB;
            break;

        case kParameterFlip:
            parameter.hints |= kParameterIsBoolean;
            parameter.name = "Flip";
            parameter.symbol = "flip";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = kDefaultFlip;
            break;

        case kParameterRect:
            parameter.hints |= kParameterIsBoolean;
            parameter.name = "Rect";
            parameter.symbol = "rect";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = kDefaultRect;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        if (index >= kParameterCount)
            return 0.0f;

        return fParams[index];
    }

    void setParameterValue(uint32_t index, float value) override
    {
        if (index >= kParameterCount)
            return;

        fParams[index] = clampParameter(index, value);
    }

    void run(const float **inputs, float **outputs, uint32_t frames) override
    {
        const float *const inLeft = inputs[0];
        const float *const inRight = inputs[1];
        float *const outLeft = outputs[0];
        float *const outRight = outputs[1];

        const float inputGain = dbToGain(fParams[kParameterInputGain]);
        const float outputGain = dbToGain(fParams[kParameterOutputGain]);
        const float threshold = dbToGain(fParams[kParameterThreshold]);
        const float ratio = std::max(1.0f, fParams[kParameterRatio]);
        const bool flip = fParams[kParameterFlip] >= 0.5f;
        const bool rect = fParams[kParameterRect] >= 0.5f;

        for (uint32_t i = 0; i < frames; ++i)
        {
            outLeft[i] = chow(inLeft[i] * inputGain, threshold, ratio, flip, rect) * outputGain;
            outRight[i] = chow(inRight[i] * inputGain, threshold, ratio, flip, rect) * outputGain;
        }
    }

private:
    static float dbToGain(const float db) noexcept
    {
        return std::pow(10.0f, db / 20.0f);
    }

    static float chow(const float x,
                      const float threshold,
                      const float ratio,
                      const bool flip,
                      const bool rect) noexcept
    {
        float y = x;

        if (!flip && x > threshold)
        {
            y = threshold;

            if (!rect)
                y += (x - threshold) / ratio;
        }
        else if (flip && x < -threshold)
        {
            y = -threshold;

            if (!rect)
                y += (x + threshold) / ratio;
        }

        return y;
    }

    static float clampParameter(const uint32_t index, const float value) noexcept
    {
        switch (index)
        {
        case kParameterThreshold:
            return std::max(-100.0f, std::min(0.0f, value));
        case kParameterRatio:
            return std::max(1.0f, std::min(50.0f, value));
        case kParameterInputGain:
        case kParameterOutputGain:
            return std::max(-30.0f, std::min(6.0f, value));
        case kParameterFlip:
        case kParameterRect:
            return value >= 0.5f ? 1.0f : 0.0f;
        default:
            return value;
        }
    }

    float fParams[kParameterCount];
};

/**
   Create an instance of the Plugin class.
   This is the entry point for DPF plugins.
   DPF will call this to either create an instance of your plugin for the host or to fetch some initial information for internal caching.
 */
Plugin *createPlugin()
{
    return new MutePlugin();
}

END_NAMESPACE_DISTRHO;
