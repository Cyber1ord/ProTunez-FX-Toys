// ProTunez FX Toys — "Plate" (like SuperPlate): five plate characters, three preamps, decay to infinity.
// Input path: bandwidth, dispersion (lows arrive after highs, as in a real steel plate) and a transducer
// resonance feeding four diffusers. The tank is a 16-line feedback delay network with a Hadamard matrix and
// a low/mid/high decay filter on every line (Jot's condition: each band loses 60 dB in its own decay time).
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
        float lowX, highX; // low/high band decay time as a multiple of the Decay setting
        float dispHz;    // dispersion break frequency (lows arrive later than highs)
        float driverHz, driverDb; // transducer resonance
    };

    static const ModelDef& model (int i)
    {
        static const ModelDef m[numModels] = {
            { "Large Steel",       1.00f, 6500.0f,  9000.0f, 0.75f,  0.0f, 1.15f, 0.55f, 700.0f, 180.0f, 1.5f },
            { "Compact Gold Foil", 0.62f, 11000.0f, 15000.0f, 0.70f, 2.0f, 0.90f, 0.55f, 1000.0f, 450.0f, 1.0f },
            { "Studio Plate A",    0.85f, 5200.0f,  8000.0f, 0.78f, -1.0f, 1.10f, 0.50f, 800.0f, 220.0f, 1.5f },
            { "Studio Plate B",    1.10f, 7500.0f,  11000.0f, 0.82f, 1.0f, 1.20f, 0.60f, 600.0f, 160.0f, 2.0f },
            { "Studio Plate C",    0.74f, 4200.0f,  7000.0f, 0.72f, -2.0f, 1.05f, 0.45f, 900.0f, 250.0f, 1.0f },
        };
        return m[clampv (i, 0, numModels - 1)];
    }

    static constexpr int kLines = 16;
    static constexpr int kDispSections = 12;
    static constexpr float kLineMs[kLines] = { 21.3f, 23.9f, 26.9f, 29.6f, 32.4f, 35.5f, 38.2f, 41.1f, 44.6f, 47.3f, 50.9f, 54.1f, 57.8f, 61.6f, 65.3f, 69.7f };

    struct BandGains { float low, mid, high; };

    /** Per-pass gains of one line so that every band loses 60 dB in decaySec * its multiplier.
        invRt = 1 / mid-band RT60 (0 = endless); lineSec = the line's delay in seconds. */
    static BandGains bandGains (int model, float invRt, float lineSec)
    {
        const auto& m = Plate::model (model);
        const float e = -3.0f * lineSec * invRt;
        return { std::pow (10.0f, e / m.lowX), std::pow (10.0f, e), std::pow (10.0f, e / m.highX) };
    }

    /** Chain of identical first-order all-passes: the lows are delayed more than the highs. */
    class Dispersion
    {
    public:
        static constexpr int kMax = 16;

        /** Sets the coefficient only; never clears state (safe to call every block). */
        void prepare (double sampleRate, float breakHz, int numSections)
        {
            a = -(float) std::exp (-2.0 * (double) kPi * (double) breakHz / sampleRate);
            sections = clampv (numSections, 0, kMax);
        }
        void reset() { for (auto& v : z) v = 0.0f; }
        float process (float x)
        {
            for (int i = 0; i < sections; ++i)
            {
                const float y = a * x + z[i];
                z[i] = undenorm (x - a * y);
                x = y;
            }
            return x;
        }

    private:
        float a = 0.0f, z[kMax] {};
        int sections = 0;
    };

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
        invRt.setTarget (endless ? 0.0f : 1.0f / clampv (p.decaySec, 0.3f, kInfinite));
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
        for (int i = 0; i < 4; ++i) inAP[i].allocate ((int) ((float) kInAP[i] * maxScale) + 64);
        for (int k = 0; k < kLines; ++k)
            line[k].allocate ((int) (kLineMs[k] * 0.001 * sr * 1.25 + 16.0 * sr / 29761.0 + 64.0));
        pre.allocate ((int) (0.26 * sr) + 8);
        for (auto* s : { &invRt, &mix, &width, &preDelay, &inGain, &outGain, &duckGain, &sizeSm }) s->prepare (sr, 50.0f);
        setParams (params);
        for (auto* s : { &invRt, &mix, &width, &preDelay, &inGain, &outGain }) s->snap (s->getTarget());
        duckGain.snap (1.0f);
        sizeSm.snap (sizeTarget);
        duckAtt = (float) std::exp (-1.0 / (0.005 * sr));
        duckRel = (float) std::exp (-1.0 / (0.3 * sr));
        reset();
    }

    void reset() override
    {
        for (auto& d : inAP) d.reset();
        for (int k = 0; k < kLines; ++k) { line[k].reset(); zl[k] = zh[k] = 0.0f; }
        disp.reset(); dcIn.reset(); driver.reset();
        gainCounter = 0;
        pre.reset();
        bandZ = 0.0f; lfo = 0.0f; duckEnv = 0.0f;
        for (auto* f : { &lo1, &lo2, &hiIn, &tilt[0], &tilt[1], &eq1[0], &eq1[1], &eq2[0], &eq2[1] }) f->reset();
        lo0.reset();
    }

    void process (float* left, float* right, int n) override
    {
        const auto& m = model (params.model);
        const float bandCoef = 1.0f - std::exp (-2.0f * kPi * m.bandHz / (float) sr);
        const float aLo = std::exp (-2.0f * kPi * 150.0f / (float) sr);
        const float aHi = std::exp (-2.0f * kPi * m.dampHz / (float) sr);
        const float modDepth = clampv (params.modDepth, 0.0f, 1.0f) * (float) kExcursion * scale;
        const float lfoInc = clampv (params.modRateHz, 0.1f, 5.0f) / (float) sr;
        sizeSm.setTarget (sizeTarget);

        for (int i = 0; i < n; ++i)
        {
            invRt.next();
            const float mx = mix.next(), w = width.next(), ig = inGain.next(), og = outGain.next();
            const float szModel = sizeSm.next();
            const float sz = szModel * scale;
            const float inL = left[i], inR = right[i];

            if (gainCounter == 0)
            {
                if (invRt.getTarget() == 0.0f && invRt.get() < 1.0e-5f) invRt.snap (0.0f);
                for (int k = 0; k < kLines; ++k)
                {
                    const auto g = bandGains (params.model, invRt.get(), kLineMs[k] * 0.001f * szModel);
                    gM[k] = g.mid; rL[k] = g.low / g.mid - 1.0f; rH[k] = g.high / g.mid - 1.0f;
                }
            }
            gainCounter = (gainCounter + 1) & 31;

            // Ducking: follow the input and turn the reverb down while it is loud.
            const float lvl = std::max (std::fabs (inL), std::fabs (inR));
            duckEnv = lvl > duckEnv ? lvl + duckAtt * (duckEnv - lvl) : lvl + duckRel * (duckEnv - lvl);
            const float over = clampv ((gainToDb (duckEnv) + 40.0f) / 10.0f, 0.0f, 1.0f);
            duckGain.setTarget (1.0f - clampv (params.duck, 0.0f, 1.0f) * over * (1.0f - dbToGain (-18.0f)));
            const float dg = duckGain.next();

            // Input: preamp colour, DC removal, filters, pre-delay, bandwidth, dispersion, transducer.
            float x = params.freeze ? 0.0f : preamp (0.5f * (inL + inR) * ig);
            x = dcIn.process (x);
            x = lowCut (x);
            if (params.highHz < 19999.0f) x = hiIn.process (x);
            pre.push (x);
            x = pre.readFrac (std::max (1.0f, preDelay.next()));
            bandZ = undenorm (bandZ + bandCoef * (x - bandZ));
            x = disp.process (bandZ);
            x = driver.process (x);

            const float d1 = m.inDiff, d2 = m.inDiff * 0.833f;
            x = allpass (inAP[0], x, (float) kInAP[0] * sz, d1);
            x = allpass (inAP[1], x, (float) kInAP[1] * sz, d1);
            x = allpass (inAP[2], x, (float) kInAP[2] * sz, d2);
            x = allpass (inAP[3], x, (float) kInAP[3] * sz, d2);

            lfo += lfoInc; if (lfo >= 1.0f) lfo -= 1.0f;

            float o[kLines], v[kLines];
            for (int k = 0; k < kLines; ++k)
            {
                float len = kLineMs[k] * 0.001f * (float) sr * szModel;
                if ((k & 3) == 0)
                {
                    float ph = lfo + 0.25f * (float) (k >> 2);
                    ph -= std::floor (ph);
                    len += modDepth * std::sin (2.0f * kPi * ph);
                }
                o[k] = line[k].readFrac (len);
                zl[k] = undenorm (zl[k] + (1.0f - aLo) * (o[k] - zl[k]));
                float y = o[k] + rL[k] * zl[k];
                zh[k] = undenorm (zh[k] + (1.0f - aHi) * (y - zh[k]));
                y = y + rH[k] * (y - zh[k]);
                v[k] = gM[k] * y;
            }
            for (int h = 1; h < kLines; h <<= 1)
                for (int a = 0; a < kLines; a += h << 1)
                    for (int b = a; b < a + h; ++b)
                    {
                        const float p = v[b], q = v[b + h];
                        v[b] = p + q; v[b + h] = p - q;
                    }
            float yL = 0.0f, yR = 0.0f;
            for (int k = 0; k < kLines; ++k)
            {
                line[k].push (safety (undenorm (0.25f * v[k] + 0.25f * (float) kIn[k] * x)));
                yL += (float) kOL[k] * o[k];
                yR += (float) kOR[k] * o[k];
            }
            yL = ceiling (yL * 0.68f); yR = ceiling (yR * 0.68f);

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
    static constexpr int kInAP[4] = { 142, 107, 379, 277 };
    static constexpr int kExcursion = 16;
    static constexpr int kIn[kLines] = { 1, -1, 1, 1, -1, 1, -1, -1, 1, 1, -1, 1, -1, 1, 1, -1 };
    static constexpr int kOL[kLines] = { 1, 1, -1, 1, -1, -1, 1, 1, 1, -1, 1, -1, -1, 1, -1, 1 };
    static constexpr int kOR[kLines] = { 1, -1, 1, 1, 1, -1, -1, 1, -1, 1, 1, 1, -1, -1, 1, -1 };

    Params params;
    double sr = 44100.0;
    float scale = 1.0f, sizeTarget = 1.0f;
    DelayLine inAP[4], line[kLines], pre;
    Smoothed invRt, mix, width, preDelay, inGain, outGain, duckGain, sizeSm;
    float zl[kLines] {}, zh[kLines] {}, gM[kLines] {}, rL[kLines] {}, rH[kLines] {};
    int gainCounter = 0;
    float bandZ = 0.0f, lfo = 0.0f, duckEnv = 0.0f, duckAtt = 0.9f, duckRel = 0.999f;
    Dispersion disp;
    OnePoleHP dcIn;
    Biquad driver;
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

    void updateFilters()
    {
        const float lo = clampv (params.lowHz, 20.0f, 1000.0f), hi = clampv (params.highHz, 1000.0f, 20000.0f);
        lo0.setCutoff (lo, sr);
        lo1.set (Biquad::Type::highPass, lo, 0.707f, 0.0f, sr);
        lo2.set (Biquad::Type::highPass, lo, 0.707f, 0.0f, sr);
        hiIn.set (Biquad::Type::lowPass, hi, 0.707f, 0.0f, sr);
        const auto& m = model (params.model);
        dcIn.setCutoff (10.0f, sr);
        disp.prepare (sr, m.dispHz, kDispSections);
        driver.set (Biquad::Type::peak, m.driverHz, 1.2f, m.driverDb, sr);
        for (int c = 0; c < 2; ++c)
        {
            tilt[c].set (Biquad::Type::highShelf, 2500.0f, 0.6f, m.tiltDb, sr);
            eq1[c].set (Biquad::Type::peak, clampv (params.eq1F, 40.0f, 18000.0f), clampv (params.eq1Q, 0.2f, 8.0f), clampv (params.eq1G, -18.0f, 18.0f), sr);
            eq2[c].set (Biquad::Type::peak, clampv (params.eq2F, 40.0f, 18000.0f), clampv (params.eq2Q, 0.2f, 8.0f), clampv (params.eq2G, -18.0f, 18.0f), sr);
        }
    }
};
} // namespace ptfx
