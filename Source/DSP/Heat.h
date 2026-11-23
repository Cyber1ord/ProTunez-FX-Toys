// ProTunez FX Toys — "Heat": analog-style saturation, from gentle warmth to fuzz.
#pragma once

#include "Common.h"

namespace ptfx
{
/** 2x oversampler (windowed-sinc FIR) so the distortion stays clean at high drive. */
class Oversampler2x
{
public:
    static constexpr int kTaps = 31;              // odd, linear phase
    static constexpr int kHalf = (kTaps - 1) / 2; // 15

    Oversampler2x()
    {
        // Low-pass at a quarter of the oversampled rate, Blackman window.
        double sum = 0.0;
        for (int i = 0; i < kTaps; ++i)
        {
            const double n = i - kHalf;
            const double sinc = n == 0 ? 0.5 : std::sin (0.5 * (double) kPi * n) / ((double) kPi * n);
            const double w = 0.42 - 0.5 * std::cos (2.0 * (double) kPi * i / (kTaps - 1))
                           + 0.08 * std::cos (4.0 * (double) kPi * i / (kTaps - 1));
            h[i] = (float) (sinc * w);
            sum += h[i];
        }
        for (auto& c : h) c = (float) (c / sum);
        reset();
    }

    void reset()
    {
        std::fill (std::begin (up), std::end (up), 0.0f);
        std::fill (std::begin (down), std::end (down), 0.0f);
        upPos = downPos = 0;
    }

    /** Turns one input sample into two oversampled ones. */
    void upsample (float x, float& o0, float& o1)
    {
        // Zero first, then the sample: keeps the total delay a whole number of samples.
        push (up, upPos, 0.0f);
        o0 = convolve (up, upPos);
        push (up, upPos, 2.0f * x);
        o1 = convolve (up, upPos);
    }

    /** Turns two oversampled samples back into one. */
    float downsample (float a, float b)
    {
        push (down, downPos, a);
        push (down, downPos, b);
        return convolve (down, downPos);
    }

    /** Total delay at the original rate, in samples. */
    static constexpr int latency() { return kHalf; }

private:
    float h[kTaps] {};
    float up[kTaps] {}, down[kTaps] {};
    int upPos = 0, downPos = 0;

    static void push (float* buf, int& pos, float x) { buf[pos] = x; pos = (pos + 1) % kTaps; }
    float convolve (const float* buf, int pos) const
    {
        float acc = 0.0f;
        for (int i = 0; i < kTaps; ++i)
        {
            pos = pos == 0 ? kTaps - 1 : pos - 1; // newest first
            acc += h[i] * buf[pos];
        }
        return acc;
    }
};

class Heat : public Effect
{
public:
    enum Style { warmTube = 0, tape, transistor, diode, fuzz, numStyles };

    struct Params
    {
        float driveDb  = 6.0f;    // 0..36
        int   style    = warmTube;
        float tone     = 0.0f;    // -1 dark .. +1 bright
        float lowCutHz = 20.0f;   // 20..1000 (applied before the distortion)
        float highCutHz = 20000.0f; // 1000..20000 (after)
        float mix      = 1.0f;    // 0..1
        float outputDb = 0.0f;    // -24..24
        bool  autoGain = true;
    };

    void setParams (const Params& p)
    {
        params = p;
        drive.setTarget (dbToGain (clampv (p.driveDb, 0.0f, 36.0f)));
        mix.setTarget (clampv (p.mix, 0.0f, 1.0f));
        output.setTarget (dbToGain (clampv (p.outputDb, -24.0f, 24.0f)));
        updateFilters();
    }

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        for (auto* s : { &drive, &mix, &output, &autoComp }) s->prepare (sr, 30.0f);
        drive.snap (dbToGain (params.driveDb));
        mix.snap (params.mix);
        output.snap (dbToGain (params.outputDb));
        autoComp.snap (1.0f);
        envAttack = (float) std::exp (-1.0 / (0.3 * sr)); // ~300 ms loudness tracking
        for (auto& d : dryDelay) d.allocate (Oversampler2x::latency() + 4);
        updateFilters();
        reset();
    }

    void reset() override
    {
        for (int c = 0; c < 2; ++c)
        {
            os[c].reset(); dc[c].reset(); lowCut[c].reset(); highCut[c].reset();
            tilt[c].reset(); dryDelay[c].reset();
        }
        inEnv = outEnv = 0.0f;
    }

    int latency() const override { return Oversampler2x::latency(); }

    void process (float* left, float* right, int n) override
    {
        float* ch[2] = { left, right };
        for (int i = 0; i < n; ++i)
        {
            const float g = drive.next(), m = mix.next(), out = output.next();
            float wet[2], dry[2];
            float inPow = 0.0f, outPow = 0.0f;

            for (int c = 0; c < 2; ++c)
            {
                const float x = ch[c][i];
                dryDelay[c].push (x);
                dry[c] = dryDelay[c].read (Oversampler2x::latency() + 1);

                const float pre = lowCut[c].process (x) * g;
                float a, b;
                os[c].upsample (pre, a, b);
                a = shape (a);
                b = shape (b);
                float y = os[c].downsample (a, b);
                y = dc[c].process (y);
                y = tilt[c].process (y);
                y = highCut[c].process (y);
                wet[c] = y;

                inPow += dry[c] * dry[c];
                outPow += y * y;
            }

            // Auto gain: keep the processed sound about as loud as the input,
            // so turning up Drive changes the colour, not just the volume.
            inEnv = inPow + envAttack * (inEnv - inPow);
            outEnv = outPow + envAttack * (outEnv - outPow);
            float comp = 1.0f;
            if (params.autoGain && outEnv > 1.0e-10f)
                comp = clampv (std::sqrt ((inEnv + 1.0e-12f) / outEnv), 0.05f, 2.0f);
            autoComp.setTarget (comp);
            const float ac = autoComp.next();

            for (int c = 0; c < 2; ++c)
                ch[c][i] = (dry[c] * (1.0f - m) + wet[c] * ac * m) * out;
        }
    }

    /** The transfer curve for each style (input already multiplied by Drive). */
    float shape (float x) const
    {
        switch (params.style)
        {
            case warmTube: // asymmetric: adds even harmonics, like a tube stage
                return x >= 0.0f ? std::tanh (x) : std::tanh (0.7f * x) / 0.7f * 0.85f;
            case tape:     // smooth, gentle compression
                return x / std::sqrt (1.0f + x * x) * 1.1f;
            case transistor: // firmer knee, odd harmonics
            {
                const float v = clampv (x * 0.8f, -1.0f, 1.0f);
                return 1.5f * (v - v * v * v / 3.0f);
            }
            case diode:    // biased, gritty
            {
                const float v = x + 0.15f;
                const float y = v >= 0.0f ? 1.0f - std::exp (-v) : -(1.0f - std::exp (v * 1.6f)) / 1.6f;
                return y - (1.0f - std::exp (-0.15f));
            }
            case fuzz:     // squared-off and aggressive
            default:
                return clampv (std::tanh (3.0f * x) * 1.2f, -1.0f, 1.0f);
        }
    }

private:
    Params params;
    double sr = 44100.0;
    Smoothed drive, mix, output, autoComp;
    Oversampler2x os[2];
    DCBlocker dc[2];
    OnePoleHP lowCut[2];
    Biquad highCut[2], tilt[2];
    DelayLine dryDelay[2];
    float inEnv = 0.0f, outEnv = 0.0f, envAttack = 0.999f;

    void updateFilters()
    {
        for (int c = 0; c < 2; ++c)
        {
            lowCut[c].setCutoff (clampv (params.lowCutHz, 10.0f, 2000.0f), sr);
            highCut[c].set (Biquad::Type::lowPass, clampv (params.highCutHz, 500.0f, 20000.0f), 0.707f, 0.0f, sr);
            tilt[c].set (Biquad::Type::highShelf, 1500.0f, 0.6f, clampv (params.tone, -1.0f, 1.0f) * 9.0f, sr);
        }
    }
};
} // namespace ptfx
