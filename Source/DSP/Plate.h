// ProTunez FX Toys — "Plate": a lush plate reverb.
// Uses the well-known figure-of-eight plate topology published by
// Jon Dattorro ("Effect Design Part 1", JAES 1997).
#pragma once

#include "Common.h"

namespace ptfx
{
class Plate : public Effect
{
public:
    struct Params
    {
        float preDelayMs = 20.0f;  // 0..250
        float decay      = 0.6f;   // 0..1 (short .. very long)
        float size       = 0.7f;   // 0..1
        float damping    = 0.4f;   // 0..1 (bright .. dark tail)
        float lowCutHz   = 150.0f; // on the reverb only
        float highCutHz  = 12000.0f;
        float modulation = 0.3f;   // 0..1 tail movement
        float width      = 1.0f;   // 0..1
        float mix        = 0.25f;  // 0..1
    };

    void setParams (const Params& p)
    {
        params = p;
        const float d = clampv (p.decay, 0.0f, 1.0f);
        decay.setTarget (0.2f + 0.78f * std::sqrt (d)); // tank feedback 0.2..0.98
        mix.setTarget (clampv (p.mix, 0.0f, 1.0f));
        width.setTarget (clampv (p.width, 0.0f, 1.0f));
        sizeSmooth.setTarget (0.5f + clampv (p.size, 0.0f, 1.0f) * 0.75f); // 0.5x..1.25x
        preDelay.setTarget (clampv (p.preDelayMs, 0.0f, 250.0f) * 0.001f * (float) sr);
        updateFilters();
    }

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        scale = (float) (sr / 29761.0);
        const float maxScale = scale * 1.3f;
        auto alloc = [&] (DelayLine& d, int base) { d.allocate ((int) (base * maxScale) + 64); };

        for (int i = 0; i < 4; ++i) alloc (inAP[i], kInAP[i]);
        for (int t = 0; t < 2; ++t)
        {
            alloc (modAP[t], kModAP[t] + kExcursion * 2);
            alloc (delay1[t], kDelay1[t]);
            alloc (ap2[t], kAP2[t]);
            alloc (delay2[t], kDelay2[t]);
        }
        pre.allocate ((int) (0.26 * sr) + 8);

        for (auto* s : { &decay, &mix, &width, &sizeSmooth, &preDelay }) s->prepare (sr, 50.0f);
        setParams (params);
        decay.snap (decay.getTarget()); mix.snap (mix.getTarget()); width.snap (width.getTarget());
        sizeSmooth.snap (sizeSmooth.getTarget()); preDelay.snap (preDelay.getTarget());
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
        bandZ = 0.0f;
        lfo = 0.0f;
        for (int c = 0; c < 2; ++c) { wetLow[c].reset(); wetHigh[c].reset(); }
    }

    void process (float* left, float* right, int n) override
    {
        const float damp = 0.05f + clampv (params.damping, 0.0f, 1.0f) * 0.75f;
        const float bandwidth = 0.9995f - clampv (params.damping, 0.0f, 1.0f) * 0.3f;
        const float modDepth = clampv (params.modulation, 0.0f, 1.0f) * (float) kExcursion * scale;
        const float lfoInc = 1.0f / (float) sr; // ~1 Hz

        for (int i = 0; i < n; ++i)
        {
            const float dec = decay.next(), m = mix.next(), w = width.next();
            const float sz = sizeSmooth.next() * scale;
            const float inL = left[i], inR = right[i];

            // Pre-delay, then soften the input.
            pre.push (0.5f * (inL + inR));
            float x = pre.readFrac (std::max (1.0f, preDelay.next()));
            bandZ = undenorm (bandZ + bandwidth * (x - bandZ));
            x = bandZ;

            // Four input diffusers smear the sound into a dense wash.
            x = allpass (inAP[0], x, kInAP[0] * sz, 0.75f);
            x = allpass (inAP[1], x, kInAP[1] * sz, 0.75f);
            x = allpass (inAP[2], x, kInAP[2] * sz, 0.625f);
            x = allpass (inAP[3], x, kInAP[3] * sz, 0.625f);

            lfo += lfoInc; if (lfo >= 1.0f) lfo -= 1.0f;
            const float modL = modDepth * std::sin (2.0f * kPi * lfo);
            const float modR = modDepth * std::cos (2.0f * kPi * lfo);

            // The two halves of the tank feed each other in a figure of eight.
            const float feedL = x + dec * tankOut[1];
            const float feedR = x + dec * tankOut[0];
            tankOut[0] = tank (0, feedL, sz, modL, damp, dec);
            tankOut[1] = tank (1, feedR, sz, modR, damp, dec);

            // Read the left and right outputs from several points in the tank.
            auto tap = [&] (const DelayLine& d, int t) { return d.readFrac (std::max (1.0f, (float) t * sz)); };
            float yL = tap (delay1[1], 266) + tap (delay1[1], 2974) - tap (ap2[1], 1913) + tap (delay2[1], 1996)
                     - tap (delay1[0], 1990) - tap (ap2[0], 187) - tap (delay2[0], 1066);
            float yR = tap (delay1[0], 353) + tap (delay1[0], 3627) - tap (ap2[0], 1228) + tap (delay2[0], 2673)
                     - tap (delay1[1], 2111) - tap (ap2[1], 335) - tap (delay2[1], 121);
            yL *= 0.6f; yR *= 0.6f;

            yL = wetHigh[0].process (wetLow[0].process (yL));
            yR = wetHigh[1].process (wetLow[1].process (yR));

            const float mid = 0.5f * (yL + yR), side = 0.5f * (yL - yR) * w;
            left[i]  = inL * (1.0f - m) + (mid + side) * m;
            right[i] = inR * (1.0f - m) + (mid - side) * m;
        }
    }

private:
    // Delay lengths from the published design (at 29761 Hz).
    static constexpr int kInAP[4]   = { 142, 107, 379, 277 };
    static constexpr int kModAP[2]  = { 672, 908 };
    static constexpr int kDelay1[2] = { 4453, 4217 };
    static constexpr int kAP2[2]    = { 1800, 2656 };
    static constexpr int kDelay2[2] = { 3720, 3163 };
    static constexpr int kExcursion = 16;

    Params params;
    double sr = 44100.0;
    float scale = 1.0f;
    DelayLine inAP[4], modAP[2], delay1[2], ap2[2], delay2[2], pre;
    Smoothed decay, mix, width, sizeSmooth, preDelay;
    float dampZ[2] {}, tankOut[2] {}, bandZ = 0.0f, lfo = 0.0f;
    OnePoleHP wetLow[2];
    Biquad wetHigh[2];

    /** Schroeder all-pass: passes all frequencies but scrambles their timing. */
    static float allpass (DelayLine& d, float x, float delay, float g)
    {
        const float delayed = d.readFrac (std::max (1.0f, delay));
        const float v = x + g * delayed;
        d.push (undenorm (v));
        return delayed - g * v;
    }

    float tank (int t, float x, float sz, float mod, float damp, float dec)
    {
        // Modulated all-pass (note the negative coefficient in the original design).
        float y = allpass (modAP[t], x, kModAP[t] * sz + mod, -0.7f);
        delay1[t].push (y);
        y = delay1[t].readFrac (std::max (1.0f, kDelay1[t] * sz));
        dampZ[t] = undenorm (dampZ[t] + (1.0f - damp) * (y - dampZ[t]));
        y = dampZ[t] * dec;
        y = allpass (ap2[t], y, kAP2[t] * sz, 0.5f);
        delay2[t].push (y);
        return delay2[t].readFrac (std::max (1.0f, kDelay2[t] * sz));
    }

    void updateFilters()
    {
        for (int c = 0; c < 2; ++c)
        {
            wetLow[c].setCutoff (clampv (params.lowCutHz, 20.0f, 1000.0f), sr);
            wetHigh[c].set (Biquad::Type::lowPass, clampv (params.highCutHz, 1000.0f, 20000.0f), 0.707f, 0.0f, sr);
        }
    }
};
} // namespace ptfx
