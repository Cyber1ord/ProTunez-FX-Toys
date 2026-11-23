// Heat parameters (ids prefixed heat_).
#pragma once
#include "../DSP/Heat.h"
#include "ParamHelpers.h"

namespace ptfx::params
{
inline void addHeatParams (Layout& L)
{
    addChoice (L, "heat_style", "Heat Style", { "A", "E", "N", "T", "P" }, 2);
    addFloat (L, "heat_drive", "Heat Drive", range (0.0f, 10.0f, 0.0f, 0.01f), 3.0f);
    addBool (L, "heat_punish", "Heat Punish", false);
    addBool (L, "heat_auto", "Heat Auto Gain", true);
    addFloat (L, "heat_out", "Heat Output", range (-24.0f, 24.0f, 0.0f, 0.1f), 0.0f, "dB");
    addPercent (L, "heat_mix", "Heat Mix", 100.0f);
    addFloat (L, "heat_lowcut", "Heat Low Cut", range (20.0f, 1000.0f, 120.0f), 20.0f, "Hz");
    addBool (L, "heat_thump", "Heat Thump", false);
    addFloat (L, "heat_tone", "Heat Tone", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addFloat (L, "heat_highcut", "Heat High Cut", range (1000.0f, 20000.0f, 6000.0f), 20000.0f, "Hz");
    addBool (L, "heat_steep", "Heat Steep", false);
}

inline Heat::Params readHeat (const APVTS& s)
{
    Heat::Params h;
    h.style = geti (s, "heat_style");
    h.drive = get (s, "heat_drive");
    h.punish = getb (s, "heat_punish");
    h.autoGain = getb (s, "heat_auto");
    h.outputDb = get (s, "heat_out");
    h.mix = get (s, "heat_mix") * 0.01f;
    h.lowCutHz = get (s, "heat_lowcut");
    h.thump = getb (s, "heat_thump");
    h.tone = get (s, "heat_tone") * 0.01f;
    h.highCutHz = get (s, "heat_highcut");
    h.steep = getb (s, "heat_steep");
    return h;
}
} // namespace ptfx::params
