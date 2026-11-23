// Tests for the ProTunez FX Toys sound engines. Run on any computer (no JUCE needed).
#include "../Source/DSP/Echo.h"
#include "../Source/DSP/Heat.h"
#include "../Source/DSP/Plate.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

using namespace ptfx;
using Catch::Approx;

namespace
{
constexpr double kSr = 48000.0;
constexpr int kBlock = 256;

struct Stereo
{
    std::vector<float> l, r;
    explicit Stereo (int n) : l ((size_t) n, 0.0f), r ((size_t) n, 0.0f) {}
};

void run (Effect& fx, Stereo& s)
{
    for (size_t i = 0; i < s.l.size(); i += kBlock)
    {
        const int n = (int) std::min ((size_t) kBlock, s.l.size() - i);
        fx.process (s.l.data() + i, s.r.data() + i, n);
    }
}

Stereo sine (int n, float freq, float amp)
{
    Stereo s (n);
    for (int i = 0; i < n; ++i) s.l[(size_t) i] = s.r[(size_t) i] = amp * std::sin (2.0f * kPi * freq * (float) i / (float) kSr);
    return s;
}

bool allFinite (const Stereo& s)
{
    for (size_t i = 0; i < s.l.size(); ++i)
        if (! std::isfinite (s.l[i]) || ! std::isfinite (s.r[i])) return false;
    return true;
}

float peak (const std::vector<float>& v, size_t from = 0, size_t to = SIZE_MAX)
{
    float p = 0.0f;
    for (size_t i = from; i < std::min (to, v.size()); ++i) p = std::max (p, std::fabs (v[i]));
    return p;
}

float rms (const std::vector<float>& v, size_t from, size_t to)
{
    double acc = 0.0;
    to = std::min (to, v.size());
    for (size_t i = from; i < to; ++i) acc += (double) v[i] * v[i];
    return (float) std::sqrt (acc / (double) std::max<size_t> (1, to - from));
}
} // namespace

namespace
{
/** Level of a pure tone in a signal (Goertzel), after skipping the first `skip` samples. */
float toneLevel (const std::vector<float>& v, float freq, size_t skip)
{
    const double w = 2.0 * 3.14159265358979 * freq / kSr, c = 2.0 * std::cos (w);
    double s1 = 0, s2 = 0;
    for (size_t i = skip; i < v.size(); ++i) { const double s0 = v[i] + c * s1 - s2; s2 = s1; s1 = s0; }
    const double n = (double) (v.size() - skip);
    return (float) (2.0 * std::sqrt (std::max (0.0, s1 * s1 + s2 * s2 - c * s1 * s2)) / n);
}

Heat::Params cleanHeat()
{
    Heat::Params p;
    p.drive = 0.0f; p.autoGain = false; p.mix = 1.0f; p.outputDb = 0.0f; p.punish = false;
    p.lowCutHz = 20.0f; p.highCutHz = 20000.0f; p.tone = 0.0f; p.thump = false; p.steep = false;
    return p;
}

float heatGainDb (Heat::Params p, float freq, float amp)
{
    Heat h; h.setParams (p); h.prepare (kSr, kBlock);
    auto s = sine (48000, freq, amp);
    run (h, s);
    return gainToDb (toneLevel (s.l, freq, 24000) / amp);
}

float heatThd (int style)
{
    auto p = cleanHeat(); p.style = style; p.drive = 6.0f;
    Heat h; h.setParams (p); h.prepare (kSr, kBlock);
    auto s = sine (48000, 200.0f, 0.3f);
    run (h, s);
    const float f1 = toneLevel (s.l, 200.0f, 24000);
    double sum = 0;
    for (int k = 2; k <= 9; ++k) { const float hk = toneLevel (s.l, 200.0f * (float) k, 24000); sum += (double) hk * hk; }
    return (float) (std::sqrt (sum) / f1);
}
} // namespace

TEST_CASE ("Heat: silence stays silent and extreme settings stay finite in every style")
{
    for (int style = 0; style < Heat::numStyles; ++style)
    {
        auto p = cleanHeat(); p.style = style; p.drive = 10.0f; p.punish = true; p.thump = true; p.steep = true; p.autoGain = true;
        Heat h; h.setParams (p); h.prepare (kSr, kBlock);
        Stereo quiet (48000);
        run (h, quiet);
        REQUIRE (peak (quiet.l) < 1.0e-6f);
        auto loud = sine (48000, 220.0f, 2.0f);
        run (h, loud);
        REQUIRE (allFinite (loud));
        REQUIRE (peak (loud.l) < 4.0f);
    }
}

TEST_CASE ("Heat: Drive 0 to 10 adds 36 dB of gain before the saturation")
{
    auto p = cleanHeat();
    const float g0 = heatGainDb (p, 1000.0f, 1.0e-4f);
    p.drive = 10.0f;
    const float g10 = heatGainDb (p, 1000.0f, 1.0e-4f);
    REQUIRE (g10 - g0 == Approx (36.0f).margin (1.5f));
}

TEST_CASE ("Heat: Punish adds 20 dB of gain")
{
    auto p = cleanHeat();
    const float off = heatGainDb (p, 1000.0f, 1.0e-4f);
    p.punish = true;
    const float on = heatGainDb (p, 1000.0f, 1.0e-4f);
    REQUIRE (on - off == Approx (20.0f).margin (1.5f));
}

TEST_CASE ("Heat: Steep makes the High Cut much sharper")
{
    auto p = cleanHeat(); p.highCutHz = 2000.0f;
    const float pass = heatGainDb (p, 200.0f, 1.0e-3f);
    const float gentle = pass - heatGainDb (p, 8000.0f, 1.0e-3f);
    p.steep = true;
    const float steep = heatGainDb (p, 200.0f, 1.0e-3f) - heatGainDb (p, 8000.0f, 1.0e-3f);
    REQUIRE (gentle > 9.0f);
    REQUIRE (gentle < 15.0f);
    REQUIRE (steep >= 50.0f);
}

TEST_CASE ("Heat: Thump boosts the lows at the Low Cut frequency")
{
    auto p = cleanHeat(); p.lowCutHz = 100.0f;
    const float off = heatGainDb (p, 100.0f, 1.0e-3f);
    p.thump = true;
    const float on = heatGainDb (p, 100.0f, 1.0e-3f);
    REQUIRE (on - off > 2.0f);
    REQUIRE (on - off < 5.0f);
}

TEST_CASE ("Heat: with High Cut off, 15 kHz passes within 1 dB")
{
    auto p = cleanHeat();
    REQUIRE (std::fabs (heatGainDb (p, 15000.0f, 1.0e-3f) - heatGainDb (p, 1000.0f, 1.0e-3f)) < 1.0f);
}

TEST_CASE ("Heat: the five styles each sound different")
{
    float thd[Heat::numStyles];
    for (int s = 0; s < Heat::numStyles; ++s) thd[s] = heatThd (s);
    for (int a = 0; a < Heat::numStyles; ++a)
        for (int b = a + 1; b < Heat::numStyles; ++b)
            REQUIRE (std::fabs (thd[a] - thd[b]) / std::max (thd[a], thd[b]) > 0.05f);
}

TEST_CASE ("Heat: Mix at zero gives back the input, delayed by the reported latency")
{
    auto p = cleanHeat(); p.mix = 0.0f; p.drive = 7.0f;
    Heat h; h.setParams (p); h.prepare (kSr, kBlock);
    auto in = sine (4800, 440.0f, 0.5f);
    auto s = in;
    run (h, s);
    const int lat = h.latency();
    for (size_t i = 200; i < 4800; ++i) REQUIRE (s.l[i] == Approx (in.l[i - (size_t) lat]).margin (1.0e-5));
}

TEST_CASE ("Heat: the processed signal lines up in time with the dry signal")
{
    auto p = cleanHeat(); p.style = Heat::styleA;
    Heat h; h.setParams (p); h.prepare (kSr, kBlock);
    auto in = sine (9600, 1000.0f, 0.01f);
    auto s = in;
    run (h, s);
    const int lat = h.latency();
    auto errorAt = [&] (int offset)
    {
        double err = 0.0, ref = 0.0, k = 0.0, kk = 0.0;
        for (size_t i = 2000; i < 9600; ++i) { k += (double) s.l[i] * in.l[i - (size_t) offset]; kk += (double) in.l[i - (size_t) offset] * in.l[i - (size_t) offset]; }
        const double g = k / kk;
        for (size_t i = 2000; i < 9600; ++i) { const double d = s.l[i] - g * in.l[i - (size_t) offset]; err += d * d; ref += (double) in.l[i] * in.l[i]; }
        return std::sqrt (err / ref);
    };
    for (int off = lat - 3; off <= lat + 3; ++off)
        if (off != lat) REQUIRE (errorAt (lat) < errorAt (off));
}

TEST_CASE ("Heat: Auto gain keeps loudness similar at high drive, even with Punish")
{
    auto p = cleanHeat(); p.drive = 8.0f; p.punish = true; p.autoGain = true;
    Heat h; h.setParams (p); h.prepare (kSr, kBlock);
    auto in = sine (96000, 200.0f, 0.3f);
    auto s = in;
    run (h, s);
    const float ratio = rms (s.l, 48000, 96000) / rms (in.l, 48000, 96000);
    REQUIRE (ratio > 0.6f);
    REQUIRE (ratio < 1.6f);
}

TEST_CASE ("Heat: the attitude meter rises with Drive")
{
    auto meterAt = [] (float drive)
    {
        auto p = cleanHeat(); p.drive = drive;
        Heat h; h.setParams (p); h.prepare (kSr, kBlock);
        auto s = sine (24000, 200.0f, 0.2f);
        run (h, s);
        return h.meter();
    };
    REQUIRE (meterAt (8.0f) > meterAt (2.0f) + 0.1f);
    REQUIRE (meterAt (10.0f) <= 1.0f);
}

TEST_CASE ("Echo: an impulse comes back after the set time")
{
    Echo e;
    Echo::Params p; p.timeMs = 100.0f; p.feedback = 0.0f; p.mix = 1.0f; p.style = Echo::digital; p.wobble = 0.0f;
    p.saturation = 0.0f; p.lowCutHz = 20.0f; p.highCutHz = 20000.0f;
    e.setParams (p); e.prepare (kSr, kBlock);
    Stereo s (12000);
    s.l[0] = s.r[0] = 1.0f;
    run (e, s);
    size_t best = 0; float bestV = 0.0f;
    for (size_t i = 0; i < s.l.size(); ++i) if (std::fabs (s.l[i]) > bestV) { bestV = std::fabs (s.l[i]); best = i; }
    REQUIRE ((int) best == Approx (4800).margin (3));
    REQUIRE (bestV > 0.5f);
}

TEST_CASE ("Echo: tempo sync follows the host tempo")
{
    Echo e;
    Echo::Params p; p.sync = true; p.division = 8; // quarter note
    e.setParams (p); e.prepare (kSr, kBlock);
    e.setTempo (120.0);
    REQUIRE (e.currentTimeMs() == Approx (500.0f));
    e.setTempo (100.0);
    REQUIRE (e.currentTimeMs() == Approx (600.0f));
}

TEST_CASE ("Echo: full feedback in every style and mode never blows up")
{
    for (int style = 0; style < Echo::numStyles; ++style)
        for (int mode = 0; mode < Echo::numModes; ++mode)
        {
            Echo e;
            Echo::Params p; p.style = style; p.mode = mode; p.feedback = 1.0f; p.timeMs = 50.0f; p.saturation = 1.0f; p.wobble = 1.0f;
            e.setParams (p); e.prepare (kSr, kBlock);
            auto s = sine (48000 * 4, 300.0f, 0.9f);
            run (e, s);
            REQUIRE (allFinite (s));
            REQUIRE (peak (s.l) < 8.0f);
        }
}

TEST_CASE ("Echo: ping-pong sends the first repeat to one side")
{
    Echo e;
    Echo::Params p; p.mode = Echo::pingPong; p.timeMs = 50.0f; p.feedback = 0.5f; p.mix = 1.0f; p.style = Echo::digital; p.wobble = 0.0f;
    e.setParams (p); e.prepare (kSr, kBlock);
    Stereo s (12000);
    s.l[0] = s.r[0] = 1.0f;
    run (e, s);
    // First repeat (~2400) mostly left, second (~4800) mostly right.
    REQUIRE (peak (s.l, 2300, 2500) > 2.0f * peak (s.r, 2300, 2500));
    REQUIRE (peak (s.r, 4700, 4900) > 2.0f * peak (s.l, 4700, 4900));
}

TEST_CASE ("Plate: makes a tail that dies away and stays stable at maximum decay")
{
    Plate pl;
    Plate::Params p; p.mix = 1.0f; p.decay = 0.5f; p.preDelayMs = 0.0f;
    pl.setParams (p); pl.prepare (kSr, kBlock);
    Stereo s ((int) kSr * 6);
    s.l[0] = s.r[0] = 1.0f;
    run (pl, s);
    REQUIRE (allFinite (s));
    const float early = rms (s.l, 4800, 24000), late = rms (s.l, (size_t) kSr * 5, (size_t) kSr * 6);
    REQUIRE (early > 1.0e-3f);
    REQUIRE (late < early * 0.1f);

    Plate big;
    Plate::Params q; q.mix = 1.0f; q.decay = 1.0f; q.damping = 0.0f; q.modulation = 1.0f; q.size = 1.0f;
    big.setParams (q); big.prepare (kSr, kBlock);
    auto loud = sine ((int) kSr * 10, 500.0f, 0.9f);
    run (big, loud);
    REQUIRE (allFinite (loud));
    REQUIRE (peak (loud.l) < 10.0f);
}

TEST_CASE ("Plate: the reverb is stereo (left and right differ)")
{
    Plate pl;
    Plate::Params p; p.mix = 1.0f; p.width = 1.0f;
    pl.setParams (p); pl.prepare (kSr, kBlock);
    Stereo s (48000);
    s.l[0] = s.r[0] = 1.0f;
    run (pl, s);
    double diff = 0.0;
    for (size_t i = 0; i < s.l.size(); ++i) diff += std::fabs (s.l[i] - s.r[i]);
    REQUIRE (diff > 1.0);
}


TEST_CASE ("Crossfader: switching effects ramps smoothly, never jumps")
{
    Crossfader x;
    x.prepare (kSr);
    x.start();
    float prev = x.gain();
    float maxStep = 0.0f;
    bool sawSwap = false;
    for (int i = 0; i < (int) (0.03 * kSr); ++i)
    {
        x.next();
        if (x.swapNow()) sawSwap = true;
        maxStep = std::max (maxStep, std::fabs (x.gain() - prev));
        prev = x.gain();
    }
    REQUIRE (sawSwap);
    REQUIRE (maxStep < 0.01f);
    REQUIRE (x.gain() == Catch::Approx (1.0f));
    REQUIRE_FALSE (x.active());
}
