// Echo parameters (ids prefixed echo_). Style Edit controls are echo_s_*.
#pragma once
#include "../DSP/Echo.h"
#include "ParamHelpers.h"

namespace ptfx::params
{
inline juce::StringArray echoStyleNames()
{
    juce::StringArray s;
    for (auto& d : Echo::styles()) s.add (d.name);
    return s;
}

inline void addEchoParams (Layout& L)
{
    const juce::StringArray notes { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars" };
    addChoice (L, "echo_mode", "Echo Mode", { "Single", "Dual", "Ping-Pong", "Rhythm" }, 0);
    addChoice (L, "echo_timemode", "Echo Time Mode", { "Time", "Note", "Dot", "Trip" }, 1);
    addFloat (L, "echo_time", "Echo Time", range (1.0f, 2500.0f, 300.0f, 0.1f), 375.0f, "ms");
    addChoice (L, "echo_note", "Echo Note", notes, 3);
    addFloat (L, "echo_time2", "Echo Time 2", range (1.0f, 2500.0f, 300.0f, 0.1f), 500.0f, "ms");
    addChoice (L, "echo_note2", "Echo Note 2", notes, 4);
    addBool (L, "echo_prime", "Echo Prime Numbers", false);
    addFloat (L, "echo_fb", "Echo Feedback", range (0.0f, 110.0f, 0.0f, 0.1f), 35.0f, "%");
    addPercent (L, "echo_sat", "Echo Saturation", 20.0f);
    addFloat (L, "echo_lowcut", "Echo Low Cut", range (20.0f, 2000.0f, 200.0f), 20.0f, "Hz");
    addFloat (L, "echo_highcut", "Echo High Cut", range (500.0f, 20000.0f, 5000.0f), 20000.0f, "Hz");
    addPercent (L, "echo_mix", "Echo Mix", 25.0f);
    addFloat (L, "echo_in", "Echo Input", range (-24.0f, 24.0f, 0.0f, 0.1f), 0.0f, "dB");
    addFloat (L, "echo_out", "Echo Output", range (-24.0f, 24.0f, 0.0f, 0.1f), 0.0f, "dB");
    addFloat (L, "echo_groove", "Echo Groove", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addFloat (L, "echo_feel", "Echo Feel", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addFloat (L, "echo_accent", "Echo Accent", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addFloat (L, "echo_accent2", "Echo Accent 2", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addPercent (L, "echo_width", "Echo Width", 100.0f);
    addFloat (L, "echo_lroffset", "Echo L/R Offset", range (0.0f, 25.0f, 0.0f, 0.1f), 8.0f, "ms");
    addPercent (L, "echo_fbmix", "Echo Feedback Mix", 0.0f);
    addFloat (L, "echo_balance", "Echo Balance", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addFloat (L, "echo_fbbal", "Echo Feedback Balance", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addFloat (L, "echo_rbeats", "Echo Rhythm Length", range (1.0f, 8.0f, 0.0f, 1.0f), 4.0f);
    addChoice (L, "echo_grid", "Echo Rhythm Grid", { "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" }, 3);
    addFloat (L, "echo_repeats", "Echo Repeats", range (1.0f, 16.0f, 0.0f, 1.0f), 4.0f);
    addChoice (L, "echo_shape", "Echo Shape", { "Decay", "Reverse", "Swell", "Fade", "Nonlin", "Flat" }, 0);
    addFloat (L, "echo_warp", "Echo Warp", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addChoice (L, "echo_pan", "Echo Pan Shape", { "Double", "Center", "Alt 1", "Alt 2", "Alt 3", "Sweep L", "Sweep R", "Pan" }, 1);
    addPercent (L, "echo_duck", "Echo Duck", 0.0f);
    addBool (L, "echo_freeze", "Echo Freeze", false);
    addBool (L, "echo_usehost", "Echo Follow Host Tempo", true);
    addFloat (L, "echo_bpm", "Echo Tempo", range (30.0f, 300.0f, 0.0f, 0.1f), 120.0f, "bpm");
    addChoice (L, "echo_style", "Echo Style", echoStyleNames(), 1);
    // Style Edit
    const auto st = Echo::styles()[1].edit;
    addFloat (L, "echo_s_bandlo", "Echo Style Band Low", range (20.0f, 2000.0f, 200.0f), st.bandLowHz, "Hz");
    addFloat (L, "echo_s_bandhi", "Echo Style Band High", range (500.0f, 20000.0f, 5000.0f), st.bandHighHz, "Hz");
    addFloat (L, "echo_s_lowf", "Echo EQ Low Freq", range (30.0f, 1000.0f, 150.0f), st.lowF, "Hz");
    addFloat (L, "echo_s_lowg", "Echo EQ Low Gain", range (-18.0f, 18.0f, 0.0f, 0.1f), st.lowG, "dB");
    addFloat (L, "echo_s_lowdec", "Echo EQ Low Decay", range (-6.0f, 6.0f, 0.0f, 0.1f), st.lowDecay, "dB");
    addFloat (L, "echo_s_midf", "Echo EQ Mid Freq", range (200.0f, 8000.0f, 1000.0f), st.midF, "Hz");
    addFloat (L, "echo_s_midg", "Echo EQ Mid Gain", range (-18.0f, 18.0f, 0.0f, 0.1f), st.midG, "dB");
    addFloat (L, "echo_s_middec", "Echo EQ Mid Decay", range (-6.0f, 6.0f, 0.0f, 0.1f), st.midDecay, "dB");
    addPercent (L, "echo_s_midres", "Echo EQ Mid Res", st.midRes * 100.0f);
    addFloat (L, "echo_s_highf", "Echo EQ High Freq", range (1000.0f, 16000.0f, 5000.0f), st.highF, "Hz");
    addFloat (L, "echo_s_highg", "Echo EQ High Gain", range (-18.0f, 18.0f, 0.0f, 0.1f), st.highG, "dB");
    addFloat (L, "echo_s_highdec", "Echo EQ High Decay", range (-6.0f, 6.0f, 0.0f, 0.1f), st.highDecay, "dB");
    addChoice (L, "echo_s_sat", "Echo Saturation Type",
               { "Clean", "Tape", "Warm", "Pump", "Dirt", "Hard Limit", "Soft Limit", "Warm Limit", "Bright Limit" }, st.satType);
    addPercent (L, "echo_s_diff", "Echo Diffusion", st.diffAmount * 100.0f);
    addPercent (L, "echo_s_diffsize", "Echo Diffusion Size", st.diffSize * 100.0f);
    addChoice (L, "echo_s_diffloop", "Echo Diffusion Mode", { "Post", "Loop" }, st.diffLoop ? 1 : 0);
    addPercent (L, "echo_s_wobdepth", "Echo Wobble Depth", st.wobDepth * 100.0f);
    addFloat (L, "echo_s_wobrate", "Echo Wobble Rate", range (0.05f, 10.0f, 1.0f, 0.01f), st.wobRate, "Hz");
    addChoice (L, "echo_s_wobshape", "Echo Wobble Shape", { "Triangle", "Sine", "Square", "Random Walk", "Random S/H" }, st.wobShape);
    addBool (L, "echo_s_wobsync", "Echo Wobble Sync", st.wobSync);
}

inline Echo::Params readEcho (const APVTS& s)
{
    Echo::Params e;
    e.mode = geti (s, "echo_mode");
    e.timeMode = geti (s, "echo_timemode");
    e.timeMs = get (s, "echo_time");
    e.note = geti (s, "echo_note");
    e.time2Ms = get (s, "echo_time2");
    e.note2 = geti (s, "echo_note2");
    e.prime = getb (s, "echo_prime");
    e.feedback = get (s, "echo_fb") * 0.01f;
    e.saturation = get (s, "echo_sat") * 0.01f;
    e.lowCutHz = get (s, "echo_lowcut");
    e.highCutHz = get (s, "echo_highcut");
    e.mix = get (s, "echo_mix") * 0.01f;
    e.inDb = get (s, "echo_in");
    e.outDb = get (s, "echo_out");
    e.groove = get (s, "echo_groove") * 0.01f;
    e.feel = get (s, "echo_feel") * 0.01f;
    e.accent = get (s, "echo_accent") * 0.01f;
    e.accent2 = get (s, "echo_accent2") * 0.01f;
    e.width = get (s, "echo_width") * 0.01f;
    e.lrOffsetMs = get (s, "echo_lroffset");
    e.fbMix = get (s, "echo_fbmix") * 0.01f;
    e.balance = get (s, "echo_balance") * 0.01f;
    e.fbBalance = get (s, "echo_fbbal") * 0.01f;
    e.rhythmBeats = geti (s, "echo_rbeats");
    e.grid = geti (s, "echo_grid");
    e.repeats = geti (s, "echo_repeats");
    e.shape = geti (s, "echo_shape");
    e.warp = get (s, "echo_warp") * 0.01f;
    e.panShape = geti (s, "echo_pan");
    e.duck = get (s, "echo_duck") * 0.01f;
    e.freeze = getb (s, "echo_freeze");
    e.styleIndex = geti (s, "echo_style");
    auto& st = e.style;
    st.bandLowHz = get (s, "echo_s_bandlo");
    st.bandHighHz = get (s, "echo_s_bandhi");
    st.lowF = get (s, "echo_s_lowf");
    st.lowG = get (s, "echo_s_lowg");
    st.lowDecay = get (s, "echo_s_lowdec");
    st.midF = get (s, "echo_s_midf");
    st.midG = get (s, "echo_s_midg");
    st.midDecay = get (s, "echo_s_middec");
    st.midRes = get (s, "echo_s_midres") * 0.01f;
    st.highF = get (s, "echo_s_highf");
    st.highG = get (s, "echo_s_highg");
    st.highDecay = get (s, "echo_s_highdec");
    st.satType = geti (s, "echo_s_sat");
    st.diffAmount = get (s, "echo_s_diff") * 0.01f;
    st.diffSize = get (s, "echo_s_diffsize") * 0.01f;
    st.diffLoop = geti (s, "echo_s_diffloop") == 1;
    st.wobDepth = get (s, "echo_s_wobdepth") * 0.01f;
    st.wobRate = get (s, "echo_s_wobrate");
    st.wobShape = geti (s, "echo_s_wobshape");
    st.wobSync = getb (s, "echo_s_wobsync");
    return e;
}

/** Copies a style's settings into the Style Edit parameters (used when a style is picked on screen). */
inline void applyEchoStyle (APVTS& s, int index)
{
    if (index < 0 || index >= Echo::numStyles()) return;
    const auto& st = Echo::styles()[(size_t) index].edit;
    auto set = [&s] (const char* id, float v) { if (auto* p = s.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set ("echo_style", (float) index);
    set ("echo_s_bandlo", st.bandLowHz); set ("echo_s_bandhi", st.bandHighHz);
    set ("echo_s_lowf", st.lowF); set ("echo_s_lowg", st.lowG); set ("echo_s_lowdec", st.lowDecay);
    set ("echo_s_midf", st.midF); set ("echo_s_midg", st.midG); set ("echo_s_middec", st.midDecay); set ("echo_s_midres", st.midRes * 100.0f);
    set ("echo_s_highf", st.highF); set ("echo_s_highg", st.highG); set ("echo_s_highdec", st.highDecay);
    set ("echo_s_sat", (float) st.satType);
    set ("echo_s_diff", st.diffAmount * 100.0f); set ("echo_s_diffsize", st.diffSize * 100.0f); set ("echo_s_diffloop", st.diffLoop ? 1.0f : 0.0f);
    set ("echo_s_wobdepth", st.wobDepth * 100.0f); set ("echo_s_wobrate", st.wobRate);
    set ("echo_s_wobshape", (float) st.wobShape); set ("echo_s_wobsync", st.wobSync ? 1.0f : 0.0f);
}
} // namespace ptfx::params
