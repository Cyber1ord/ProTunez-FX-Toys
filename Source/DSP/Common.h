// ProTunez FX Toys — shared DSP building blocks.
// Pure C++ (no JUCE) so the same sound engines can run in the plugin,
// in the unit tests, and later in the browser test copy.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace ptfx
{
constexpr float kPi = 3.14159265358979323846f;

inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
inline float gainToDb (float g) { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
template <typename T> inline T clampv (T v, T lo, T hi) { return std::min (hi, std::max (lo, v)); }

/** Flushes values that are too small to hear to zero (avoids slow "denormal" maths). */
inline float undenorm (float x) { return std::fabs (x) < 1.0e-15f ? 0.0f : x; }

/** Smooths a parameter towards its target so knob moves don't click. */
class Smoothed
{
public:
    void prepare (double sampleRate, float timeMs = 20.0f)
    {
        coeff = (float) std::exp (-1.0 / (0.001 * timeMs * sampleRate));
    }
    void setTarget (float t) { target = t; }
    void snap (float v) { target = current = v; }
    float next() { current = target + coeff * (current - target); return current; }
    float get() const { return current; }
    float getTarget() const { return target; }

private:
    float coeff = 0.0f, current = 0.0f, target = 0.0f;
};

/** One-pole low-pass filter. */
class OnePoleLP
{
public:
    void setCutoff (float hz, double sampleRate)
    {
        hz = clampv (hz, 1.0f, 0.49f * (float) sampleRate);
        a = (float) std::exp (-2.0 * (double) kPi * hz / sampleRate);
    }
    float process (float x) { z = undenorm (x + a * (z - x)); return z; }
    void reset() { z = 0.0f; }

private:
    float a = 0.0f, z = 0.0f;
};

/** One-pole high-pass filter. */
class OnePoleHP
{
public:
    void setCutoff (float hz, double sampleRate) { lp.setCutoff (hz, sampleRate); }
    float process (float x) { return x - lp.process (x); }
    void reset() { lp.reset(); }

private:
    OnePoleLP lp;
};

/** Removes DC offset (asymmetric saturation creates some). */
class DCBlocker
{
public:
    float process (float x)
    {
        const float y = x - x1 + 0.995f * y1;
        x1 = x;
        y1 = undenorm (y);
        return y1;
    }
    void reset() { x1 = y1 = 0.0f; }

private:
    float x1 = 0.0f, y1 = 0.0f;
};

/** Standard "cookbook" biquad filter (Robert Bristow-Johnson's formulas). */
class Biquad
{
public:
    enum class Type { lowPass, highPass, bandPass, lowShelf, highShelf, peak };

    void set (Type type, float hz, float q, float gainDb, double sampleRate)
    {
        hz = clampv (hz, 5.0f, 0.45f * (float) sampleRate);
        const double w = 2.0 * (double) kPi * hz / sampleRate;
        const double cw = std::cos (w), sw = std::sin (w);
        const double alpha = sw / (2.0 * std::max (0.05f, q));
        const double A = std::pow (10.0, gainDb / 40.0);
        double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

        switch (type)
        {
            case Type::lowPass:  b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = (1 - cw) / 2; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case Type::highPass: b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = (1 + cw) / 2; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case Type::bandPass: b0 = alpha; b1 = 0; b2 = -alpha; a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
            case Type::peak:
                b0 = 1 + alpha * A; b1 = -2 * cw; b2 = 1 - alpha * A;
                a0 = 1 + alpha / A; a1 = -2 * cw; a2 = 1 - alpha / A; break;
            case Type::lowShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                b0 = A * ((A + 1) - (A - 1) * cw + s); b1 = 2 * A * ((A - 1) - (A + 1) * cw);
                b2 = A * ((A + 1) - (A - 1) * cw - s); a0 = (A + 1) + (A - 1) * cw + s;
                a1 = -2 * ((A - 1) + (A + 1) * cw);    a2 = (A + 1) + (A - 1) * cw - s; break;
            }
            case Type::highShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                b0 = A * ((A + 1) + (A - 1) * cw + s); b1 = -2 * A * ((A - 1) + (A + 1) * cw);
                b2 = A * ((A + 1) + (A - 1) * cw - s); a0 = (A + 1) - (A - 1) * cw + s;
                a1 = 2 * ((A - 1) - (A + 1) * cw);     a2 = (A + 1) - (A - 1) * cw - s; break;
            }
        }
        c0 = (float) (b0 / a0); c1 = (float) (b1 / a0); c2 = (float) (b2 / a0);
        d1 = (float) (a1 / a0); d2 = (float) (a2 / a0);
    }

    float process (float x)
    {
        const float y = c0 * x + s1;
        s1 = undenorm (c1 * x - d1 * y + s2);
        s2 = undenorm (c2 * x - d2 * y);
        return y;
    }
    void reset() { s1 = s2 = 0.0f; }

private:
    float c0 = 1, c1 = 0, c2 = 0, d1 = 0, d2 = 0, s1 = 0, s2 = 0;
};

/** Circular delay buffer with whole-sample and smooth (fractional) reads. */
class DelayLine
{
public:
    void allocate (int maxSamples)
    {
        int size = 1;
        while (size < maxSamples + 4) size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }
    void reset() { std::fill (buffer.begin(), buffer.end(), 0.0f); writePos = 0; }
    void push (float x) { buffer[(size_t) writePos] = x; writePos = (writePos + 1) & mask; }

    /** Sample written `delay` samples ago (delay >= 1 means before the latest push). */
    float read (int delay) const { return buffer[(size_t) ((writePos - delay) & mask)]; }

    /** Smooth read between samples (4-point Hermite interpolation). */
    float readFrac (float delay) const
    {
        delay = clampv (delay, 1.0f, (float) (mask - 3));
        const int i = (int) delay;
        const float f = delay - (float) i;
        const float xm1 = read (i - 1 < 1 ? 1 : i - 1), x0 = read (i), x1 = read (i + 1), x2 = read (i + 2);
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
    int capacity() const { return mask - 3; }

private:
    std::vector<float> buffer { 0.0f };
    int mask = 0, writePos = 0;
};

/** Click-free effect switching: fades down over 10 ms, signals the swap, fades back up. */
class Crossfader
{
public:
    void prepare (double sampleRate) { step = (float) (1.0 / (0.010 * sampleRate)); }
    void start() { phase = fadingOut; swapFlag = false; }
    /** Advance one sample. */
    void next()
    {
        swapFlag = false;
        if (phase == fadingOut)
        {
            g -= step;
            if (g <= 0.0f) { g = 0.0f; phase = fadingIn; swapFlag = true; }
        }
        else if (phase == fadingIn)
        {
            g += step;
            if (g >= 1.0f) { g = 1.0f; phase = idle; }
        }
    }
    float gain() const { return g; }
    bool swapNow() const { return swapFlag; }
    bool active() const { return phase != idle; }

private:
    enum Phase { idle, fadingOut, fadingIn } phase = idle;
    float g = 1.0f, step = 0.002f;
    bool swapFlag = false;
};

/** Every effect processes a stereo block in place. */
class Effect
{
public:
    virtual ~Effect() = default;
    virtual void prepare (double sampleRate, int maxBlockSize) = 0;
    virtual void reset() = 0;
    virtual void process (float* left, float* right, int numSamples) = 0;
    /** Samples of delay this effect adds (so the rack can line up the dry signal). */
    virtual int latency() const { return 0; }
    virtual void setTempo (double /*bpm*/) {}
    /** A 0..1 value for the effect's meter (attitude, gain reduction...). */
    virtual float meter() const { return 0.0f; }
};
} // namespace ptfx
