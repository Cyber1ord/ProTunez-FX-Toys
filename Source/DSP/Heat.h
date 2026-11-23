// ProTunez FX Toys — "Heat" (like Decapitator): analog-style saturation from warmth to destruction.
#pragma once

#include "Analog.h"

namespace ptfx
{
class Heat : public Effect
{
public:
    /** Five saturation characters: tape machine, British console, classic preamp, triode tube, pentode tube. */
    enum Style { styleA = 0, styleE, styleN, styleT, styleP, numStyles };

    struct Params
    {
        int   style     = styleN;
        float drive     = 3.0f;     // 0..10  (0..+36 dB into the circuit)
        bool  punish    = false;    // +20 dB more
        bool  autoGain  = true;
        float outputDb  = 0.0f;     // -24..+24
        float mix       = 1.0f;     // 0..1
        float lowCutHz  = 20.0f;    // 20..1000 before the saturation (20 = off)
        bool  thump     = false;    // low boost at the Low Cut frequency
        float tone      = 0.0f;     // -1 dark .. +1 bright
        float highCutHz = 20000.0f; // 1000..20000 after the saturation (>= 19999 = off)
        bool  steep     = false;    // High Cut 30 dB/oct instead of 6 dB/oct
    };

    void setParams (const Params& p)
    {
        params = p;
        const float driveDb = clampv (p.drive, 0.0f, 10.0f) * 3.6f + (p.punish ? 20.0f : 0.0f);
        drive.setTarget (dbToGain (driveDb));
        mix.setTarget (clampv (p.mix, 0.0f, 1.0f));
        output.setTarget (dbToGain (clampv (p.outputDb, -24.0f, 24.0f)));
        updateFilters();
    }

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        for (auto* s : { &drive, &mix, &output, &autoComp }) s->prepare (sr, 30.0f);
        setParams (params);
        drive.snap (drive.getTarget());
        mix.snap (mix.getTarget());
        output.snap (output.getTarget());
        autoComp.snap (1.0f);
        envCoeff = (float) std::exp (-1.0 / (0.3 * sr));  // ~300 ms loudness tracking
        meterCoeff = (float) std::exp (-1.0 / (0.15 * sr));
        for (auto& d : dryDelay) d.allocate (Oversampler2x::latency() + 4);
        updateFilters();
        reset();
    }

    void reset() override
    {
        for (int c = 0; c < 2; ++c)
        {
            os[c].reset(); dc[c].reset(); lowCut[c].reset(); thumpEq[c].reset(); tilt[c].reset();
            hc1[c].reset(); hc2[c].reset(); hcPole[c].reset(); dryDelay[c].reset();
        }
        inEnv = outEnv = 0.0f;
        attitude = 0.0f;
    }

    int latency() const override { return Oversampler2x::latency(); }
    float meter() const override { return attitudeOut.load(); }

    void process (float* left, float* right, int n) override
    {
        float* ch[2] = { left, right };
        const bool lowCutOn = params.lowCutHz > 21.0f;
        const bool highCutOn = params.highCutHz < 19999.0f;

        for (int i = 0; i < n; ++i)
        {
            const float g = drive.next(), m = mix.next(), out = output.next();
            float wet[2], dry[2], inPow = 0.0f, outPow = 0.0f, hot = 0.0f;

            for (int c = 0; c < 2; ++c)
            {
                const float x = ch[c][i];
                dryDelay[c].push (x);
                dry[c] = dryDelay[c].read (Oversampler2x::latency() + 1);

                float pre = lowCutOn ? lowCut[c].process (x) : x;
                if (params.thump) pre = thumpEq[c].process (pre);
                pre *= g;
                hot = std::max (hot, std::fabs (pre));

                float a, b;
                os[c].upsample (pre, a, b);
                float y = os[c].downsample (shape (a), shape (b));
                y = dc[c].process (y);
                y = tilt[c].process (y);
                if (highCutOn)
                {
                    y = hcPole[c].process (y);
                    if (params.steep) y = hc2[c].process (hc1[c].process (y));
                }
                wet[c] = y;
                inPow += dry[c] * dry[c];
                outPow += y * y;
            }

            // Attitude: how hard the circuit is being pushed (smoothed peak, on a dB scale).
            attitude = std::max (hot, hot + meterCoeff * (attitude - hot));
            // Auto gain: keep the processed sound about as loud as the input.
            inEnv = inPow + envCoeff * (inEnv - inPow);
            outEnv = outPow + envCoeff * (outEnv - outPow);
            float comp = 1.0f;
            if (params.autoGain && outEnv > 1.0e-10f) comp = clampv (std::sqrt ((inEnv + 1.0e-12f) / outEnv), 0.03f, 2.0f);
            autoComp.setTarget (comp);
            const float ac = autoComp.next();

            for (int c = 0; c < 2; ++c) ch[c][i] = (dry[c] * (1.0f - m) + wet[c] * ac * m) * out;
        }
        attitudeOut.store (clampv ((gainToDb (attitude) + 12.0f) / 24.0f, 0.0f, 1.0f));
    }

    /** The transfer curve for each style. All have a slope of 1 for small signals.
        Each style also has its own internal headroom (k): lower = cleaner, higher = breaks up sooner. */
    float shape (float x) const
    {
        static constexpr float k[numStyles] = { 0.6f, 1.4f, 1.0f, 1.6f, 2.0f };
        const float ks = k[clampv (params.style, 0, (int) numStyles - 1)];
        return curve (x * ks) / ks;
    }

    float curve (float x) const
    {
        switch (params.style)
        {
            case styleA: // tape machine: smooth, gradual and symmetric
                return x / std::sqrt (1.0f + x * x);
            case styleE: // British console: transformer-style asymmetry
                return std::tanh (x + 0.6f * x * x / (1.0f + x * x));
            case styleN: // classic preamp: warm, mildly asymmetric
                return x >= 0.0f ? std::tanh (x) : std::tanh (0.8f * x) / 0.8f;
            case styleT: // triode tube: strong even harmonics
                return x >= 0.0f ? (1.0f - std::exp (-1.5f * x)) / 1.5f : (std::exp (0.5f * x) - 1.0f) / 0.5f;
            case styleP: // pentode tube: hard knee, odd harmonics
            default:
            {
                const float x2 = x * x;
                return x / std::sqrt (std::sqrt (1.0f + x2 * x2));
            }
        }
    }

private:
    Params params;
    double sr = 44100.0;
    Smoothed drive, mix, output, autoComp;
    Oversampler2x os[2];
    DCBlocker dc[2];
    Biquad lowCut[2], thumpEq[2], tilt[2], hc1[2], hc2[2];
    OnePoleLP hcPole[2];
    DelayLine dryDelay[2];
    float inEnv = 0.0f, outEnv = 0.0f, envCoeff = 0.999f, meterCoeff = 0.999f, attitude = 0.0f;
    std::atomic<float> attitudeOut { 0.0f };

    void updateFilters()
    {
        const float lc = clampv (params.lowCutHz, 20.0f, 1000.0f);
        const float hc = clampv (params.highCutHz, 1000.0f, 20000.0f);
        for (int c = 0; c < 2; ++c)
        {
            lowCut[c].set (Biquad::Type::highPass, lc, 0.707f, 0.0f, sr);
            thumpEq[c].set (Biquad::Type::peak, lc, 1.0f, 4.0f, sr);
            tilt[c].set (Biquad::Type::highShelf, 1500.0f, 0.6f, clampv (params.tone, -1.0f, 1.0f) * 9.0f, sr);
            hcPole[c].setCutoff (hc, sr);
            hc1[c].set (Biquad::Type::lowPass, hc, 0.707f, 0.0f, sr);
            hc2[c].set (Biquad::Type::lowPass, hc, 0.707f, 0.0f, sr);
        }
    }
};
} // namespace ptfx
