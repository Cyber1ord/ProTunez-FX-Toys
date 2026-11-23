// Heat parameters (ids prefixed heat_).
#pragma once
#include "../DSP/Heat.h"
#include "ParamHelpers.h"

namespace ptfx::params
{
inline void addHeatParams (Layout& L)
{
    addFloat (L, "heat_drive", "Heat Drive", range (0.0f, 36.0f, 0.0f, 0.1f), 6.0f, "dB");
    addChoice (L, "heat_style", "Heat Style", { "Warm Tube", "Tape", "Transistor", "Diode", "Fuzz" }, 0);
    addFloat (L, "heat_tone", "Heat Tone", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addFloat (L, "heat_lowcut", "Heat Low Cut", range (20.0f, 1000.0f, 120.0f), 20.0f, "Hz");
    addFloat (L, "heat_highcut", "Heat High Cut", range (1000.0f, 20000.0f, 6000.0f), 20000.0f, "Hz");
    addPercent (L, "heat_mix", "Heat Mix", 100.0f);
    addFloat (L, "heat_out", "Heat Output", range (-24.0f, 24.0f, 0.0f, 0.1f), 0.0f, "dB");
    addBool (L, "heat_auto", "Heat Auto Gain", true);
}

inline Heat::Params readHeat (const APVTS& s)
{
    Heat::Params h;
    h.driveDb = get (s, "heat_drive");
    h.style = geti (s, "heat_style");
    h.tone = get (s, "heat_tone") * 0.01f;
    h.lowCutHz = get (s, "heat_lowcut");
    h.highCutHz = get (s, "heat_highcut");
    h.mix = get (s, "heat_mix") * 0.01f;
    h.outputDb = get (s, "heat_out");
    h.autoGain = getb (s, "heat_auto");
    return h;
}
} // namespace ptfx::params
