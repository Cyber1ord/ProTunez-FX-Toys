// Picks the screen for each effect.
#include "EffectView.h"

namespace ptfx::ui
{
std::unique_ptr<EffectView> makeHeatView (ProTunezFXToysProcessor&);
std::unique_ptr<EffectView> makeEchoView (ProTunezFXToysProcessor&);

std::unique_ptr<EffectView> makeView (EffectId e, ProTunezFXToysProcessor& p)
{
    switch (e)
    {
        case EffectId::heat: return makeHeatView (p);
        case EffectId::echo: return makeEchoView (p);
        case EffectId::plate:
        case EffectId::morph: case EffectId::wider: case EffectId::filter: case EffectId::crystal: case EffectId::space:
        default: return std::make_unique<ComingSoonView> (p, e);
    }
}
} // namespace ptfx::ui
