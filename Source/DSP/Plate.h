// ProTunez FX Toys — "Plate" (like SuperPlate): five plate characters, three preamps, decay to infinity.
// The tank is the well-known figure-of-eight plate topology published by Jon Dattorro
// ("Effect Design Part 1", JAES 1997); each model changes its size, damping and colour.
#pragma once

#include "Common.h"

namespace ptfx
{
class Plate : public Effect
{
public:
    enum Preamp { tube = 0, solid, clean, numPreamps };
    static constexpr int numModels = 5;
    static constexpr float kInfinite = 60.0f; // decay at or above this = endless

    struct ModelDef
    {
        const char* name;
        float size;      // scales every delay in the tank
        float dampHz;    // how fast highs die away in the tail
        float bandHz;    // brightness of the sound going in
        float inDiff;    // input diffusion
        float tiltDb;    // overall colour of the reverb
    };

    static const ModelDef& model (int i)
    {
        static const ModelDef m[numModels] = {
            { "Large Steel",       1.00f, 6500.0f,  9000.0f, 0.75f,  0.0f },
            { "Compact Gold Foil", 0.62f, 11000.0f, 15000.0f, 0.70f, 2.0f },
            { "Studio Plate A",    0.85f, 5200.0f,  8000.0f, 0.78f, -1.0f },
            { "Studio Plate B",    1.10f, 7500.0f,  11000.0f, 0.82f, 1.0f },
            { "Studio Plate C",    0.74f, 4200.0f,  7000.0f, 0.72f, -2.0f },
        };
        return m[clampv (i, 0, numModels - 1)];
    }

    struct Params
    {
        int   model = 0;
        int   preamp = tube;
        float decaySec = 2.0f;    // 0.5 .. 60 (60 = infinite)
        float preDelayMs = 0.0f;  // 0..250
        float modDepth = 0.3f;    // 0..1
        float modRateHz = 0.8f;   // 0.1..5
        float lowHz = 20.0f;      // low cut on what goes into the plate
        float highHz = 20000.0f;  // high cut on what goes into the plate
        int   lowSlope = 1;       // 0 = 6, 1 = 12, 2 = 24 dB/oct
        float eq1F = 400.0f, eq1G = 0.0f, eq1Q = 1.0f;   // output EQ band 1
        float eq2F = 4000.0f, eq2G = 0.0f, eq2Q = 1.0f;  // output EQ band 2
        float width = 1.0f;       // 0 mono .. 1 normal .. 2 extra wide
        float balance = 0.0f;     // -1..1
        float inDb = 0.0f, outDb = 0.0f;
        float mix = 1.0f;
        float duck = 0.0f;        // 0..1
        bool  freeze = false;
    };

    void setParams (const Params& p)
    {
        params = p;
        const auto& m = model (p.model);
        sizeTarget = m.size;
        const bool endless = p.freeze || p.decaySec >= kInfinite - 0.01f;
        // Each pass through half the tank multiplies by decay twice (after the damping and on the way across),
        // so choose it so the tail falls 60 dB in decaySec: decay^2 per half loop.
        const float halfLoopSec = kHalfLoopSec * m.size;
        const float g = endless ? 1.0f : std::pow (10.0f, -1.5f * halfLoopSec / clampv (p.decaySec, 0.3f, kInfinite));
        decay.setTarget (g);
        mix.setTarget (clampv (p.mix, 0.0f, 1.0f));
        inGain.setTarget (dbToGain (clampv (p.inDb, -24.0f, 24.0f)));
        outGain.setTarget (dbToGain (clampv (p.outDb, -24.0f, 24.0f)));
        width.setTarget (clampv (p.width, 0.0f, 2.0f));
        preDelay.setTarget (clampv (p.preDelayMs, 0.0f, 250.0f) * 0.001f * (float) sr);
        updateFilters();
    }

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        scale = (float) (sr / 29761.0);
        const float maxScale = scale * 1.25f;
        auto alloc = [&] (DelayLine& d, int base) { d.allocate ((int) ((float) base * maxScale) + 64); };
        for (int i = 0; i < 4; ++i) alloc (inAP[i], kInAP[i]);
        for (int t = 0; t < 2; ++t)
        {
            alloc (modAP[t], kModAP[t] + kExcursion * 2);
            alloc (delay1[t], kDelay1[t]);
            alloc (ap2[t], kAP2[t]);
            alloc (delay2[t], kDelay2[t]);
        }
        pre.allocate ((int) (0.26 * sr) + 8);
        for (auto* s : { &decay, &mix, &width, &preDelay, &inGain, &outGain, &duckGain, &sizeSm }) s->prepare (sr, 50.0f);
        setParams (params);
        for (auto* s : { &decay, &mix, &width, &preDelay, &inGain, &outGain }) s->snap (s->getTarget());
        duckGain.snap (1.0f);
        sizeSm.snap (sizeTarget);
        duckAtt = (float) std::exp (-1.0 / (0.005 * sr));
        duckRel = (float) std::exp (-1.0 / (0.3 * sr));
        reset();
    }

    void reset() override
    {
        for (auto& d : inAP) d.reset();
        for (int t = 0; t < 2; ++t)
        {
            modAP[t].reset(); delay1[t].reset(); ap2[t].reset(); delay2[t].reset();
            dampZ[t] = 0.0f; tankOut[t] = 0.0f;
        }
        pre.reset();
        bandZ = 0.0f; lfo = 0.0f; duckEnv = 0.0f;
        for (auto* f : { &lo1, &lo2, &hiIn, &tilt[0], &tilt[1], &eq1[0], &eq1[1], &eq2[0], &eq2[1] }) f->reset();
        lo0.reset();
    }

    void process (float* left, float* right, int n) override
    {
        const auto& m = model (params.model);
        const bool endless = params.freeze || params.decaySec >= kInfinite - 0.01f;
        const float bandCoef = 1.0f - std::exp (-2.0f * kPi * m.bandHz / (float) sr);
        const float dampCoef = endless ? 0.0f : std::exp (-2.0f * kPi * m.dampHz / (float) sr); // 0 = no damping
        const float modDepth = clampv (params.modDepth, 0.0f, 1.0f) * (float) kExcursion * scale;
        const float lfoInc = clampv (params.modRateHz, 0.1f, 5.0f) / (float) sr;
        sizeSm.setTarget (sizeTarget);

        for (int i = 0; i < n; ++i)
        {
            const float dec = decay.next(), mx = mix.next(), w = width.next(), ig = inGain.next(), og = outGain.next();
            const float sz = sizeSm.next() * scale;
            const float inL = left[i], inR = right[i];

            // Ducking: follow the input and turn the reverb down while it is loud.
            const float lvl = std::max (std::fabs (inL), std::fabs (inR));
            duckEnv = lvl > duckEnv ? lvl + duckAtt * (duckEnv - lvl) : lvl + duckRel * (duckEnv - lvl);
            const float over = clampv ((gainToDb (duckEnv) + 40.0f) / 10.0f, 0.0f, 1.0f);
            duckGain.setTarget (1.0f - clampv (params.duck, 0.0f, 1.0f) * over * (1.0f - dbToGain (-18.0f)));
            const float dg = duckGain.next();

            // Input: preamp colour, filters, pre-delay, then bandwidth.
            float x = params.freeze ? 0.0f : preamp (0.5f * (inL + inR) * ig);
            x = lowCut (x);
            if (params.highHz < 19999.0f) x = hiIn.process (x);
            pre.push (x);
            x = pre.readFrac (std::max (1.0f, preDelay.next()));
            bandZ = undenorm (bandZ + bandCoef * (x - bandZ));
            x = bandZ;

            const float d1 = m.inDiff, d2 = m.inDiff * 0.833f;
            x = allpass (inAP[0], x, (float) kInAP[0] * sz, d1);
            x = allpass (inAP[1], x, (float) kInAP[1] * sz, d1);
            x = allpass (inAP[2], x, (float) kInAP[2] * sz, d2);
            x = allpass (inAP[3], x, (float) kInAP[3] * sz, d2);

            lfo += lfoInc; if (lfo >= 1.0f) lfo -= 1.0f;
            const float modL = modDepth * std::sin (2.0f * kPi * lfo);
            const float modR = modDepth * std::cos (2.0f * kPi * lfo);

            const float feedL = x + dec * tankOut[1];
            const float feedR = x + dec * tankOut[0];
            tankOut[0] = tank (0, feedL, sz, modL, dampCoef, dec);
            tankOut[1] = tank (1, feedR, sz, modR, dampCoef, dec);

            auto tap = [&] (const DelayLine& d, int t) { return d.readFrac (std::max (1.0f, (float) t * sz)); };
            float yL = tap (delay1[1], 266) + tap (delay1[1], 2974) - tap (ap2[1], 1913) + tap (delay2[1], 1996)
                     - tap (delay1[0], 1990) - tap (ap2[0], 187) - tap (delay2[0], 1066);
            float yR = tap (delay1[0], 353) + tap (delay1[0], 3627) - tap (ap2[0], 1228) + tap (delay2[0], 2673)
                     - tap (delay1[1], 2111) - tap (ap2[1], 335) - tap (delay2[1], 121);
            yL = ceiling (yL * 0.6f); yR = ceiling (yR * 0.6f);

            yL = eq2[0].process (eq1[0].process (tilt[0].process (yL)));
            yR = eq2[1].process (eq1[1].process (tilt[1].process (yR)));

            const float mid = 0.5f * (yL + yR), side = 0.5f * (yL - yR) * w;
            yL = mid + side; yR = mid - side;
            const float bal = clampv (params.balance, -1.0f, 1.0f);
            const float wet = og * dg;
            left[i]  = inL * (1.0f - mx) + yL * std::min (1.0f, 1.0f - bal) * wet * mx;
            right[i] = inR * (1.0f - mx) + yR * std::min (1.0f, 1.0f + bal) * wet * mx;
        }
    }

    /** Preamp characters. Clean is a straight wire. */
    float preamp (float x) const
    {
        switch (params.preamp)
        {
            case tube:  { const float v = x * 1.4f; return (v >= 0 ? std::tanh (v) : std::tanh (0.75f * v) / 0.75f) / 1.4f; }
            case solid: { const float v = x * 1.2f; return std::tanh (v) / 1.2f; }
            case clean:
            default:    return x;
        }
    }

private:
    static constexpr int kInAP[4]   = { 142, 107, 379, 277 };
    static constexpr int kModAP[2]  = { 672, 908 };
    static constexpr int kDelay1[2] = { 4453, 4217 };
    static constexpr int kAP2[2]    = { 1800, 2656 };
    static constexpr int kDelay2[2] = { 3720, 3163 };
    static constexpr int kExcursion = 16;
    static constexpr float kHalfLoopSec = 0.3627f; // average half-tank loop at size 1

    Params params;
    double sr = 44100.0;
    float scale = 1.0f, sizeTarget = 1.0f;
    DelayLine inAP[4], modAP[2], delay1[2], ap2[2], delay2[2], pre;
    Smoothed decay, mix, width, preDelay, inGain, outGain, duckGain, sizeSm;
    float dampZ[2] {}, tankOut[2] {}, bandZ = 0.0f, lfo = 0.0f, duckEnv = 0.0f, duckAtt = 0.9f, duckRel = 0.999f;
    OnePoleHP lo0;
    Biquad lo1, lo2, hiIn, tilt[2], eq1[2], eq2[2];

    float lowCut (float x)
    {
        if (params.lowHz <= 21.0f) return x;
        switch (params.lowSlope)
        {
            case 0:  return lo0.process (x);
            case 2:  return lo2.process (lo1.process (x));
            default: return lo1.process (x);
        }
    }

    /** Schroeder all-pass: passes all frequencies but scrambles their timing. */
    static float allpass (DelayLine& d, float x, float delay, float g)
    {
        const float delayed = d.readFrac (std::max (1.0f, delay));
        const float v = x + g * delayed;
        d.push (undenorm (v));
        return delayed - g * v;
    }

    /** Never lets the tank run away (only matters for endless decay with loud input). */
    static float safety (float x)
    {
        const float a = std::fabs (x);
        if (a <= 0.5f) return x;
        return std::copysign (0.5f + 0.5f * std::tanh ((a - 0.5f) / 0.5f), x);
    }

    /** Output ceiling: transparent below 2.0, never above 2.5 (protects ears with endless decay and hot input). */
    static float ceiling (float x)
    {
        const float a = std::fabs (x);
        if (a <= 2.0f) return x;
        return std::copysign (2.0f + 0.5f * std::tanh ((a - 2.0f) / 0.5f), x);
    }

    float tank (int t, float x, float sz, float mod, float dampCoef, float dec)
    {
        float y = allpass (modAP[t], x, (float) kModAP[t] * sz + mod, -0.7f);
        delay1[t].push (y);
        y = delay1[t].readFrac (std::max (1.0f, (float) kDelay1[t] * sz));
        dampZ[t] = undenorm (y + dampCoef * (dampZ[t] - y));
        y = dampZ[t] * dec;
        y = allpass (ap2[t], y, (float) kAP2[t] * sz, 0.5f);
        delay2[t].push (safety (y));
        return delay2[t].readFrac (std::max (1.0f, (float) kDelay2[t] * sz));
    }

    void updateFilters()
    {
        const float lo = clampv (params.lowHz, 20.0f, 1000.0f), hi = clampv (params.highHz, 1000.0f, 20000.0f);
        lo0.setCutoff (lo, sr);
        lo1.set (Biquad::Type::highPass, lo, 0.707f, 0.0f, sr);
        lo2.set (Biquad::Type::highPass, lo, 0.707f, 0.0f, sr);
        hiIn.set (Biquad::Type::lowPass, hi, 0.707f, 0.0f, sr);
        const auto& m = model (params.model);
        for (int c = 0; c < 2; ++c)
        {
            tilt[c].set (Biquad::Type::highShelf, 2500.0f, 0.6f, m.tiltDb, sr);
            eq1[c].set (Biquad::Type::peak, clampv (params.eq1F, 40.0f, 18000.0f), clampv (params.eq1Q, 0.2f, 8.0f), clampv (params.eq1G, -18.0f, 18.0f), sr);
            eq2[c].set (Biquad::Type::peak, clampv (params.eq2F, 40.0f, 18000.0f), clampv (params.eq2Q, 0.2f, 8.0f), clampv (params.eq2G, -18.0f, 18.0f), sr);
        }
    }
};
} // namespace ptfx
