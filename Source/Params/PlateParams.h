// Plate parameters (ids prefixed plate_).
#pragma once
#include "../DSP/Plate.h"
#include "ParamHelpers.h"

namespace ptfx::params
{
inline void addPlateParams (Layout& L)
{
    addFloat (L, "plate_pre", "Plate Pre-Delay", range (0.0f, 250.0f, 0.0f, 0.1f), 20.0f, "ms");
    addPercent (L, "plate_decay", "Plate Decay", 55.0f);
    addPercent (L, "plate_size", "Plate Size", 70.0f);
    addPercent (L, "plate_damp", "Plate Damping", 40.0f);
    addFloat (L, "plate_lowcut", "Plate Low Cut", range (20.0f, 1000.0f, 150.0f), 150.0f, "Hz");
    addFloat (L, "plate_highcut", "Plate High Cut", range (1000.0f, 20000.0f, 7000.0f), 12000.0f, "Hz");
    addPercent (L, "plate_mod", "Plate Modulation", 30.0f);
    addPercent (L, "plate_width", "Plate Width", 100.0f);
    addPercent (L, "plate_mix", "Plate Mix", 100.0f);
}

inline Plate::Params readPlate (const APVTS& s)
{
    Plate::Params p;
    p.preDelayMs = get (s, "plate_pre");
    p.decay = get (s, "plate_decay") * 0.01f;
    p.size = get (s, "plate_size") * 0.01f;
    p.damping = get (s, "plate_damp") * 0.01f;
    p.lowCutHz = get (s, "plate_lowcut");
    p.highCutHz = get (s, "plate_highcut");
    p.modulation = get (s, "plate_mod") * 0.01f;
    p.width = get (s, "plate_width") * 0.01f;
    p.mix = get (s, "plate_mix") * 0.01f;
    return p;
}
} // namespace ptfx::params
