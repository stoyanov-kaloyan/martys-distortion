#ifndef FUZZCLIPPER_H_INCLUDED
#define FUZZCLIPPER_H_INCLUDED

#include "../chowdsp_wdf.h"
#include <numbers>

using namespace chowdsp::wdft;

class FuzzClipper
{
public:
    FuzzClipper() = default;

    void prepare(double sampleRate)
    {
        C1.prepare((float)sampleRate);
    }

    void reset()
    {
        C1.reset();
    }

    void setCircuitParams(float cutoff)
    {
        constexpr auto Cap = 47.0e-9f;
        const auto Res = 1.0f / (2.0f * std::numbers::pi * cutoff * Cap);

        C1.setCapacitanceValue(Cap);
        R1.setResistanceValue(Res);
    }

    void setCircuitElements(float res, float cap)
    {
        C1.setCapacitanceValue(cap);
        R1.setResistanceValue(res);
    }

    inline float processSample(float x)
    {
        Vs.setVoltage(x);

        dp.incident(P1.reflected());
        P1.incident(dp.reflected());

        return voltage<float>(C1);
    }

private:
    ResistorT<float> R1{4700.0f};
    ResistiveVoltageSourceT<float> Vs;
    WDFSeriesT<float, decltype(Vs), decltype(R1)> S1{Vs, R1};

    CapacitorT<float> C1{47.0e-9f};
    WDFParallelT<float, decltype(S1), decltype(C1)> P1{S1, C1};

    DiodeT<float, decltype(P1)> dp{P1, 1.52e-9f};
    DiodeT<float, decltype(P1)> dp2{P1, 2.52e-9f, 33.57e-3f, 2};
};

#endif // FUZZCLIPPER_H_INCLUDED
