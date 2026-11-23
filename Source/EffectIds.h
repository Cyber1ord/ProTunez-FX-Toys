// ProTunez FX Toys — the 8 effects, in menu order. Indices are saved in sessions: never reorder.
#pragma once

namespace ptfx
{
enum class EffectId { heat = 0, echo, plate, morph, wider, filter, crystal, space };
constexpr int kNumEffects = 8;

inline bool isBuilt (EffectId e) { return e == EffectId::heat || e == EffectId::echo || e == EffectId::plate; }

inline const char* effectName (EffectId e)
{
    static const char* n[kNumEffects] = { "Heat", "Echo", "Plate", "Voice Morph", "Wider", "Filter", "Crystal Echo", "Space" };
    return n[(int) e];
}

inline const char* effectLike (EffectId e)
{
    static const char* n[kNumEffects] = { "Decapitator", "EchoBoy", "SuperPlate", "Little AlterBoy", "MicroShift", "FilterFreak", "Crystallizer", "SpaceBlender" };
    return n[(int) e];
}

/** Parameter id prefix for each effect ("heat_", ...). */
inline const char* effectPrefix (EffectId e)
{
    static const char* n[kNumEffects] = { "heat_", "echo_", "plate_", "morph_", "wider_", "filter_", "crystal_", "space_" };
    return n[(int) e];
}
} // namespace ptfx
