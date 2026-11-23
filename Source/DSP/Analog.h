// ProTunez FX Toys — analog-style building blocks for the deeper engines.
// Each block models one behaviour of real gear in a simple, cheap way.
#pragma once

#include "Common.h"
#include <cstdint>

namespace ptfx
{
/** 2x oversampling stage (linear-phase windowed sinc). Total delay = (Taps-1)/2 samples at the lower rate. */
template <int Taps>
class Oversampler2xT
{
public:
    static_assert (Taps % 2 == 1, "odd number of taps");
    static constexpr int kTaps = Taps;
    static constexpr int kHalf = (Taps - 1) / 2;

    Oversampler2xT()
    {
        double sum = 0.0;
        for (int i = 0; i < kTaps; ++i)
        {
            const double n = i - kHalf;
            const double sinc = i == kHalf ? 0.5 : std::sin (0.5 * (double) kPi * n) / ((double) kPi * n);
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

    void upsample (float x, float& o0, float& o1)
    {
        // Zero first, then the sample: keeps the total delay a whole number of samples.
        push (up, upPos, 0.0f);
        o0 = convolve (up, upPos);
        push (up, upPos, 2.0f * x);
        o1 = convolve (up, upPos);
    }

    float downsample (float a, float b)
    {
        push (down, downPos, a);
        push (down, downPos, b);
        return convolve (down, downPos);
    }

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
            pos = pos == 0 ? kTaps - 1 : pos - 1;
            acc += h[i] * buf[pos];
        }
        return acc;
    }
};

using Oversampler2x = Oversampler2xT<63>;

/** 4x oversampling: a sharp 2x stage, then a short 2x stage. */
class Oversampler4x
{
public:
    void reset() { outer.reset(); inner.reset(); }
    void up (float x, float out[4])
    {
        float a, b;
        outer.upsample (x, a, b);
        inner.upsample (a, out[0], out[1]);
        inner.upsample (b, out[2], out[3]);
    }
    float down (const float in[4])
    {
        const float a = inner.downsample (in[0], in[1]);
        const float b = inner.downsample (in[2], in[3]);
        return outer.downsample (a, b);
    }
    // The inner stage runs at 2x, so its delay counts half at the original rate.
    static constexpr int latency() { return Oversampler2xT<63>::latency() + Oversampler2xT<21>::latency() / 2; }

private:
    Oversampler2xT<63> outer;
    Oversampler2xT<21> inner;
};

/** Cheap repeatable noise (white, or pink for hiss). */
class NoiseSource
{
public:
    float white()
    {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return (float) (int32_t) state * (1.0f / 2147483648.0f);
    }
    float pink()
    {
        const float w = white();
        b0 = 0.99886f * b0 + w * 0.0555179f; b1 = 0.99332f * b1 + w * 0.0750759f;
        b2 = 0.96900f * b2 + w * 0.1538520f; b3 = 0.86650f * b3 + w * 0.3104856f;
        b4 = 0.55000f * b4 + w * 0.5329522f; b5 = -0.7616f * b5 - w * 0.0168980f;
        const float p = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f;
        b6 = w * 0.115926f;
        return p * 0.11f;
    }

private:
    uint32_t state = 0x9E3779B9u;
    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
};

/** Transformer: low frequencies saturate before high ones (the core is driven hardest by bass). */
class Transformer
{
public:
    void prepare (double sampleRate, float lowHz, float amount)
    {
        split.setCutoff (lowHz, sampleRate);
        gLow = 1.0f + 3.0f * clampv (amount, 0.0f, 2.0f);
        gHigh = 1.0f + 0.4f * clampv (amount, 0.0f, 2.0f);
    }
    void reset() { split.reset(); }
    float process (float x)
    {
        const float low = split.process (x), high = x - low;
        return std::tanh (low * gLow) / gLow + std::tanh (high * gHigh) / gHigh;
    }

private:
    OnePoleLP split;
    float gLow = 1.0f, gHigh = 1.0f;
};

/** Tube gain stage: asymmetric curve whose bias moves with the signal (harmonics change with level),
    plus power-supply sag (gain dips on sustained loud parts). */
class TubeStage
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        envA = (float) std::exp (-1.0 / (0.004 * sr));
        envR = (float) std::exp (-1.0 / (0.06 * sr));
        sagA = (float) std::exp (-1.0 / (0.08 * sr));
        sagR = (float) std::exp (-1.0 / (0.35 * sr));
        reset();
    }
    void reset() { env = sagEnv = 0.0f; }
    void setDrive (float g) { drive = g; }
    void setBias (float b) { bias = b; }
    void setBiasShift (float s) { biasShift = s; }
    void setSag (float s) { sag = s; }
    void setCurve (float softness, float negScale) { soft = softness; neg = negScale; }

    float process (float x)
    {
        const float a = std::fabs (x);
        env = undenorm (a > env ? a + envA * (env - a) : a + envR * (env - a));
        sagEnv = undenorm (a > sagEnv ? a + sagA * (sagEnv - a) : a + sagR * (sagEnv - a));
        const float g = drive / (1.0f + sag * 2.5f * sagEnv * drive * 0.5f);
        const float b = bias + biasShift * std::min (env * drive, 2.0f) * 0.25f;
        return curve (x * g + b) - curve (b);
    }

    float curve (float v) const
    {
        if (v >= 0.0f) return soft > 0.0f ? v / std::pow (1.0f + std::pow (v, 2.5f / soft), soft / 2.5f) : std::tanh (v);
        return std::tanh (v * neg) / neg;
    }

private:
    double sr = 48000.0;
    float drive = 1.0f, bias = 0.0f, biasShift = 0.0f, sag = 0.0f, soft = 0.0f, neg = 0.7f;
    float env = 0.0f, sagEnv = 0.0f, envA = 0.9f, envR = 0.99f, sagA = 0.999f, sagR = 0.9999f;
};

/** Tape hysteresis: magnetisation lags the input (memory of direction), then saturates.
    The memory relaxes over ~10 ms, so it never sticks after the music stops. */
class TapeHysteresis
{
public:
    void prepare (double sampleRate) { relax = (float) std::exp (-1.0 / (0.01 * sampleRate)); reset(); }
    void reset() { m = 0.0f; }
    void setDrive (float d) { drive = std::max (0.1f, d); width = 0.06f; }
    float process (float x)
    {
        if (x > m + width) m = x - width;
        else if (x < m - width) m = x + width;
        m = undenorm (m * relax);
        const float v = drive * (0.65f * x + 0.35f * m);
        return std::tanh (v) / drive;
    }

private:
    float m = 0.0f, drive = 1.0f, width = 0.06f, relax = 0.999f;
};

/** Compander as used around bucket-brigade delays: 2:1 compression before, 1:2 expansion after.
    Noise added in between "breathes" with the music. */
class Compander
{
public:
    void prepare (double sampleRate)
    {
        coef = (float) std::exp (-1.0 / (0.01 * sampleRate));
        reset();
    }
    void reset() { e1 = e2 = 0.0f; }
    float compress (float x)
    {
        e1 = undenorm (std::fabs (x) + coef * (e1 - std::fabs (x)));
        return x / std::sqrt (e1 + 1.0e-4f) * 0.3f;
    }
    float expand (float y)
    {
        const float a = std::fabs (y) / 0.3f;
        e2 = undenorm (a + coef * (e2 - a));
        return y / 0.3f * (e2 + 1.0e-2f * 0.0f);
    }

private:
    float coef = 0.99f, e1 = 0.0f, e2 = 0.0f;
};

/** Bucket-brigade colour: the chip's clock slows as the delay gets longer, so its filters close down;
    plus a little clock noise. Use pre() before the delay line and post() after it. */
class BbdColour
{
public:
    void prepare (double sampleRate) { sr = sampleRate; setDelaySeconds (0.3f); reset(); }
    void reset() { for (auto* f : { &aa1, &aa2, &rc1, &rc2 }) f->reset(); }
    void setNoise (float amount) { noiseAmt = amount; }
    void setDelaySeconds (float s)
    {
        // A 4096-stage chip: usable bandwidth ~ stages / (5 x delay).
        const float bw = clampv (4096.0f / (5.0f * std::max (0.005f, s)), 1200.0f, 0.45f * (float) sr);
        if (std::fabs (bw - cutoff) < 1.0f) return;
        cutoff = bw;
        aa1.set (Biquad::Type::lowPass, bw, 0.54f, 0.0f, sr);
        aa2.set (Biquad::Type::lowPass, bw, 1.31f, 0.0f, sr);
        rc1.set (Biquad::Type::lowPass, bw, 0.54f, 0.0f, sr);
        rc2.set (Biquad::Type::lowPass, bw, 1.31f, 0.0f, sr);
    }
    float pre (float x) { return aa2.process (aa1.process (x)); }
    float post (float x) { return rc2.process (rc1.process (x + noiseAmt * 0.002f * noise.white())); }

private:
    double sr = 48000.0;
    float cutoff = 0.0f, noiseAmt = 0.0f;
    Biquad aa1, aa2, rc1, rc2;
    NoiseSource noise;
};

/** Tape head bump: a low resonance whose frequency follows tape speed. */
class HeadBump
{
public:
    void prepare (double sampleRate) { sr = sampleRate; }
    void set (float hz, float gainDb) { f.set (Biquad::Type::peak, clampv (hz, 20.0f, 400.0f), 1.3f, gainDb, sr); }
    void reset() { f.reset(); }
    float process (float x) { return f.process (x); }

private:
    double sr = 48000.0;
    Biquad f;
};
} // namespace ptfx
