// Tests for the ProTunez FX Toys sound engines. Run on any computer (no JUCE needed).
#include "../Source/DSP/Rack.h"
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

TEST_CASE ("Heat: silence stays silent and output is always finite")
{
    for (int style = 0; style < Heat::numStyles; ++style)
    {
        Heat h;
        Heat::Params p; p.style = style; p.driveDb = 36.0f;
        h.setParams (p); h.prepare (kSr, kBlock);
        Stereo s (48000);
        run (h, s);
        REQUIRE (peak (s.l) < 1.0e-6f);

        auto loud = sine (48000, 220.0f, 2.0f);
        run (h, loud);
        REQUIRE (allFinite (loud));
        REQUIRE (peak (loud.l) < 4.0f);
    }
}

TEST_CASE ("Heat: Mix at zero gives back the input, delayed by the reported latency")
{
    Heat h;
    Heat::Params p; p.mix = 0.0f; p.driveDb = 20.0f;
    h.setParams (p); h.prepare (kSr, kBlock);
    auto in = sine (4800, 440.0f, 0.5f);
    auto s = in;
    run (h, s);
    const int lat = h.latency();
    for (size_t i = 200; i < 4800; ++i) REQUIRE (s.l[i] == Approx (in.l[i - (size_t) lat]).margin (1.0e-5));
}

TEST_CASE ("Heat: the processed signal lines up in time with the dry signal")
{
    // With almost no drive the wet path is nearly linear; it should match the delayed input.
    Heat h;
    Heat::Params p; p.driveDb = 0.0f; p.style = Heat::tape; p.autoGain = false; p.lowCutHz = 10.0f; p.highCutHz = 20000.0f;
    h.setParams (p); h.prepare (kSr, kBlock);
    auto in = sine (9600, 1000.0f, 0.05f);
    auto s = in;
    run (h, s);
    const int lat = h.latency();
    auto errorAt = [&] (int offset)
    {
        double err = 0.0, ref = 0.0;
        for (size_t i = 2000; i < 9600; ++i) { const double d = s.l[i] - 1.1 * in.l[i - (size_t) offset]; err += d * d; ref += (double) in.l[i] * in.l[i]; }
        return std::sqrt (err / ref);
    };
    // Best match is at exactly the reported latency.
    for (int off = lat - 3; off <= lat + 3; ++off)
        if (off != lat) REQUIRE (errorAt (lat) < errorAt (off));
    REQUIRE (errorAt (lat) < 0.1);
}

TEST_CASE ("Heat: drive adds harmonics and auto gain keeps loudness similar")
{
    Heat h;
    Heat::Params p; p.driveDb = 24.0f; p.autoGain = true;
    h.setParams (p); h.prepare (kSr, kBlock);
    auto in = sine (96000, 200.0f, 0.3f);
    auto s = in;
    run (h, s);
    const float ratio = rms (s.l, 48000, 96000) / rms (in.l, 48000, 96000);
    REQUIRE (ratio > 0.6f);
    REQUIRE (ratio < 1.6f);
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

TEST_CASE ("Squeeze: loud signals are turned down by about the ratio")
{
    Squeeze c;
    Squeeze::Params p; p.thresholdDb = -20.0f; p.ratio = 4.0f; p.kneeDb = 0.0f; p.attackMs = 1.0f; p.releaseMs = 50.0f;
    p.sidechainHpHz = 20.0f;
    c.setParams (p); c.prepare (kSr, kBlock);
    auto s = sine (48000, 1000.0f, dbToGain (-4.0f)); // 16 dB over
    run (c, s);
    const float outDb = gainToDb (peak (s.l, 24000));
    REQUIRE (outDb == Approx (-20.0f + 16.0f / 4.0f).margin (1.5f));
    REQUIRE (c.gainReductionDb.load() < -10.0f);
}

TEST_CASE ("Squeeze: quiet signals below threshold pass unchanged")
{
    Squeeze c;
    Squeeze::Params p; p.thresholdDb = -10.0f; p.kneeDb = 0.0f;
    c.setParams (p); c.prepare (kSr, kBlock);
    auto in = sine (24000, 1000.0f, dbToGain (-30.0f));
    auto s = in;
    run (c, s);
    for (size_t i = 0; i < in.l.size(); ++i) REQUIRE (s.l[i] == Approx (in.l[i]).margin (1.0e-6));
}

namespace
{
struct RackFx : Effect
{
    Rack rack;
    void prepare (double sr, int b) override { rack.prepare (sr, b); }
    void reset() override { rack.reset(); }
    void process (float* l, float* r, int n) override { rack.process (l, r, n); }
};
} // namespace

TEST_CASE ("Rack: everything switched off gives back the input, in time")
{
    RackFx r;
    for (int e = 0; e < Rack::numEffects; ++e) r.rack.setEnabled (e, false);
    r.prepare (kSr, kBlock);
    auto in = sine (9600, 440.0f, 0.5f);
    auto s = in;
    run (r, s);
    const int lat = r.rack.latency();
    REQUIRE (lat == Oversampler2x::latency());
    for (size_t i = 1000; i < in.l.size(); ++i) REQUIRE (s.l[i] == Approx (in.l[i - (size_t) lat]).margin (1.0e-5));
}

TEST_CASE ("Rack: global Mix at zero gives back the input even with effects on")
{
    RackFx r;
    r.rack.setMix (0.0f);
    r.prepare (kSr, kBlock);
    auto in = sine (9600, 440.0f, 0.5f);
    auto s = in;
    run (r, s);
    const int lat = r.rack.latency();
    for (size_t i = 1000; i < in.l.size(); ++i) REQUIRE (s.l[i] == Approx (in.l[i - (size_t) lat]).margin (1.0e-5));
}

TEST_CASE ("Rack: order can be changed and bad orders are refused")
{
    Rack r;
    r.setOrder ({ Rack::plate, Rack::echo, Rack::heat, Rack::squeeze });
    REQUIRE (r.getOrder()[0] == Rack::plate);
    r.setOrder ({ Rack::plate, Rack::plate, Rack::heat, Rack::squeeze }); // duplicate — ignored
    REQUIRE (r.getOrder()[1] == Rack::echo);
}

TEST_CASE ("Rack: blocks bigger than prepared are handled")
{
    RackFx r;
    r.rack.prepare (kSr, 64);
    auto s = sine (4096, 440.0f, 0.5f);
    r.rack.process (s.l.data(), s.r.data(), 4096);
    REQUIRE (allFinite (s));
}
