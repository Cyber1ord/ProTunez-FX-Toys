// Echo parameters (ids prefixed echo_).
#pragma once
#include "../DSP/Echo.h"
#include "ParamHelpers.h"

namespace ptfx::params
{
inline void addEchoParams (Layout& L)
{
    juce::StringArray divs;
    for (auto* n : Echo::divisionNames) divs.add (n);
    addFloat (L, "echo_time", "Echo Time", range (1.0f, 2000.0f, 300.0f), 375.0f, "ms");
    addBool (L, "echo_sync", "Echo Sync", false);
    addChoice (L, "echo_div", "Echo Note", divs, 8);
    addPercent (L, "echo_fb", "Echo Feedback", 35.0f);
    addChoice (L, "echo_mode", "Echo Mode", { "Single", "Ping-Pong", "Wide" }, 0);
    addChoice (L, "echo_style", "Echo Style", { "Digital", "Tape", "Analog", "Lo-Fi" }, 1);
    addFloat (L, "echo_lowcut", "Echo Low Cut", range (20.0f, 2000.0f, 200.0f), 120.0f, "Hz");
    addFloat (L, "echo_highcut", "Echo High Cut", range (500.0f, 20000.0f, 5000.0f), 6000.0f, "Hz");
    addPercent (L, "echo_wobble", "Echo Wobble", 20.0f);
    addPercent (L, "echo_sat", "Echo Saturation", 20.0f);
    addPercent (L, "echo_width", "Echo Width", 100.0f);
    addPercent (L, "echo_mix", "Echo Mix", 25.0f);
}

inline Echo::Params readEcho (const APVTS& s)
{
    Echo::Params e;
    e.timeMs = get (s, "echo_time");
    e.sync = getb (s, "echo_sync");
    e.division = geti (s, "echo_div");
    e.feedback = get (s, "echo_fb") * 0.01f;
    e.mode = geti (s, "echo_mode");
    e.style = geti (s, "echo_style");
    e.lowCutHz = get (s, "echo_lowcut");
    e.highCutHz = get (s, "echo_highcut");
    e.wobble = get (s, "echo_wobble") * 0.01f;
    e.saturation = get (s, "echo_sat") * 0.01f;
    e.width = get (s, "echo_width") * 0.01f;
    e.mix = get (s, "echo_mix") * 0.01f;
    return e;
}
} // namespace ptfx::params
