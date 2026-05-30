/* DPF plugin include */
#include "DistrhoPlugin.hpp"

#include <cstring>

/* Make DPF related classes available for us to use without any extra namespace references */
USE_NAMESPACE_DISTRHO;

START_NAMESPACE_DISTRHO;

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
        : Plugin(0, 0, 0) // 0 parameters, 0 programs and 0 states
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
        return "Mute";
    }

    /**
       Get the plugin author/maker.
     */
    const char *getMaker() const override
    {
        return "DPF";
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
        return d_cconst('M', 'u', 't', 'e');
    }

    /* ----------------------------------------------------------------------------------------
     * Audio/MIDI Processing */

    /**
       Run/process function for plugins without MIDI input.
     */
    void run(const float **, float **outputs, uint32_t frames) override
    {
        // get the left and right audio outputs
        float *const outL = outputs[0];
        float *const outR = outputs[1];

        // mute audio
        std::memset(outL, 0, sizeof(float) * frames);
        std::memset(outR, 0, sizeof(float) * frames);
    }
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
