// The full parameter list: the effect menu plus every effect's controls.
#pragma once
#include "../EffectIds.h"
#include "EchoParams.h"
#include "HeatParams.h"
#include "PlateParams.h"

namespace ptfx::params
{
inline Layout createLayout()
{
    Layout L;
    juce::StringArray names;
    for (int i = 0; i < kNumEffects; ++i) names.add (effectName ((EffectId) i));
    addChoice (L, "effect", "Effect", names, 0);
    addBool (L, "bypass", "Bypass", false);
    addHeatParams (L);
    addEchoParams (L);
    addPlateParams (L);
    return L;
}
} // namespace ptfx::params
