// Tests for the analog building blocks used by the deeper engines.
#include "../Source/DSP/Analog.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace ptfx;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;

std::vector<float> tone (int n, float f, float a)
{
    std::vector<float> v ((size_t) n);
    for (int i = 0; i < n; ++i) v[(size_t) i] = a * std::sin (2.0f * kPi * f * (float) i / (float) sr);
    return v;
}

float level (const std::vector<float>& v, float f, size_t from, size_t to)
{
    const double w = 2.0 * 3.14159265358979 * f / sr, c = 2.0 * std::cos (w);
    double s1 = 0, s2 = 0;
    for (size_t i = from; i < to; ++i) { const double s0 = v[i] + c * s1 - s2; s2 = s1; s1 = s0; }
    return (float) (2.0 * std::sqrt (std::max (0.0, s1 * s1 + s2 * s2 - c * s1 * s2)) / (double) (to - from));
}

float thd (const std::vector<float>& v, float f, size_t from)
{
    double h = 0;
    for (int k = 2; k <= 7; ++k) { const float x = level (v, f * (float) k, from, v.size()); h += (double) x * x; }
    return (float) std::sqrt (h) / level (v, f, from, v.size());
}

float rmsOf (const std::vector<float>& v, size_t a, size_t b)
{
    double e = 0; for (size_t i = a; i < b; ++i) e += (double) v[i] * v[i];
    return (float) std::sqrt (e / (double) (b - a));
}

template <typename F> std::vector<float> map (std::vector<float> v, F&& f) { for (auto& x : v) x = f (x); return v; }
} // namespace

TEST_CASE ("Analog: a transformer distorts low notes more than high notes")
{
    auto run = [] (float f)
    {
        Transformer t; t.prepare (sr, 150.0f, 1.0f);
        return thd (map (tone (48000, f, 0.7f), [&] (float x) { return t.process (x); }), f, 24000);
    };
    REQUIRE (run (80.0f) > 1.5f * run (2000.0f));
}

TEST_CASE ("Analog: a tube stage's harmonics change with how hard it is driven")
{
    auto h2ratio = [] (float amp)
    {
        TubeStage t; t.prepare (sr); t.setDrive (3.0f); t.setBias (0.1f); t.setBiasShift (0.6f); t.setSag (0.0f);
        auto y = map (tone (48000, 200.0f, amp), [&] (float x) { return t.process (x); });
        return level (y, 400.0f, 24000, y.size()) / level (y, 200.0f, 24000, y.size());
    };
    const float quiet = h2ratio (dbToGain (-20.0f)), loud = h2ratio (dbToGain (-3.0f));
    REQUIRE (std::fabs (loud - quiet) / std::max (loud, quiet) > 0.2f);
}

TEST_CASE ("Analog: a tube stage sags on sustained loud parts")
{
    TubeStage t; t.prepare (sr); t.setDrive (4.0f); t.setBias (0.0f); t.setBiasShift (0.0f); t.setSag (1.0f);
    auto y = map (tone (48000, 300.0f, 0.5f), [&] (float x) { return t.process (x); });
    const float early = rmsOf (y, 0, 480), late = rmsOf (y, 14400, 19200);
    REQUIRE (gainToDb (early / late) >= 1.0f);
}

TEST_CASE ("Analog: tape hysteresis remembers direction and lets go after the input stops")
{
    TapeHysteresis h; h.prepare (sr); h.setDrive (2.0f);
    // A slow ramp up then down: at the same input value, rising and falling outputs differ.
    std::vector<float> up, down;
    for (int i = 0; i <= 200; ++i) up.push_back (h.process (-0.8f + 1.6f * (float) i / 200.0f));
    for (int i = 0; i <= 200; ++i) down.push_back (h.process (0.8f - 1.6f * (float) i / 200.0f));
    REQUIRE (std::fabs (up[100] - down[100]) > 0.02f); // both at input 0.0
    float last = 1.0f;
    for (int i = 0; i < (int) (0.05 * sr); ++i) last = h.process (0.0f);
    REQUIRE (std::fabs (last) < 1.0e-3f);
}

TEST_CASE ("Analog: compress then expand gives back the original level")
{
    Compander c; c.prepare (sr);
    auto y = map (tone (48000, 500.0f, 0.3f), [&] (float x) { return c.expand (c.compress (x)); });
    REQUIRE (std::fabs (gainToDb (rmsOf (y, 24000, 48000) / (0.3f / std::sqrt (2.0f)))) < 1.0f);
}

TEST_CASE ("Analog: a bucket-brigade chip loses treble as its delay gets longer")
{
    auto at10k = [] (float delaySec)
    {
        BbdColour b; b.prepare (sr); b.setDelaySeconds (delaySec); b.setNoise (0.0f);
        auto y = map (tone (24000, 8000.0f, 0.1f), [&] (float x) { return b.post (b.pre (x)); });
        return level (y, 8000.0f, 12000, y.size());
    };
    REQUIRE (gainToDb (at10k (0.5f) / at10k (0.05f)) < -6.0f);
}

TEST_CASE ("Analog: 4x oversampling passes 18 kHz and has a whole-sample delay")
{
    Oversampler4x os;
    REQUIRE (Oversampler4x::latency() > 0);
    auto y = map (tone (48000, 18000.0f, 0.5f), [&] (float x) { float o[4]; os.up (x, o); return os.down (o); });
    REQUIRE (std::fabs (gainToDb (level (y, 18000.0f, 24000, y.size()) / 0.5f)) < 1.0f);
    Oversampler4x imp;
    std::vector<float> r;
    for (int i = 0; i < 200; ++i) { float o[4]; imp.up (i == 0 ? 1.0f : 0.0f, o); r.push_back (imp.down (o)); }
    size_t pk = 0; for (size_t i = 0; i < r.size(); ++i) if (std::fabs (r[i]) > std::fabs (r[pk])) pk = i;
    REQUIRE ((int) pk == Oversampler4x::latency());
    REQUIRE (std::fabs (r[pk]) > 0.8f);
}

TEST_CASE ("Analog: noise is silent at zero and never builds up")
{
    NoiseSource n;
    float mx = 0.0f;
    for (int i = 0; i < 480000; ++i) mx = std::max (mx, std::fabs (n.pink()));
    REQUIRE (mx < 1.5f);
    REQUIRE (mx > 0.05f);
}
