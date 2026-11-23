// ProTunez FX Toys — "Squeeze": a compressor, from clean levelling to all-out smash.
#pragma once

#include "Common.h"
#include <atomic>

namespace ptfx
{
class Squeeze : public Effect
{
public:
    enum Character { clean = 0, vintage, smash, numCharacters };

    struct Params
    {
        float thresholdDb = -18.0f; // -60..0
        float ratio       = 3.0f;   // 1..20
        float attackMs    = 10.0f;  // 0.1..100
        float releaseMs   = 120.0f; // 10..1000
        float kneeDb      = 6.0f;   // 0..12
        float makeupDb    = 0.0f;   // 0..24
        float sidechainHpHz = 80.0f; // 20..500: ignore bass when deciding how much to squeeze
        int   character   = clean;
        float mix         = 1.0f;   // 0..1 (parallel compression)
    };

    void setParams (const Params& p)
    {
        params = p;
        makeup.setTarget (dbToGain (clampv (p.makeupDb, 0.0f, 24.0f)));
        mix.setTarget (clampv (p.mix, 0.0f, 1.0f));
        updateTimes();
        for (auto& f : scHp) f.setCutoff (clampv (p.sidechainHpHz, 20.0f, 500.0f), sr);
    }

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        makeup.prepare (sr, 20.0f); mix.prepare (sr, 20.0f);
        makeup.snap (dbToGain (params.makeupDb)); mix.snap (params.mix);
        setParams (params);
        reset();
    }

    void reset() override
    {
        envDb = 0.0f; grDb = 0.0f; lastGain = 1.0f;
        for (auto& f : scHp) f.reset();
        gainReductionDb.store (0.0f);
    }

    void process (float* left, float* right, int n) override
    {
        const float thr = clampv (params.thresholdDb, -60.0f, 0.0f);
        const float ratio = clampv (params.ratio, 1.0f, 20.0f) * (params.character == smash ? 2.0f : 1.0f);
        const float knee = clampv (params.kneeDb, 0.0f, 12.0f);
        float maxGr = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            const float mk = makeup.next(), m = mix.next();
            const float inL = left[i], inR = right[i];

            // "Vintage" listens to its own output (feedback style), the others to the input.
            float detL = params.character == vintage ? inL * lastGain : inL;
            float detR = params.character == vintage ? inR * lastGain : inR;
            detL = scHp[0].process (detL);
            detR = scHp[1].process (detR);
            const float level = std::max (std::fabs (detL), std::fabs (detR)); // stereo linked
            const float levelDb = gainToDb (level);

            // How far over the threshold we are, with a soft knee.
            float over = levelDb - thr;
            float targetGr = 0.0f;
            if (knee > 0.0f && std::fabs (over) <= knee * 0.5f)
                targetGr = (1.0f / ratio - 1.0f) * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
            else if (over > 0.0f)
                targetGr = (1.0f / ratio - 1.0f) * over;

            // Attack when squeezing harder, release when letting go.
            const float coeff = targetGr < grDb ? attackCoeff : releaseCoeff;
            grDb = targetGr + coeff * (grDb - targetGr);
            const float gain = dbToGain (grDb);
            lastGain = gain;
            maxGr = std::min (maxGr, grDb);

            float oL = inL * gain * mk, oR = inR * gain * mk;
            if (params.character == vintage)
            {
                oL = std::tanh (oL * 1.2f) / 1.2f;
                oR = std::tanh (oR * 1.2f) / 1.2f;
            }
            else if (params.character == smash)
            {
                oL = std::tanh (oL * 2.0f) / 1.6f;
                oR = std::tanh (oR * 2.0f) / 1.6f;
            }

            left[i]  = inL * (1.0f - m) + oL * m;
            right[i] = inR * (1.0f - m) + oR * m;
        }
        gainReductionDb.store (maxGr);
    }

    /** Latest gain reduction in dB (0 or negative), for the meter. */
    std::atomic<float> gainReductionDb { 0.0f };

private:
    Params params;
    double sr = 44100.0;
    Smoothed makeup, mix;
    OnePoleHP scHp[2];
    float envDb = 0.0f, grDb = 0.0f, lastGain = 1.0f;
    float attackCoeff = 0.0f, releaseCoeff = 0.0f;

    void updateTimes()
    {
        float att = clampv (params.attackMs, 0.1f, 100.0f), rel = clampv (params.releaseMs, 10.0f, 1000.0f);
        if (params.character == smash) { att *= 0.3f; rel *= 0.5f; }
        attackCoeff  = (float) std::exp (-1.0 / (0.001 * att * sr));
        releaseCoeff = (float) std::exp (-1.0 / (0.001 * rel * sr));
    }
};
} // namespace ptfx
