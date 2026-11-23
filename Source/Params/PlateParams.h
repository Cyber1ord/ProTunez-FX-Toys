// Plate parameters (ids prefixed plate_).
#pragma once
#include "../DSP/Plate.h"
#include "ParamHelpers.h"

namespace ptfx::params
{
inline void addPlateParams (Layout& L)
{
    juce::StringArray models;
    for (int i = 0; i < Plate::numModels; ++i) models.add (Plate::model (i).name);
    addChoice (L, "plate_model", "Plate Model", models, 0);
    addChoice (L, "plate_preamp", "Plate Preamp", { "Tube", "Solid-State", "Clean" }, 0);
    addFloat (L, "plate_decay", "Plate Decay", range (0.5f, 60.0f, 4.0f, 0.01f), 2.4f, "s");
    addFloat (L, "plate_pre", "Plate Pre-Delay", range (0.0f, 250.0f, 40.0f, 0.1f), 20.0f, "ms");
    addPercent (L, "plate_mod", "Plate Modulation", 30.0f);
    addFloat (L, "plate_modrate", "Plate Mod Rate", range (0.1f, 5.0f, 1.0f, 0.01f), 0.8f, "Hz");
    addFloat (L, "plate_low", "Plate Low Cut", range (20.0f, 1000.0f, 150.0f), 20.0f, "Hz");
    addFloat (L, "plate_high", "Plate High Cut", range (1000.0f, 20000.0f, 7000.0f), 20000.0f, "Hz");
    addChoice (L, "plate_lowslope", "Plate Low Cut Slope", { "6 dB/oct", "12 dB/oct", "24 dB/oct" }, 1);
    addFloat (L, "plate_eq1f", "Plate EQ 1 Freq", range (40.0f, 18000.0f, 1000.0f), 400.0f, "Hz");
    addFloat (L, "plate_eq1g", "Plate EQ 1 Gain", range (-18.0f, 18.0f, 0.0f, 0.1f), 0.0f, "dB");
    addFloat (L, "plate_eq1q", "Plate EQ 1 Q", range (0.2f, 8.0f, 1.0f, 0.01f), 1.0f);
    addFloat (L, "plate_eq2f", "Plate EQ 2 Freq", range (40.0f, 18000.0f, 1000.0f), 4000.0f, "Hz");
    addFloat (L, "plate_eq2g", "Plate EQ 2 Gain", range (-18.0f, 18.0f, 0.0f, 0.1f), 0.0f, "dB");
    addFloat (L, "plate_eq2q", "Plate EQ 2 Q", range (0.2f, 8.0f, 1.0f, 0.01f), 1.0f);
    addFloat (L, "plate_width", "Plate Width", range (0.0f, 200.0f, 0.0f, 0.1f), 100.0f, "%");
    addFloat (L, "plate_balance", "Plate Balance", range (-100.0f, 100.0f, 0.0f, 1.0f), 0.0f);
    addFloat (L, "plate_in", "Plate Input", range (-24.0f, 24.0f, 0.0f, 0.1f), 0.0f, "dB");
    addFloat (L, "plate_out", "Plate Output", range (-24.0f, 24.0f, 0.0f, 0.1f), 0.0f, "dB");
    addPercent (L, "plate_mix", "Plate Mix", 100.0f);
    addPercent (L, "plate_duck", "Plate Duck", 0.0f);
    addBool (L, "plate_freeze", "Plate Freeze", false);
}

inline Plate::Params readPlate (const APVTS& s)
{
    Plate::Params p;
    p.model = geti (s, "plate_model");
    p.preamp = geti (s, "plate_preamp");
    p.decaySec = get (s, "plate_decay");
    p.preDelayMs = get (s, "plate_pre");
    p.modDepth = get (s, "plate_mod") * 0.01f;
    p.modRateHz = get (s, "plate_modrate");
    p.lowHz = get (s, "plate_low");
    p.highHz = get (s, "plate_high");
    p.lowSlope = geti (s, "plate_lowslope");
    p.eq1F = get (s, "plate_eq1f"); p.eq1G = get (s, "plate_eq1g"); p.eq1Q = get (s, "plate_eq1q");
    p.eq2F = get (s, "plate_eq2f"); p.eq2G = get (s, "plate_eq2g"); p.eq2Q = get (s, "plate_eq2q");
    p.width = get (s, "plate_width") * 0.01f;
    p.balance = get (s, "plate_balance") * 0.01f;
    p.inDb = get (s, "plate_in");
    p.outDb = get (s, "plate_out");
    p.mix = get (s, "plate_mix") * 0.01f;
    p.duck = get (s, "plate_duck") * 0.01f;
    p.freeze = getb (s, "plate_freeze");
    return p;
}
} // namespace ptfx::params
