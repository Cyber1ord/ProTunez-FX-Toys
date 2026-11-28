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
        return Echo::StyleDef { name, e, {} };
    };
    using E = Echo;
    using Kind = Echo::EngineKind;
    // Engine builders, in the order of the Echo deep engines table.
    auto tapeE = [] (float drive, float hyst, float gap, float bump, float wow, float flutter, float hissDb)
    {
        Echo::EngineDef d; d.kind = Kind::tape; d.tapeDrive = drive; d.hystMix = hyst; d.gapHz = gap; d.bumpDb = bump;
        d.wow = wow; d.flutter = flutter; d.noiseDb = hissDb; return d;
    };
    auto bbdE = [] (float scale, bool compander, float noiseDb)
    {
        Echo::EngineDef d; d.kind = Kind::bbd; d.bbdScale = scale; d.compander = compander; d.noiseDb = noiseDb; return d;
    };
    auto digiE = [] (int bits, float memSamples)
    {
        Echo::EngineDef d; d.kind = Kind::digital; d.bits = bits; d.memSamples = memSamples; return d;
    };
    auto radioE = [] (float lo, float hi, float drive, float asym, float noiseDb, float fadeDepth, float fadeHz)
    {
        Echo::EngineDef d; d.kind = Kind::radio; d.loHz = lo; d.hiHz = hi; d.drive = drive; d.asym = asym;
        d.noiseDb = noiseDb; d.fadeDepth = fadeDepth; d.fadeHz = fadeHz; return d;
    };
    auto springE = [] (float amt, float size, float modMs, float hz)
    {
        Echo::EngineDef d; d.kind = Kind::spring; d.smearAmt = amt; d.smearSize = size; d.smearModMs = modMs; d.smearHz = hz; return d;
    };
    static const std::vector<Echo::StyleDef> list = [&]
    {
    std::vector<Echo::StyleDef> v = {
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
    v[0]  = { v[0].name,  v[0].edit,  tapeE (1.0f, 0.25f, 16000, 1.0f, 0.2f, 0.2f, -78) };
    v[1]  = { v[1].name,  v[1].edit,  tapeE (1.2f, 0.30f, 13000, 1.5f, 0.3f, 0.3f, -74) };
    v[2]  = { v[2].name,  v[2].edit,  tapeE (1.5f, 0.35f, 10000, 2.0f, 0.3f, 0.3f, -72) };
    v[3]  = { v[3].name,  v[3].edit,  tapeE (2.0f, 0.50f, 7000, 2.5f, 1.0f, 0.8f, -64) };
    v[4]  = { v[4].name,  v[4].edit,  tapeE (1.5f, 0.40f, 8000, 2.0f, 0.6f, 0.5f, -68) };
    v[5]  = { v[5].name,  v[5].edit,  tapeE (1.5f, 0.40f, 8000, 2.0f, 0.5f, 0.6f, -68) };
    v[21] = { v[21].name, v[21].edit, tapeE (2.5f, 0.60f, 12000, 1.0f, 0.2f, 0.2f, -74) };
    v[22] = { v[22].name, v[22].edit, tapeE (1.5f, 0.40f, 9000, 3.0f, 0.2f, 0.2f, -72) };
    v[23] = { v[23].name, v[23].edit, tapeE (2.5f, 0.60f, 6000, 2.5f, 1.0f, 1.0f, -62) };
    v[27] = { v[27].name, v[27].edit, tapeE (1.0f, 0.25f, 10000, 1.0f, 0.3f, 0.3f, -74) };
    v[6]  = { v[6].name,  v[6].edit,  bbdE (1.0f, true, -70) };
    v[7]  = { v[7].name,  v[7].edit,  bbdE (0.5f, true, -74) };
    v[9]  = { v[9].name,  v[9].edit,  bbdE (1.5f, false, -66) };
    v[18] = { v[18].name, v[18].edit, bbdE (1.0f, true, -72) };
    v[19] = { v[19].name, v[19].edit, bbdE (1.5f, true, -68) };
    v[15] = { v[15].name, v[15].edit, digiE (0, 0) };
    v[16] = { v[16].name, v[16].edit, digiE (12, 16000) };
    v[24] = { v[24].name, v[24].edit, digiE (0, 0) };
    v[25] = { v[25].name, v[25].edit, digiE (10, 24000) };
    v[26] = { v[26].name, v[26].edit, digiE (8, 12000) };
    v[28] = { v[28].name, v[28].edit, digiE (0, 0) };
    v[29] = { v[29].name, v[29].edit, digiE (8, 8000) };
    v[10] = { v[10].name, v[10].edit, radioE (300, 3400, 3.0f, 0.10f, -60, 0.0f, 0.0f) };
    v[11] = { v[11].name, v[11].edit, radioE (200, 4500, 2.0f, 0.20f, -56, 0.3f, 0.15f) };
    v[12] = { v[12].name, v[12].edit, radioE (50, 15000, 1.2f, 0.05f, -66, 0.1f, 0.1f) };
    v[13] = { v[13].name, v[13].edit, radioE (350, 2800, 2.5f, 0.25f, -48, 0.6f, 0.4f) };
    v[14] = { v[14].name, v[14].edit, radioE (250, 5000, 4.0f, 0.15f, -54, 0.2f, 0.3f) };
    v[8]  = { v[8].name,  v[8].edit,  springE (0.8f, 1.0f, 0.25f, 3.5f) };
    v[17] = { v[17].name, v[17].edit, springE (0.3f, 0.6f, 0.10f, 1.2f) };
    v[20] = { v[20].name, v[20].edit, springE (0.2f, 0.5f, 0.05f, 5.5f) };
    return v;
    }();
    return list;
}
} // namespace ptfx
