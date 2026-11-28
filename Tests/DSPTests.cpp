// Tests for the ProTunez FX Toys sound engines. Run on any computer (no JUCE needed).
#include "../Source/DSP/Echo.h"
#include "../Source/DSP/Heat.h"
#include "../Source/DSP/Plate.h"
#include <catch2/catch_approx.hpp>
#include <chrono>
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

/** THD (harmonics 2..7) of one Heat style at a given frequency, drive and amplitude; 1 s, first 24000 samples skipped. */
float heatThdAt (int style, float freq, float drive, float amp)
{
    auto p = cleanHeat(); p.style = style; p.drive = drive;
    Heat h; h.setParams (p); h.prepare (kSr, kBlock);
    auto s = sine (48000, freq, amp);
    run (h, s);
    const float f1 = toneLevel (s.l, freq, 24000);
    double sum = 0;
    for (int k = 2; k <= 7; ++k) { const float hk = toneLevel (s.l, freq * (float) k, 24000); sum += (double) hk * hk; }
    return (float) (std::sqrt (sum) / f1);
}

/** Like sine(), but continues a sine that already ran for `start` samples. */
Stereo sineAt (int n, float freq, float amp, int start)
{
    Stereo s (n);
    for (int i = 0; i < n; ++i) s.l[(size_t) i] = s.r[(size_t) i] = amp * std::sin (2.0f * kPi * freq * (float) (i + start) / (float) kSr);
    return s;
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

TEST_CASE ("Heat: style A has a tape head bump at 80 Hz at low drive")
{
    auto p = cleanHeat(); p.style = Heat::styleA; p.drive = 1.0f;
    const float bump = heatGainDb (p, 80.0f, 0.01f) - heatGainDb (p, 1000.0f, 0.01f);
    REQUIRE (bump >= 1.0f);
    REQUIRE (bump <= 3.0f);
}

TEST_CASE ("Heat: triode and pentode sag on sustained loud parts")
{
    for (int style : { (int) Heat::styleT, (int) Heat::styleP })
    {
        auto p = cleanHeat(); p.style = style; p.drive = 8.0f;
        Heat h; h.setParams (p); h.prepare (kSr, kBlock);
        auto s = sine (48000, 300.0f, 0.1f);
        run (h, s);
        const float dip = gainToDb (rms (s.l, 240, 720) / rms (s.l, 14400, 19200));
        INFO ("style " << style << " dip " << dip << " dB");
        REQUIRE (dip >= 1.0f);
    }
}

TEST_CASE ("Heat: console and preamp distort low notes more than high notes")
{
    for (int style : { (int) Heat::styleE, (int) Heat::styleN })
    {
        const float low = heatThdAt (style, 80.0f, 1.0f, 0.05f), high = heatThdAt (style, 2000.0f, 1.0f, 0.05f);
        INFO ("style " << style << " low " << low << " high " << high);
        REQUIRE (low > 1.3f * high);
    }
}

TEST_CASE ("Heat: the triode's even/odd harmonic balance changes with level")
{
    auto evenOdd = [] (float amp)
    {
        auto p = cleanHeat(); p.style = Heat::styleT; p.drive = 3.0f;
        Heat h; h.setParams (p); h.prepare (kSr, kBlock);
        auto s = sine (48000, 200.0f, amp);
        run (h, s);
        auto t = [&] (int k) { return toneLevel (s.l, 200.0f * (float) k, 24000); };
        return std::hypot (t (2), t (4)) / std::hypot (t (3), t (5));
    };
    const float quiet = evenOdd (dbToGain (-30.0f)), loud = evenOdd (dbToGain (-6.0f));
    INFO ("quiet " << quiet << " loud " << loud);
    REQUIRE (quiet > 2.0f * loud);
}

TEST_CASE ("Heat: Auto keeps every style within 3 dB of the input at drive 6, also after a style change")
{
    for (int a = 0; a < Heat::numStyles; ++a)
        for (int b = 0; b < Heat::numStyles; ++b)
        {
            if (a == b) continue;
            auto p = cleanHeat(); p.style = a; p.drive = 6.0f; p.autoGain = true;
            Heat h; h.setParams (p); h.prepare (kSr, kBlock);
            auto s1 = sineAt (48000, 200.0f, 0.3f, 0);
            run (h, s1);
            p.style = b;
            h.setParams (p);
            auto s2 = sineAt (48000, 200.0f, 0.3f, 48000);
            run (h, s2);
            const float ref = 0.3f / std::sqrt (2.0f);
            for (auto* s : { &s1, &s2 })
                for (size_t from = 4800; from + 2400 <= s->l.size(); from += 2400)
                {
                    const float db = gainToDb (rms (s->l, from, from + 2400) / ref);
                    INFO ("styles " << a << " -> " << b << " window at " << from << ": " << db << " dB");
                    CHECK (std::fabs (db) <= 3.0f);
                }
        }
}

TEST_CASE ("Heat: every style processes 1 s of 48 kHz stereo in under 250 ms")
{
    for (int style = 0; style < Heat::numStyles; ++style)
    {
        auto p = cleanHeat(); p.style = style; p.drive = 10.0f; p.punish = true; p.autoGain = true; p.mix = 0.5f;
        p.lowCutHz = 200.0f; p.thump = true; p.highCutHz = 5000.0f; p.steep = true; p.tone = 0.5f;
        Heat h; h.setParams (p); h.prepare (kSr, kBlock);
        auto s = sine (48000, 220.0f, 0.5f);
        const auto t0 = std::chrono::steady_clock::now();
        run (h, s);
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
        INFO ("style " << style << ": " << ms << " ms");
        UNSCOPED_INFO ("Heat CPU style " << style << ": " << ms << " ms");
        REQUIRE (ms < 250.0);
    }
}

TEST_CASE ("Heat: every style is silent, DC-free, finite and lets go at 44.1 and 96 kHz")
{
    for (double rate : { 44100.0, 96000.0 })
        for (int style = 0; style < Heat::numStyles; ++style)
        {
            auto p = cleanHeat(); p.style = style; p.drive = 10.0f; p.punish = true; p.autoGain = true; p.thump = true; p.steep = true;
            Heat h; h.setParams (p); h.prepare (rate, kBlock);
            const int n = (int) rate;
            auto runAt = [&] (Stereo& s) { for (size_t i = 0; i < s.l.size(); i += kBlock) { const int m = (int) std::min ((size_t) kBlock, s.l.size() - i); h.process (s.l.data() + i, s.r.data() + i, m); } };
            INFO ("style " << style << " rate " << rate);

            Stereo quiet (n); runAt (quiet);
            REQUIRE (peak (quiet.l) < 1.0e-6f);

            Stereo sq (n);
            for (int i = 0; i < n; ++i) sq.l[(size_t) i] = sq.r[(size_t) i] = ((i / 50) % 2 == 0) ? 1.0f : -1.0f;
            runAt (sq);
            REQUIRE (allFinite (sq));
            REQUIRE (peak (sq.l) < 4.0f);

            Stereo dc (2 * n);
            for (auto& v : dc.l) v = 0.5f;
            for (auto& v : dc.r) v = 0.5f;
            runAt (dc);
            REQUIRE (allFinite (dc));
            double mean = 0; for (int i = n; i < 2 * n; ++i) mean += dc.l[(size_t) i];
            REQUIRE (std::fabs (mean / n) < 1.0e-3);

            Stereo tail (n); runAt (tail);
            REQUIRE (peak (tail.l, (size_t) (n / 2)) < 1.0e-6f);
        }
}

namespace
{
Echo::Params dryEcho()
{
    Echo::Params p;
    p.mode = Echo::single; p.timeMode = Echo::timeMs; p.timeMs = 100.0f; p.feedback = 0.0f; p.mix = 1.0f;
    p.saturation = 0.0f; p.lowCutHz = 20.0f; p.highCutHz = 20000.0f; p.groove = 0.0f; p.feel = 0.0f;
    p.width = 0.0f; p.prime = false; p.style = Echo::StyleEdit::clean(); p.duck = 0.0f; p.freeze = false;
    return p;
}

std::vector<size_t> peaksAbove (const std::vector<float>& v, float thresh, size_t minGap)
{
    std::vector<size_t> out;
    for (size_t i = 1; i + 1 < v.size(); ++i)
        if (std::fabs (v[i]) > thresh && std::fabs (v[i]) >= std::fabs (v[i - 1]) && std::fabs (v[i]) >= std::fabs (v[i + 1]))
            if (out.empty() || i - out.back() > minGap) out.push_back (i);
    return out;
}

Stereo impulseThrough (Echo& e, int n)
{
    Stereo s (n);
    s.l[0] = s.r[0] = 1.0f;
    run (e, s);
    return s;
}

bool isPrime (int n) { if (n < 2) return false; for (int d = 2; d * d <= n; ++d) if (n % d == 0) return false; return true; }
} // namespace

TEST_CASE ("Echo: an impulse comes back after the set time")
{
    Echo e; e.setParams (dryEcho()); e.prepare (kSr, kBlock);
    auto s = impulseThrough (e, 12000);
    auto pk = peaksAbove (s.l, 0.3f, 100);
    REQUIRE (pk.size() == 1);
    REQUIRE ((int) pk[0] == Approx (4800).margin (2));
}

TEST_CASE ("Echo: Prime Numbers moves the echo to a prime number of samples")
{
    auto p = dryEcho(); p.prime = true;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    auto pk = peaksAbove (impulseThrough (e, 12000).l, 0.3f, 100);
    REQUIRE (pk.size() == 1);
    REQUIRE (isPrime (Echo::nearestPrime (4800)));
    REQUIRE ((int) pk[0] == Approx (Echo::nearestPrime (4800)).margin (1));
    REQUIRE (std::abs (Echo::nearestPrime (4800) - 4800) <= 10);
}

TEST_CASE ("Echo: note, dotted and triplet times follow the tempo")
{
    auto p = dryEcho(); p.timeMode = Echo::timeNote; p.note = Echo::noteQuarter;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    e.setTempo (120.0);
    REQUIRE (e.echoTimeMs (0) == Approx (500.0f));
    p.timeMode = Echo::timeDot; e.setParams (p);
    REQUIRE (e.echoTimeMs (0) == Approx (750.0f));
    p.timeMode = Echo::timeTrip; e.setParams (p);
    REQUIRE (e.echoTimeMs (0) == Approx (1000.0f / 3.0f).margin (0.01));
}

TEST_CASE ("Echo: with no tempo from the host, synced times use 120 BPM")
{
    auto p = dryEcho(); p.timeMode = Echo::timeNote; p.note = Echo::noteQuarter;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    e.setTempo (0.0);
    REQUIRE (e.echoTimeMs (0) == Approx (500.0f));
}

TEST_CASE ("Echo: Groove shifts every other repeat (shuffle and swing)")
{
    auto p = dryEcho(); p.feedback = 0.7f; p.groove = 1.0f;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    auto pk = peaksAbove (impulseThrough (e, 20000).l, 0.2f, 500);
    REQUIRE (pk.size() >= 3);
    REQUIRE ((int) pk[0] == Approx (4800 - 1600).margin (3));
    REQUIRE ((int) pk[1] == Approx (9600).margin (3));
    REQUIRE ((int) pk[2] == Approx (9600 + 3200).margin (3));
}

TEST_CASE ("Echo: Feel moves the echoes ahead of or behind the beat by up to 25 ms")
{
    auto p = dryEcho(); p.feel = 1.0f;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    auto pk = peaksAbove (impulseThrough (e, 12000).l, 0.3f, 100);
    REQUIRE ((int) pk[0] == Approx (4800 - 1200).margin (2));
    p.feel = -1.0f; e.setParams (p); e.reset();
    pk = peaksAbove (impulseThrough (e, 12000).l, 0.3f, 100);
    REQUIRE ((int) pk[0] == Approx (4800 + 1200).margin (2));
}

TEST_CASE ("Echo: Ping-Pong bounces repeats from side to side")
{
    auto p = dryEcho(); p.mode = Echo::pingPong; p.feedback = 0.6f; p.width = 1.0f;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    auto s = impulseThrough (e, 12000);
    REQUIRE (peak (s.l, 4700, 4900) > 3.0f * peak (s.r, 4700, 4900));
    REQUIRE (peak (s.r, 9500, 9700) > 3.0f * peak (s.l, 9500, 9700));
}

TEST_CASE ("Echo: Dual mode runs two echoes with their own times")
{
    auto p = dryEcho(); p.mode = Echo::dual; p.timeMs = 100.0f; p.time2Ms = 150.0f; p.width = 1.0f;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    auto s = impulseThrough (e, 12000);
    auto l = peaksAbove (s.l, 0.3f, 100), r = peaksAbove (s.r, 0.3f, 100);
    REQUIRE (l.size() == 1); REQUIRE (r.size() == 1);
    REQUIRE ((int) l[0] == Approx (4800).margin (2));
    REQUIRE ((int) r[0] == Approx (7200).margin (2));
}

TEST_CASE ("Echo: Rhythm mode plays the taps of the pattern")
{
    auto p = dryEcho(); p.mode = Echo::rhythm; p.rhythmBeats = 2; p.shape = Echo::shapeFlat; p.panShape = Echo::panCenter;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock); e.setTempo (120.0);
    RhythmPattern pat; pat.count = 4;
    for (int i = 0; i < 4; ++i) pat.taps[(size_t) i] = { 0.25f * (float) (i + 1), 1.0f };
    e.setPattern (pat);
    auto pk = peaksAbove (impulseThrough (e, 30000).l, 0.2f, 1000);
    REQUIRE (pk.size() == 4);
    for (int i = 0; i < 4; ++i) REQUIRE ((int) pk[(size_t) i] == Approx (6000 * (i + 1)).margin (3));
}

TEST_CASE ("Echo: the Decay shape makes each tap quieter than the last")
{
    auto lv = Echo::shapeLevels (Echo::shapeDecay, 8);
    for (size_t i = 1; i < lv.size(); ++i) REQUIRE (lv[i] < lv[i - 1]);
    auto sw = Echo::shapeLevels (Echo::shapeReverse, 8);
    for (size_t i = 1; i < sw.size(); ++i) REQUIRE (sw[i] > sw[i - 1]);
}

TEST_CASE ("Echo: there are 30 styles and the Telephone style cuts the bass")
{
    REQUIRE (Echo::numStyles() == 30);
    auto bassLevel = [] (const char* name)
    {
        auto p = dryEcho();
        Echo::applyStyle (p, Echo::styleIndex (name));
        Echo e; e.setParams (p); e.prepare (kSr, kBlock);
        auto s = sine (48000, 100.0f, 0.1f);
        run (e, s);
        return rms (s.l, 24000, 48000);
    };
    REQUIRE (Echo::styleIndex ("Telephone") >= 0);
    REQUIRE (gainToDb (bassLevel ("Telephone") / bassLevel ("Clean Digital")) < -20.0f);
}

TEST_CASE ("Echo: Duck turns the echoes down while you sing")
{
    auto level = [] (float duck)
    {
        auto p = dryEcho(); p.duck = duck; p.feedback = 0.3f;
        Echo e; e.setParams (p); e.prepare (kSr, kBlock);
        auto s = sine (48000, 500.0f, 0.5f);
        run (e, s);
        return rms (s.l, 24000, 48000);
    };
    REQUIRE (gainToDb (level (1.0f) / level (0.0f)) <= -12.0f);
}

TEST_CASE ("Echo: Freeze holds the echoes at a steady level")
{
    auto p = dryEcho(); p.timeMs = 300.0f; p.feedback = 0.5f; p.style.wobDepth = 0.5f;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    const int n = (int) kSr * 11;
    Stereo s ((int) (kSr * 0.6));
    for (int i = 0; i < (int) kSr / 2; ++i) s.l[(size_t) i] = s.r[(size_t) i] = 0.3f * std::sin ((float) i * 0.05f) * std::sin ((float) i * 0.0013f);
    run (e, s); // fill
    p.freeze = true; e.setParams (p);
    Stereo t (n);
    run (e, t);
    const float early = rms (t.l, (size_t) kSr, (size_t) kSr * 2), late = rms (t.l, (size_t) kSr * 9, (size_t) kSr * 10);
    REQUIRE (early > 1.0e-3f);
    REQUIRE (std::fabs (gainToDb (late / early)) < 3.0f);
}

TEST_CASE ("Echo: feedback above 100 % stays bounded in every mode and saturation type")
{
    for (int mode = 0; mode < Echo::numModes; ++mode)
        for (int sat = 0; sat < Echo::numSatTypes; ++sat)
        {
            auto p = dryEcho(); p.mode = mode; p.feedback = 1.1f; p.saturation = 0.5f; p.timeMs = 50.0f;
            p.style.satType = sat; p.style.wobDepth = 1.0f; p.style.diffAmount = 1.0f; p.style.diffLoop = true; p.groove = 0.5f;
            Echo e; e.setParams (p); e.prepare (kSr, kBlock);
            auto s = sine ((int) kSr * 4, 300.0f, 0.5f);
            run (e, s);
            REQUIRE (allFinite (s));
            REQUIRE (peak (s.l) < 4.0f);
        }
}

TEST_CASE ("Echo: one minute at 110 % feedback stays below +12 dBFS")
{
    auto p = dryEcho(); p.feedback = 1.1f; p.saturation = 0.3f; p.timeMs = 230.0f;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    auto s = sine ((int) kSr * 60, 220.0f, 0.5f);
    run (e, s);
    REQUIRE (allFinite (s));
    REQUIRE (peak (s.l) < dbToGain (12.0f));
}

TEST_CASE ("Echo: long times at 192 kHz stay in range")
{
    auto p = dryEcho(); p.timeMs = 2500.0f; p.mode = Echo::single; p.lrOffsetMs = 25.0f; p.width = 1.0f; p.feedback = 0.9f;
    Echo e; e.setParams (p); e.prepare (192000.0, 512);
    std::vector<float> l (192000 * 3, 0.0f), r (l.size(), 0.0f);
    l[0] = r[0] = 1.0f;
    for (size_t i = 0; i < l.size(); i += 512) e.process (l.data() + i, r.data() + i, (int) std::min<size_t> (512, l.size() - i));
    for (float v : l) REQUIRE (std::isfinite (v));
}

// ---------------------------------------------------------------- Echo deep engines
namespace
{
Echo::Params engineOnly (int styleIndex)
{
    auto p = dryEcho();
    p.styleIndex = styleIndex;
    return p;
}

/** dB of rms (high-passed at 4 kHz) / rms in [from, to). */
float hfShareDb (const std::vector<float>& v, size_t from, size_t to)
{
    Biquad f; f.set (Biquad::Type::highPass, 4000.0f, 0.707f, 0.0f, kSr);
    std::vector<float> h (v.size());
    for (size_t i = 0; i < v.size(); ++i) h[i] = f.process (v[i]);
    return gainToDb (rms (h, from, to) / std::max (1.0e-9f, rms (v, from, to)));
}

float lfShareDb (const std::vector<float>& v, size_t from, size_t to)
{
    Biquad f; f.set (Biquad::Type::lowPass, 250.0f, 0.707f, 0.0f, kSr);
    std::vector<float> h (v.size());
    for (size_t i = 0; i < v.size(); ++i) h[i] = f.process (v[i]);
    return gainToDb (rms (h, from, to) / std::max (1.0e-9f, rms (v, from, to)));
}

struct EngineSig { float hfDb, lfDb, corr; };

EngineSig echoSignature (int styleIndex)
{
    auto p = engineOnly (styleIndex); p.timeMs = 1000.0f;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    Stereo s ((int) (kSr * 2.5));
    NoiseSource n;
    for (int i = 0; i < (int) (kSr * 1.5); ++i) s.l[(size_t) i] = s.r[(size_t) i] = 0.25f * n.white();
    const std::vector<float> x = s.l;
    run (e, s);
    const size_t from = 52800, to = 96000;
    EngineSig sig { hfShareDb (s.l, from, to), lfShareDb (s.l, from, to), 0.0f };
    double best = 0.0;
    for (size_t lag = 47520; lag <= 48480; ++lag)
    {
        double xy = 0, xx = 0, yy = 0;
        for (size_t i = from; i < to; ++i) { const double a = x[i - lag], b = s.l[i]; xy += a * b; xx += a * a; yy += b * b; }
        best = std::max (best, std::fabs (xy) / std::sqrt (std::max (1.0e-30, xx * yy)));
    }
    sig.corr = (float) best;
    return sig;
}

/** HF share of the first echo of 1.5 s of white noise (window from delay + 100 ms, 0.9 s long). */
float firstEchoHfDb (int styleIndex, float ms)
{
    auto p = engineOnly (styleIndex); p.timeMs = ms;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    const size_t delay = (size_t) (ms * 0.001 * kSr);
    Stereo s ((int) (kSr * 1.5) + (int) delay + (int) kSr);
    NoiseSource n;
    for (int i = 0; i < (int) (kSr * 1.5); ++i) s.l[(size_t) i] = s.r[(size_t) i] = 0.25f * n.white();
    run (e, s);
    const size_t from = delay + (size_t) (0.1 * kSr);
    return hfShareDb (s.l, from, from + (size_t) (0.9 * kSr));
}

float styleBound (int style)
{
    const auto& st = Echo::styles()[(size_t) style].edit;
    return 4.0f * dbToGain (std::max ({ 0.0f, st.lowG, st.midG, st.highG }));
}
} // namespace

TEST_CASE ("Echo: each engine kind has its own sound")
{
    const int ids[6] = { 1, 6, 16, 11, 8, 15 }; // tape, bbd, digital, radio, spring, clean
    EngineSig sig[6];
    for (int i = 0; i < 6; ++i) sig[i] = echoSignature (ids[i]);
    for (int a = 0; a < 6; ++a)
        for (int b = a + 1; b < 6; ++b)
        {
            INFO ("styles " << ids[a] << " and " << ids[b] << ": hf " << sig[a].hfDb << "/" << sig[b].hfDb << " lf " << sig[a].lfDb << "/" << sig[b].lfDb
                  << " corr " << sig[a].corr << "/" << sig[b].corr);
            CHECK ((std::fabs (sig[a].hfDb - sig[b].hfDb) >= 1.5f || std::fabs (sig[a].lfDb - sig[b].lfDb) >= 1.5f
                    || std::fabs (sig[a].corr - sig[b].corr) >= 0.1f));
        }
}

TEST_CASE ("Echo: BBD echoes lose treble as the delay gets longer")
{
    const float hf50 = firstEchoHfDb (6, 50.0f), hf150 = firstEchoHfDb (6, 150.0f), hf500 = firstEchoHfDb (6, 500.0f);
    INFO (hf50 << " " << hf150 << " " << hf500);
    REQUIRE (hf50 - hf150 >= 3.0f);
    REQUIRE (hf150 - hf500 >= 10.0f);
}

TEST_CASE ("Echo: tape echoes get darker as the tape slows down")
{
    const float hf100 = firstEchoHfDb (1, 100.0f), hf1000 = firstEchoHfDb (1, 1000.0f);
    INFO (hf100 << " " << hf1000);
    REQUIRE (hf100 - hf1000 >= 4.0f);
}

TEST_CASE ("Echo: a cheap digital echo drops its sample rate on long delays")
{
    auto ratioDb = [] (float ms)
    {
        auto p = engineOnly (16); p.timeMs = ms;
        Echo e; e.setParams (p); e.prepare (kSr, kBlock);
        auto s = sine ((int) (kSr * 2), 1000.0f, 0.1f);
        run (e, s);
        return gainToDb (toneLevel (s.l, 15000.0f, (size_t) (kSr * 1.5)) / toneLevel (s.l, 1000.0f, (size_t) (kSr * 1.5)));
    };
    const float slow = ratioDb (1000.0f), fast = ratioDb (100.0f);
    INFO (slow << " " << fast);
    REQUIRE (slow > -35.0f);
    REQUIRE (fast < -60.0f);
}

TEST_CASE ("Echo: radio styles are band-limited")
{
    auto level = [] (float freq)
    {
        auto p = engineOnly (11); p.timeMs = 100.0f;
        Echo e; e.setParams (p); e.prepare (kSr, kBlock);
        auto s = sine ((int) kSr, freq, 0.1f);
        run (e, s);
        return gainToDb (toneLevel (s.l, freq, (size_t) (kSr * 0.5)));
    };
    const float g80 = level (80.0f), g1k = level (1000.0f), g9k = level (9000.0f);
    INFO (g80 << " " << g1k << " " << g9k);
    REQUIRE (g80 - g1k <= -20.0f);
    REQUIRE (g9k - g1k <= -20.0f);
}

TEST_CASE ("Echo: the oil can smear keeps the echo on time")
{
    auto p = engineOnly (8); p.timeMs = 100.0f;
    Echo e; e.setParams (p); e.prepare (kSr, kBlock);
    auto s = impulseThrough (e, 12000);
    size_t best = 0; float bv = 0.0f;
    for (size_t i = 0; i < s.l.size(); ++i) if (std::fabs (s.l[i]) > bv) { bv = std::fabs (s.l[i]); best = i; }
    INFO ("peak at " << best);
    REQUIRE ((int) best >= 4800 - 96);
    REQUIRE ((int) best <= 4800 + 96);
}

TEST_CASE ("Echo: every style stays finite and bounded at 110 % feedback in every mode")
{
    for (int style = 0; style < Echo::numStyles(); ++style)
        for (int mode = 0; mode < Echo::numModes; ++mode)
        {
            auto p = dryEcho(); Echo::applyStyle (p, style);
            p.mode = mode; p.feedback = 1.1f; p.saturation = 0.5f; p.timeMs = 50.0f; p.groove = 0.5f;
            Echo e; e.setParams (p); e.prepare (kSr, kBlock);
            auto s = sine ((int) kSr * 4, 300.0f, 0.5f);
            run (e, s);
            INFO ("style " << style << " mode " << mode);
            REQUIRE (allFinite (s));
            REQUIRE (peak (s.l) < styleBound (style));
        }
}

TEST_CASE ("Echo: one minute at 110 % feedback through each engine stays below +12 dBFS")
{
    for (int style : { 1, 6, 16, 11, 8 })
    {
        auto p = engineOnly (style); p.feedback = 1.1f; p.saturation = 0.3f; p.timeMs = 230.0f;
        Echo e; e.setParams (p); e.prepare (kSr, kBlock);
        auto s = sine ((int) kSr * 60, 220.0f, 0.5f);
        run (e, s);
        INFO ("style " << style);
        REQUIRE (allFinite (s));
        REQUIRE (peak (s.l) < dbToGain (12.0f));
    }
}

TEST_CASE ("Echo: every style processes 1 s of 48 kHz stereo in under 250 ms")
{
    double worst = 0.0;
    for (int style = 0; style < Echo::numStyles(); ++style)
    {
        auto p = dryEcho(); Echo::applyStyle (p, style);
        p.mode = Echo::pingPong; p.feedback = 0.9f; p.saturation = 0.5f;
        p.style.diffAmount = 0.5f; p.style.diffLoop = true; p.style.wobDepth = 0.5f;
        Echo e; e.setParams (p); e.prepare (kSr, kBlock);
        auto s = sine (48000, 220.0f, 0.5f);
        const auto t0 = std::chrono::steady_clock::now();
        run (e, s);
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
        worst = std::max (worst, ms);
        INFO ("style " << style << ": " << ms << " ms");
        UNSCOPED_INFO ("Echo CPU style " << style << ": " << ms << " ms");
        REQUIRE (ms < 250.0);
    }
    UNSCOPED_INFO ("Echo CPU worst: " << worst << " ms");
}

TEST_CASE ("Echo: every style is silent, DC-free, finite and lets go at 44.1 and 96 kHz")
{
    for (double rate : { 44100.0, 96000.0 })
        for (int style = 0; style < Echo::numStyles(); ++style)
        {
            auto p = dryEcho(); Echo::applyStyle (p, style);
            p.mode = Echo::single; p.timeMs = 100.0f; p.feedback = 0.0f; p.saturation = 0.5f;
            Echo e; e.setParams (p); e.prepare (rate, kBlock);
            const int n = (int) rate;
            auto runAt = [&] (Stereo& s) { for (size_t i = 0; i < s.l.size(); i += kBlock) { const int m = (int) std::min ((size_t) kBlock, s.l.size() - i); e.process (s.l.data() + i, s.r.data() + i, m); } };
            INFO ("style " << style << " rate " << rate);

            Stereo quiet (n); runAt (quiet);
            REQUIRE (peak (quiet.l) < 1.0e-6f);

            Stereo sq (n);
            for (int i = 0; i < n; ++i) sq.l[(size_t) i] = sq.r[(size_t) i] = ((i / 50) % 2 == 0) ? 1.0f : -1.0f;
            runAt (sq);
            REQUIRE (allFinite (sq));
            REQUIRE (peak (sq.l) < styleBound (style));

            Stereo dc (2 * n);
            for (auto& v : dc.l) v = 0.5f;
            for (auto& v : dc.r) v = 0.5f;
            runAt (dc);
            REQUIRE (allFinite (dc));
            double mean = 0; for (int i = n; i < 2 * n; ++i) mean += dc.l[(size_t) i];
            REQUIRE (std::fabs (mean / n) < 1.0e-3);

            Stereo tail (n); runAt (tail);
            REQUIRE (peak (tail.l, (size_t) (n / 2)) < 1.0e-6f);
        }
}

TEST_CASE ("Echo: a tempo change mid-delay glides without clicks on every engine")
{
    for (int style : { 1, 6, 16, 11, 8, 15 })
    {
        auto p = engineOnly (style); p.timeMode = Echo::timeNote; p.note = Echo::noteQuarter; p.feedback = 0.6f;
        Echo e; e.setParams (p); e.prepare (kSr, kBlock);
        e.setTempo (120.0);
        auto s = sine ((int) kSr * 3, 440.0f, 0.3f);
        for (int seg = 0; seg < 3; ++seg)
        {
            if (seg == 1) e.setTempo (90.0);
            if (seg == 2) e.setTempo (160.0);
            for (size_t i = (size_t) seg * 48000; i < (size_t) (seg + 1) * 48000; i += kBlock)
                e.process (s.l.data() + i, s.r.data() + i, (int) std::min<size_t> (kBlock, (size_t) (seg + 1) * 48000 - i));
        }
        float maxStep = 0.0f;
        for (size_t i = 1; i < s.l.size(); ++i) maxStep = std::max (maxStep, std::fabs (s.l[i] - s.l[i - 1]));
        INFO ("style " << style << " max step " << maxStep);
        REQUIRE (allFinite (s));
        REQUIRE (peak (s.l) < 4.0f);
        REQUIRE (maxStep < 0.25f);
    }
}


namespace
{
Plate::Params wetPlate()
{
    Plate::Params p;
    p.model = 0; p.preamp = Plate::clean; p.decaySec = 2.0f; p.preDelayMs = 0.0f; p.modDepth = 0.0f; p.lowHz = 20.0f; p.highHz = 20000.0f;
    p.width = 1.0f; p.balance = 0.0f; p.mix = 1.0f; p.inDb = 0.0f; p.outDb = 0.0f; p.duck = 0.0f; p.freeze = false;
    p.eq1G = 0.0f; p.eq2G = 0.0f;
    return p;
}

/** Decay time (RT60) of an impulse response, measured in the 500 Hz band from the -5 to -25 dB slope. */
float measureRt60 (Plate::Params p, double sr = kSr, float bandHz = 500.0f)
{
    Plate pl; pl.setParams (p); pl.prepare (sr, kBlock);
    const int n = (int) (sr * std::max (4.0f, p.decaySec * 1.6f));
    std::vector<float> l ((size_t) n, 0.0f), r ((size_t) n, 0.0f);
    l[0] = r[0] = 1.0f;
    for (int i = 0; i < n; i += kBlock) pl.process (l.data() + i, r.data() + i, std::min (kBlock, n - i));
    Biquad bp; bp.set (Biquad::Type::bandPass, bandHz, 1.4f, 0.0f, sr);
    const int win = (int) (0.05 * sr);
    std::vector<float> db;
    for (int i = 0; i + win <= n; i += win)
    {
        double e = 0;
        for (int k = 0; k < win; ++k) { const float v = bp.process (l[(size_t) (i + k)]); e += (double) v * v; }
        db.push_back (10.0f * std::log10 ((float) (e / win) + 1.0e-20f));
    }
    const float top = *std::max_element (db.begin(), db.end());
    int i5 = -1, i25 = -1;
    for (int i = 0; i < (int) db.size(); ++i)
    {
        if (i5 < 0 && db[(size_t) i] <= top - 5.0f && i > 0) i5 = i;
        if (i5 >= 0 && db[(size_t) i] <= top - 25.0f) { i25 = i; break; }
    }
    if (i5 < 0 || i25 < 0) return 1.0e9f; // never got 25 dB down: very long
    const float seconds = (float) (i25 - i5) * 0.05f;
    return 60.0f * seconds / 20.0f;
}

Stereo squareFs (int n, double sr)
{
    (void) sr;
    Stereo s (n);
    for (int i = 0; i < n; ++i) s.l[(size_t) i] = s.r[(size_t) i] = ((i / 50) % 2 == 0) ? 1.0f : -1.0f;
    return s;
}

/** Share of a band's energy that falls in the first `sec` seconds. */
float bandShareFirst (const std::vector<float>& v, float hz, double sr, double sec)
{
    Biquad bp; bp.set (Biquad::Type::bandPass, hz, 1.4f, 0.0f, sr);
    const size_t first = (size_t) (sec * sr);
    double e1 = 0.0, tot = 0.0;
    for (size_t i = 0; i < v.size(); ++i)
    {
        const double y = bp.process (v[i]);
        tot += y * y;
        if (i < first) e1 += y * y;
    }
    return (float) (e1 / std::max (tot, 1.0e-30));
}

float plateThd (int preamp)
{
    auto p = wetPlate(); p.preamp = preamp; p.decaySec = 0.6f;
    Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
    auto s = sine ((int) kSr * 3, 200.0f, 0.5f);
    run (pl, s);
    const float f1 = toneLevel (s.l, 200.0f, (size_t) kSr * 2);
    double sum = 0;
    for (int k = 2; k <= 6; ++k) { const float h = toneLevel (s.l, 200.0f * (float) k, (size_t) kSr * 2); sum += (double) h * h; }
    return (float) std::sqrt (sum) / f1;
}
} // namespace

TEST_CASE ("Plate: the Decay setting gives that decay time (within 20 %) on every plate model")
{
    for (int model = 0; model < Plate::numModels; ++model)
    {
        auto p = wetPlate(); p.model = model;
        const float rt = measureRt60 (p);
        INFO ("model " << model << " RT60 " << rt);
        REQUIRE (rt == Approx (2.0f).epsilon (0.2));
    }
}

TEST_CASE ("Plate: a 6 second decay measures about 6 seconds")
{
    auto p = wetPlate(); p.decaySec = 6.0f;
    REQUIRE (measureRt60 (p) == Approx (6.0f).epsilon (0.2));
}

TEST_CASE ("Plate: infinite decay holds the sound for 10 seconds")
{
    auto p = wetPlate(); p.decaySec = Plate::kInfinite; p.modDepth = 0.5f;
    Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
    Stereo s ((int) kSr * 12);
    for (int i = 0; i < (int) (kSr * 0.2); ++i) s.l[(size_t) i] = s.r[(size_t) i] = 0.3f * std::sin ((float) i * 0.07f);
    run (pl, s);
    const float early = rms (s.l, (size_t) kSr, (size_t) kSr * 2), late = rms (s.l, (size_t) kSr * 10, (size_t) kSr * 11);
    REQUIRE (early > 1.0e-3f);
    REQUIRE (std::fabs (gainToDb (late / early)) < 3.0f);
}

TEST_CASE ("Plate: Clean preamp is clean, Tube preamp adds harmonics")
{
    REQUIRE (plateThd (Plate::clean) < 0.001f);
    REQUIRE (plateThd (Plate::tube) > 0.005f);
    REQUIRE (plateThd (Plate::solid) > 0.001f);
}

TEST_CASE ("Plate: nothing comes out before the pre-delay")
{
    auto p = wetPlate(); p.preDelayMs = 100.0f;
    Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
    Stereo s (12000);
    s.l[0] = s.r[0] = 1.0f;
    run (pl, s);
    REQUIRE (peak (s.l, 0, 4700) < 1.0e-4f);
    REQUIRE (peak (s.l, 4800, 12000) > 1.0e-3f);
}

TEST_CASE ("Plate: Width 0 is mono, Width 1 is stereo")
{
    auto p = wetPlate(); p.width = 0.0f;
    Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
    Stereo s (48000);
    s.l[0] = s.r[0] = 1.0f;
    run (pl, s);
    for (size_t i = 0; i < s.l.size(); ++i) REQUIRE (s.l[i] == Approx (s.r[i]).margin (1.0e-6));
    p.width = 1.0f;
    Plate st; st.setParams (p); st.prepare (kSr, kBlock);
    Stereo t (48000);
    t.l[0] = t.r[0] = 1.0f;
    run (st, t);
    double diff = 0.0;
    for (size_t i = 0; i < t.l.size(); ++i) diff += std::fabs (t.l[i] - t.r[i]);
    REQUIRE (diff > 1.0);
}

TEST_CASE ("Plate: Duck turns the reverb down while you sing")
{
    auto level = [] (float duck)
    {
        auto p = wetPlate(); p.duck = duck;
        Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
        auto s = sine ((int) kSr * 2, 400.0f, 0.5f);
        run (pl, s);
        return rms (s.l, (size_t) kSr, (size_t) kSr * 2);
    };
    REQUIRE (gainToDb (level (1.0f) / level (0.0f)) <= -10.0f);
}

TEST_CASE ("Plate: Freeze holds the reverb after the input stops")
{
    auto p = wetPlate(); p.decaySec = 1.0f;
    Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
    auto fill = sine ((int) (kSr * 0.5), 300.0f, 0.3f);
    run (pl, fill);
    p.freeze = true; pl.setParams (p);
    Stereo s ((int) kSr * 11);
    run (pl, s);
    const float early = rms (s.l, (size_t) kSr, (size_t) kSr * 2), late = rms (s.l, (size_t) kSr * 9, (size_t) kSr * 10);
    REQUIRE (early > 1.0e-3f);
    REQUIRE (std::fabs (gainToDb (late / early)) < 3.0f);
}

TEST_CASE ("Plate: infinite decay with loud input for 10 seconds at 192 kHz stays bounded")
{
    auto p = wetPlate(); p.decaySec = Plate::kInfinite; p.modDepth = 1.0f; p.preamp = Plate::tube; p.inDb = 12.0f;
    Plate pl; pl.setParams (p); pl.prepare (192000.0, 512);
    std::vector<float> l (192000 * 10), r (l.size());
    for (size_t i = 0; i < l.size(); ++i) l[i] = r[i] = 0.5f * std::sin ((float) i * 0.013f);
    for (size_t i = 0; i < l.size(); i += 512) pl.process (l.data() + i, r.data() + i, (int) std::min<size_t> (512, l.size() - i));
    float pk = 0.0f;
    for (float v : l) { REQUIRE (std::isfinite (v)); pk = std::max (pk, std::fabs (v)); }
    REQUIRE (pk < dbToGain (12.0f));
}

TEST_CASE ("Plate: dispersion makes the lows of an impulse arrive later")
{
    auto share = [] (int sections, double sr)
    {
        Plate::Dispersion d; d.prepare (sr, Plate::model (0).dispHz, sections);
        std::vector<float> v ((size_t) (sr * 0.2));
        for (size_t i = 0; i < v.size(); ++i) v[i] = d.process (i == 0 ? 1.0f : 0.0f);
        return bandShareFirst (v, 300.0f, sr, 0.005);
    };
    REQUIRE (share (0, kSr) > 0.95f);
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        const float sh = share (Plate::kDispSections, sr);
        INFO ("sr " << sr << " share " << sh);
        REQUIRE (sh < 0.8f);
    }
}

TEST_CASE ("Plate: each band's per-pass gain follows its decay time")
{
    for (int m = 0; m < Plate::numModels; ++m)
    {
        const auto& md = Plate::model (m);
        const auto g = Plate::bandGains (m, 0.5f, 0.05f);
        REQUIRE (g.mid == Approx (std::pow (10.0f, -0.075f)).epsilon (1e-5));
        REQUIRE (g.low == Approx (std::pow (10.0f, -0.075f / md.lowX)).epsilon (1e-5));
        REQUIRE (g.high == Approx (std::pow (10.0f, -0.075f / md.highX)).epsilon (1e-5));
        REQUIRE (g.low > g.high);
        const auto e = Plate::bandGains (m, 0.0f, 0.05f);
        REQUIRE (e.low == 1.0f); REQUIRE (e.mid == 1.0f); REQUIRE (e.high == 1.0f);
    }
}

TEST_CASE ("Plate: lows ring longer than highs on every model")
{
    for (int m = 0; m < Plate::numModels; ++m)
    {
        auto p = wetPlate(); p.model = m; p.decaySec = 4.0f;
        const float rt125 = measureRt60 (p, kSr, 125.0f), rt8k = measureRt60 (p, kSr, 8000.0f), rt500 = measureRt60 (p, kSr, 500.0f);
        INFO ("model " << m << " rt125 " << rt125 << " rt500 " << rt500 << " rt8k " << rt8k);
        REQUIRE (rt125 > 1.15f * rt8k);
        REQUIRE (rt8k < 0.9f * rt500);
    }
}

TEST_CASE ("Plate: every model hits 4 s and 6 s decays within 10 %")
{
    for (int m = 0; m < Plate::numModels; ++m)
        for (float d : { 4.0f, 6.0f })
        {
            auto p = wetPlate(); p.model = m; p.decaySec = d;
            const float rt = measureRt60 (p);
            INFO ("model " << m << " decay " << d << " RT60 " << rt);
            REQUIRE (rt == Approx (d).epsilon (0.1));
        }
}

TEST_CASE ("Plate: 60 s decay with full-scale input stays finite and bounded on every model")
{
    for (int m = 0; m < Plate::numModels; ++m)
        for (int pre = 0; pre < Plate::numPreamps; ++pre)
        {
            auto p = wetPlate(); p.model = m; p.preamp = pre; p.decaySec = 59.0f; p.modDepth = 1.0f; p.inDb = 24.0f;
            Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
            for (float d : { 59.0f, Plate::kInfinite })
            {
                p.decaySec = d; pl.setParams (p);
                auto s = squareFs ((int) kSr * 10, kSr);
                run (pl, s);
                INFO ("model " << m << " preamp " << pre << " decay " << d);
                REQUIRE (allFinite (s));
                REQUIRE (peak (s.l) < 4.0f); REQUIRE (peak (s.r) < 4.0f);
            }
        }
}

TEST_CASE ("Plate: every model processes 1 s of 48 kHz stereo in under 250 ms")
{
    for (int m = 0; m < Plate::numModels; ++m)
    {
        auto p = wetPlate(); p.model = m; p.preamp = Plate::tube; p.decaySec = 5.0f; p.modDepth = 1.0f; p.preDelayMs = 40.0f;
        p.highHz = 8000.0f; p.eq1G = 3.0f; p.eq2G = -3.0f; p.duck = 0.5f;
        Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
        auto s = sine ((int) kSr, 220.0f, 0.5f);
        const auto t0 = std::chrono::steady_clock::now();
        run (pl, s);
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
        UNSCOPED_INFO ("Plate model " << m << ": " << ms << " ms per 1 s");
        REQUIRE (ms < 250.0);
    }
}

TEST_CASE ("Plate: every model is silent, DC-free, finite and lets go at 44.1 and 96 kHz")
{
    for (double sr : { 44100.0, 96000.0 })
        for (int m = 0; m < Plate::numModels; ++m)
        {
            auto p = wetPlate(); p.model = m; p.preamp = Plate::tube; p.modDepth = 0.5f; p.decaySec = 2.0f;
            Plate pl; pl.setParams (p); pl.prepare (sr, kBlock);
            const int one = (int) sr;
            auto runN = [&] (Stereo& s)
            {
                for (size_t i = 0; i < s.l.size(); i += kBlock)
                    pl.process (s.l.data() + i, s.r.data() + i, (int) std::min ((size_t) kBlock, s.l.size() - i));
            };
            INFO ("sr " << sr << " model " << m);
            Stereo sil (one); runN (sil);
            REQUIRE (peak (sil.l) < 1.0e-6f); REQUIRE (peak (sil.r) < 1.0e-6f);
            auto sq = squareFs (one, sr); runN (sq);
            REQUIRE (allFinite (sq)); REQUIRE (peak (sq.l) < 4.0f);
            Stereo dc (3 * one);
            std::fill (dc.l.begin(), dc.l.end(), 0.5f); dc.r = dc.l;
            runN (dc);
            double mean = 0.0;
            for (size_t i = (size_t) (2 * one); i < dc.l.size(); ++i) mean += dc.l[i];
            REQUIRE (std::fabs (mean / one) < 1.0e-3);
            Stereo tail (8 * one); runN (tail);
            REQUIRE (peak (tail.l, (size_t) (6 * one)) < 1.0e-6f);
            REQUIRE (peak (tail.r, (size_t) (6 * one)) < 1.0e-6f);
        }
}

TEST_CASE ("Plate: freeze on then off holds and then lets the tail go")
{
    auto p = wetPlate(); p.decaySec = 1.0f;
    Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
    auto fill = sine ((int) (kSr * 0.5), 300.0f, 0.3f);
    run (pl, fill);
    p.freeze = true; pl.setParams (p);
    Stereo held ((int) kSr * 3);
    run (pl, held);
    const float a = rms (held.l, (size_t) (kSr * 0.5), (size_t) (kSr * 1.5)), b = rms (held.l, (size_t) kSr * 2, (size_t) kSr * 3);
    REQUIRE (a > 1.0e-3f);
    REQUIRE (std::fabs (gainToDb (b / a)) < 3.0f);
    p.freeze = false; pl.setParams (p);
    Stereo after ((int) kSr * 3);
    run (pl, after);
    REQUIRE (allFinite (after));
    REQUIRE (rms (after.l, (size_t) (kSr * 2.5), (size_t) (kSr * 3)) < 1.0e-5f);
}

TEST_CASE ("Plate: changing Decay mid-tail glides and takes effect")
{
    auto p = wetPlate(); p.decaySec = 6.0f;
    Plate pl; pl.setParams (p); pl.prepare (kSr, kBlock);
    Stereo s ((int) kSr * 3);
    for (int i = 0; i < (int) (kSr * 0.1); ++i) s.l[(size_t) i] = s.r[(size_t) i] = 0.5f * std::sin (0.05f * (float) i);
    for (size_t i = 0; i < s.l.size(); i += kBlock)
    {
        if (i >= (size_t) kSr && p.decaySec == 6.0f) { p.decaySec = 0.5f; pl.setParams (p); }
        if (i >= (size_t) kSr * 2 && p.decaySec < Plate::kInfinite) { p.decaySec = Plate::kInfinite; pl.setParams (p); }
        pl.process (s.l.data() + i, s.r.data() + i, (int) std::min ((size_t) kBlock, s.l.size() - i));
    }
    REQUIRE (allFinite (s));
    float maxStep = 0.0f;
    for (size_t i = 1; i < s.l.size(); ++i) maxStep = std::max (maxStep, std::fabs (s.l[i] - s.l[i - 1]));
    REQUIRE (maxStep < 0.25f);
    const float r1 = rms (s.l, (size_t) (kSr * 1.9), (size_t) (kSr * 2.0)), r0 = rms (s.l, (size_t) (kSr * 0.9), (size_t) kSr);
    REQUIRE (gainToDb (r1 / r0) < -40.0f);
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

TEST_CASE ("Review fix: resetting a long delay line is instant and silences old sound")
{
    DelayLine d;
    d.allocate (1 << 21);
    for (int i = 0; i < 1000; ++i) d.push (1.0f);
    const auto t0 = std::chrono::steady_clock::now();
    for (int k = 0; k < 100; ++k) d.reset();
    const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
    REQUIRE (ms < 5.0);
    REQUIRE (d.read (10) == 0.0f);
    REQUIRE (d.readFrac (500.5f) == 0.0f);
    d.push (0.5f);
    REQUIRE (d.read (1) == 0.5f);
    REQUIRE (d.read (2) == 0.0f);
}

TEST_CASE ("Review fix: Prime Numbers in Rhythm mode at 192 kHz is cheap")
{
    auto p = dryEcho(); p.mode = Echo::rhythm; p.prime = true; p.rhythmBeats = 8; p.grid = 2; p.repeats = 16; p.feedback = 0.5f;
    Echo e; e.setParams (p); e.prepare (192000.0, 512); e.setTempo (120.0);
    std::vector<float> l (192000), r (192000);
    l[0] = r[0] = 1.0f;
    const auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < l.size(); i += 512) e.process (l.data() + i, r.data() + i, 512);
    const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count();
    REQUIRE (ms < 400.0); // one second of audio must take well under a second
}
