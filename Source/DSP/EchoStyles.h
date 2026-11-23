// ProTunez FX Toys — Echo's 30 styles: our own versions of classic echo machines and effects.
// Each style is a starting point for the Style Edit panel (band, EQ, saturation, diffusion, wobble).
#pragma once

namespace ptfx
{
inline const std::vector<Echo::StyleDef>& Echo::styles()
{
    // band lo, band hi, satType, wobble depth, rate, shape, diffusion, EQ tweaks
    auto make = [] (const char* name, float lo, float hi, int sat, float wob, float rate, int wshape, float diff,
                    float lowG = 0, float midF = 1000, float midG = 0, float highG = 0, float highDecay = 0, float lowDecay = 0)
    {
        Echo::StyleEdit e;
        e.bandLowHz = lo; e.bandHighHz = hi; e.satType = sat;
        e.wobDepth = wob; e.wobRate = rate; e.wobShape = wshape;
        e.diffAmount = diff; e.diffLoop = diff > 0.4f;
        e.lowG = lowG; e.midF = midF; e.midG = midG; e.highG = highG; e.highDecay = highDecay; e.lowDecay = lowDecay;
        return Echo::StyleDef { name, e };
    };
    using E = Echo;
    static const std::vector<Echo::StyleDef> list = {
        make ("Master Tape",     30, 16000, 1, 0.10f, 0.6f, E::wobSine, 0.0f, 1.0f, 1000, 0, 0, -0.5f),
        make ("Studio Tape",     40, 12000, 1, 0.15f, 0.7f, E::wobSine, 0.0f, 1.5f, 1000, 0, -1, -1.0f),
        make ("Tube Tape",       60, 9000, 2, 0.20f, 0.8f, E::wobSine, 0.05f, 2.0f, 800, 1, -2, -1.5f),
        make ("Cheap Tape",      120, 6000, 1, 0.45f, 1.2f, E::wobRandomWalk, 0.05f, 0, 1200, 2, -3, -2.0f),
        make ("Tape Loop",       80, 7000, 1, 0.35f, 0.5f, E::wobSine, 0.1f, 1.0f, 1000, 0, -2, -1.5f, -0.5f),
        make ("Space Tape",      100, 6500, 1, 0.30f, 0.9f, E::wobSine, 0.3f, 0, 1500, 2, -2, -1.5f, -1.0f),
        make ("Bucket Analog",   80, 4500, 2, 0.20f, 0.4f, E::wobTriangle, 0.0f, 0, 1000, 0, -4, -2.0f),
        make ("Analog Bright",   60, 8000, 2, 0.15f, 0.4f, E::wobTriangle, 0.0f, 0, 2500, 2, 0, -1.0f),
        make ("Oil Can",         200, 4000, 3, 0.55f, 3.5f, E::wobSine, 0.5f, -2, 1000, 3, -4, -2.0f),
        make ("Transistor Box",  150, 5000, 4, 0.25f, 0.6f, E::wobTriangle, 0.0f, -1, 1500, 2, -2, -1.5f),
        make ("Telephone",       300, 3400, 4, 0.0f, 0.5f, E::wobSine, 0.0f, 0, 1500, 6, 0, 0.0f),
        make ("AM Radio",        250, 4500, 4, 0.05f, 0.3f, E::wobRandomWalk, 0.0f, 0, 1200, 4, -2, 0.0f),
        make ("FM Radio",        60, 15000, 6, 0.0f, 0.3f, E::wobSine, 0.0f, 0, 3000, 1, 2, 0.0f),
        make ("Shortwave",       400, 3000, 4, 0.40f, 6.0f, E::wobRandomSH, 0.0f, 0, 1500, 6, 0, 0.0f),
        make ("Transmitter",     300, 5000, 5, 0.25f, 4.0f, E::wobSquare, 0.0f, 0, 2000, 4, 0, 0.0f),
        make ("Clean Digital",   20, 20000, 0, 0.0f, 0.5f, E::wobSine, 0.0f),
        make ("Cheap Digital",   80, 10000, 5, 0.0f, 0.5f, E::wobSine, 0.0f, 0, 1000, 0, 0, -0.5f),
        make ("Digital Chorus",  40, 16000, 0, 0.60f, 1.2f, E::wobSine, 0.0f),
        make ("Analog Chorus",   60, 9000, 2, 0.70f, 0.9f, E::wobTriangle, 0.0f, 0, 1000, 0, -2, -1.0f),
        make ("Vintage Chorus",  80, 7500, 2, 0.80f, 0.6f, E::wobTriangle, 0.1f, 0, 1000, 0, -3, -1.0f),
        make ("Vibrato",         40, 12000, 1, 1.00f, 5.5f, E::wobSine, 0.0f),
        make ("Saturated",       40, 14000, 1, 0.10f, 0.6f, E::wobSine, 0.0f, 1, 1000, 0, 0, -0.5f),
        make ("Fat",             30, 8000, 2, 0.10f, 0.5f, E::wobSine, 0.0f, 4, 250, 2, -3, -1.0f, 0.5f),
        make ("Distressed",      150, 6000, 4, 0.50f, 1.5f, E::wobRandomWalk, 0.1f, -2, 1500, 3, -3, -2.0f),
        make ("Limited",         40, 14000, 6, 0.05f, 0.5f, E::wobSine, 0.0f),
        make ("Distorted",       120, 9000, 4, 0.10f, 0.5f, E::wobSine, 0.0f, 0, 1200, 4, -1, -1.0f),
        make ("Squeaky",         600, 9000, 8, 0.20f, 2.0f, E::wobSine, 0.0f, -6, 3000, 6, 2, 0.5f),
        make ("Ambient",         80, 10000, 1, 0.25f, 0.4f, E::wobSine, 0.45f, 0, 1000, 0, -2, -1.0f),
        make ("Diffused",        60, 12000, 0, 0.10f, 0.5f, E::wobSine, 0.8f),
        make ("Splattered",      200, 8000, 4, 0.40f, 2.5f, E::wobRandomSH, 0.6f, 0, 1500, 4, -2, -1.5f),
    };
    return list;
}
} // namespace ptfx
