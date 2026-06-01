
#ifndef LOWPASSFILTER_H_INCLUDED
#define LOWPASSFILTER_H_INCLUDED

#include <algorithm>
#include <cmath>
#include <numbers>

class LowPassFilter
{
public:
    LowPassFilter() = default;

    void prepare(double sampleRate)
    {
        fSampleRate = static_cast<float>(sampleRate);
        updateCoefficient();
        reset();
    }

    void reset()
    {
        fState = 0.0f;
    }

    void setCircuitParams(float cutoff)
    {
        constexpr float Cap = 47.0e-9f;

        cutoff = std::max(1.0f, cutoff);
        fCapacitance = Cap;
        fResistance = 1.0f / (2.0f * std::numbers::pi * cutoff * fCapacitance);
        updateCoefficient();
    }

    void setCircuitElements(float res, float cap)
    {
        fResistance = std::max(1.0f, res);
        fCapacitance = std::max(1.0e-12f, cap);
        updateCoefficient();
    }

    inline float processSample(float x) noexcept
    {
        fState += fCoeff * (x - fState);
        return fState;
    }

private:
    void updateCoefficient()
    {
        constexpr float Pi = 3.14159265358979323846f;

        const float safeSampleRate = std::max(1.0f, fSampleRate);
        const float cutoff = 1.0f / (2.0f * Pi * fResistance * fCapacitance);
        const float clampedCutoff = std::min(cutoff, safeSampleRate * 0.45f);

        fCoeff = 1.0f - std::exp(-2.0f * Pi * clampedCutoff / safeSampleRate);
    }

    float fSampleRate = 48000.0f;
    float fResistance = 4700.0f;
    float fCapacitance = 47.0e-9f;
    float fCoeff = 1.0f;
    float fState = 0.0f;
};

#endif // LOWPASSFILTER_H_INCLUDED
