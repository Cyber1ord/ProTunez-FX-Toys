// ProTunez FX Toys — "Echo" (like EchoBoy): 30 echo styles, four modes, groove, rhythm taps, ducking, freeze.
#pragma once

#include "Analog.h"
#include "Common.h"
#include <array>
#include <cstring>
#include <random>
#include <vector>

namespace ptfx
{
/** Up to 16 echo taps placed in beats (Rhythm mode). */
struct RhythmPattern
{
    struct Tap { float pos = 0.0f; float level = 1.0f; };
    std::array<Tap, 16> taps {};
    int count = 0;
};

class Echo : public Effect
{
public:
    enum Mode { single = 0, dual, pingPong, rhythm, numModes };
    enum TimeMode { timeMs = 0, timeNote, timeDot, timeTrip, numTimeModes };
    enum Note { note64 = 0, note32, note16, note8, noteQuarter, noteHalf, noteBar, note2Bars, numNotes };
    enum Shape { shapeDecay = 0, shapeReverse, shapeSwell, shapeFade, shapeNonlin, shapeFlat, numShapes };
    enum Pan { panDouble = 0, panCenter, panAlt1, panAlt2, panAlt3, panSweepL, panSweepR, panPan, numPans };
    enum Wobble { wobTriangle = 0, wobSine, wobSquare, wobRandomWalk, wobRandomSH, numWobbleShapes };
    static constexpr int numSatTypes = 9; // Clean, Tape, Warm, Pump, Dirt, Hard Limit, Soft Limit, Warm Limit, Bright Limit
    static constexpr int numGrids = 6;    // 1/2 .. 1/64 note
    static constexpr float kMaxSeconds = 6.0f;

    static constexpr float noteBeats[numNotes] = { 1.0f / 16, 1.0f / 8, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };
    static constexpr float gridBeats[numGrids] = { 2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };

    /** The "Style Edit" panel: the tone, saturation, diffusion and wobble of the echoes. */
    struct StyleEdit
    {
        float bandLowHz = 20.0f, bandHighHz = 20000.0f; // the style's own frequency band
        float lowF = 150.0f, lowG = 0.0f, lowDecay = 0.0f;
        float midF = 1000.0f, midG = 0.0f, midDecay = 0.0f, midRes = 0.3f;
        float highF = 5000.0f, highG = 0.0f, highDecay = 0.0f;
        int   satType = 1;
        float diffAmount = 0.0f, diffSize = 0.5f;
        bool  diffLoop = false;
        float wobDepth = 0.0f, wobRate = 0.8f;
        int   wobShape = wobSine;
        bool  wobSync = false;
        static StyleEdit clean() { StyleEdit s; s.satType = 0; return s; }
    };

    struct Params
    {
        int   mode = single;
        int   timeMode = timeNote;
        float timeMs = 375.0f;  // 1..2500
        int   note = note8;
        float time2Ms = 500.0f; // Dual mode, second echo
        int   note2 = noteQuarter;
        bool  prime = false;
        float feedback = 0.35f;   // 0..1.1
        float saturation = 0.2f;  // 0..1
        float lowCutHz = 20.0f, highCutHz = 20000.0f;
        float mix = 0.25f;
        float inDb = 0.0f, outDb = 0.0f;
        float groove = 0.0f;      // -1 shuffle .. +1 swing
        float feel = 0.0f;        // -1 dragging .. +1 rushing (±25 ms)
        float accent = 0.0f, accent2 = 0.0f; // -1..1
        float width = 1.0f;       // 0..1
        float lrOffsetMs = 8.0f;  // 0..25
        float fbMix = 0.0f;       // Dual: 0 = each feeds itself, 1 = each feeds the other
        float balance = 0.0f;     // -1 L .. +1 R
        float fbBalance = 0.0f;   // -1 L .. +1 R
        int   rhythmBeats = 4;    // 1..8
        int   grid = 3;           // index into gridBeats
        int   repeats = 4;        // 1..16
        int   shape = shapeDecay;
        float warp = 0.0f;        // -1..1 (Rhythm, clusters taps)
        int   panShape = panCenter;
        StyleEdit style;
        int   styleIndex = -1;    // which style's engine to run (-1 = none)
        float duck = 0.0f;        // 0..1
        bool  freeze = false;
    };

    // ------------------------------------------------------------------ helpers (tested)

    static int nearestPrime (int n)
    {
        auto isPrime = [] (int v) { if (v < 2) return false; for (int d = 2; d * d <= v; ++d) if (v % d == 0) return false; return true; };
        for (int k = 0; k < 1000; ++k)
        {
            if (isPrime (n - k)) return n - k;
            if (isPrime (n + k)) return n + k;
        }
        return n;
    }

    /** Level of tap i of n for a Shape (no allocation; safe on the audio thread). */
    static float shapeLevel (int shape, int i, int n)
    {
        const float t = n > 1 ? (float) i / (float) (n - 1) : 0.0f;
        switch (shape)
        {
            case shapeDecay:   return std::pow (0.25f, t);
            case shapeReverse: return std::pow (0.25f, 1.0f - t);
            case shapeSwell:   return 0.25f + 0.75f * std::sin (kPi * t);
            case shapeFade:    return 1.0f - 0.85f * t;
            case shapeNonlin:  return (i % 3 == 0) ? 1.0f : 0.45f;
            default:           return 1.0f;
        }
    }

    static std::vector<float> shapeLevels (int shape, int n)
    {
        std::vector<float> v ((size_t) std::max (1, n), 1.0f);
        for (int i = 0; i < n; ++i) v[(size_t) i] = shapeLevel (shape, i, n);
        return v;
    }

    /** What kind of machine a style models. The defaults of EngineDef mean "no colour". */
    enum class EngineKind { digital, tape, bbd, radio, spring };

    struct EngineDef
    {
        EngineKind kind = EngineKind::digital;
        // tape
        float tapeDrive = 1.0f, hystMix = 0.0f, gapHz = 20000.0f, bumpDb = 0.0f, wow = 0.0f, flutter = 0.0f;
        // bbd
        float bbdScale = 1.0f; bool compander = false;
        // digital (0 = off)
        int bits = 0; float memSamples = 0.0f;
        // radio
        float loHz = 20.0f, hiHz = 20000.0f, drive = 1.0f, asym = 0.0f, fadeDepth = 0.0f, fadeHz = 0.0f;
        // spring / chorus smear
        float smearAmt = 0.0f, smearSize = 1.0f, smearModMs = 0.0f, smearHz = 1.0f;
        // shared: noise floor in dB, <= -120 means off
        float noiseDb = -200.0f;
    };

    struct StyleDef { const char* name; StyleEdit edit; EngineDef engine; };
    static const std::vector<StyleDef>& styles();
    static int numStyles() { return (int) styles().size(); }
    static int styleIndex (const char* name)
    {
        for (int i = 0; i < numStyles(); ++i) if (std::strcmp (styles()[(size_t) i].name, name) == 0) return i;
        return -1;
    }
    static void applyStyle (Params& p, int index)
    {
        p.styleIndex = index;
        if (index >= 0 && index < numStyles()) p.style = styles()[(size_t) index].edit;
    }

    // ------------------------------------------------------------------ API

    void setParams (const Params& p)
    {
        params = p;
        mix.setTarget (clampv (p.mix, 0.0f, 1.0f));
        inGain.setTarget (dbToGain (clampv (p.inDb, -24.0f, 24.0f)));
        outGain.setTarget (dbToGain (clampv (p.outDb, -24.0f, 24.0f)));
        feedback.setTarget (p.freeze ? 1.0f : clampv (p.feedback, 0.0f, 1.1f));
        width.setTarget (clampv (p.width, 0.0f, 1.0f));
        if (p.styleIndex != engApplied) applyEngine();
        smearComp.setTarget (eng.kind == EngineKind::spring && ! p.freeze ? smearSumSamples() : 0.0f);
        updateTimes();
        updateFilters();
        buildTaps();
    }

    void setTempo (double newBpm) override
    {
        if (newBpm > 20.0 && newBpm < 400.0 && std::fabs (newBpm - bpm) > 0.001) { bpm = newBpm; updateTimes(); buildTaps(); }
    }

    void setPattern (const RhythmPattern& p) { custom = p; buildTaps(); }

    /** The echo time in ms for echo 0 (or 1 = Dual's second echo), after tempo sync. */
    float echoTimeMs (int which) const
    {
        const int tm = params.timeMode;
        if (tm == timeMs) return clampv (which == 0 ? params.timeMs : params.time2Ms, 1.0f, 2500.0f);
        float beats = noteBeats[clampv (which == 0 ? params.note : params.note2, 0, (int) numNotes - 1)];
        if (tm == timeDot) beats *= 1.5f;
        if (tm == timeTrip) beats *= 2.0f / 3.0f;
        return std::min (beats * (float) (60000.0 / bpm), kMaxSeconds * 1000.0f * 0.45f);
    }

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        const int maxSamples = (int) (sr * kMaxSeconds) + 64;
        for (auto& d : line) d.allocate (maxSamples);
        for (int c = 0; c < 2; ++c) for (auto& ap : diff[c]) ap.allocate ((int) (0.05 * sr) + 8);
        for (auto* s : { &mix, &inGain, &outGain, &feedback, &width, &duckGain }) s->prepare (sr, 30.0f);
        for (auto& t : timeSm) t.prepare (sr, 120.0f); // glide = tape-style pitch bend when the time changes
        smearComp.prepare (sr, 120.0f);
        for (int c = 0; c < 2; ++c)
        {
            for (auto& ap : spr[c]) ap.allocate ((int) (0.012 * sr) + 8);
            hyst[c].prepare (sr); bump[c].prepare (sr); bbd[c].prepare (sr); comp[c].prepare (sr);
        }
        envRel = (float) std::exp (-1.0 / (0.05 * sr));
        engApplied = -2;
        setParams (params);
        for (auto* s : { &mix, &inGain, &outGain, &feedback, &width }) s->snap (s->getTarget());
        duckGain.snap (1.0f);
        for (auto& t : timeSm) t.snap (t.getTarget());
        smearComp.snap (smearComp.getTarget());
        applyEngine();
        duckAtt = (float) std::exp (-1.0 / (0.005 * sr));
        duckRel = (float) std::exp (-1.0 / (0.25 * sr));
        reset();
    }

    void reset() override
    {
        for (auto& d : line) d.reset();
        for (int c = 0; c < 2; ++c)
        {
            for (auto& ap : diff[c]) ap.reset();
            for (auto* f : { &lc[c][0], &lc[c][1], &hc[c][0], &hc[c][1], &bandL[c][0], &bandL[c][1], &bandH[c][0], &bandH[c][1],
                             &dLow[c], &dMid[c], &dHigh[c], &oLow[c], &oMid[c], &oHigh[c] }) f->reset();
            dc[c].reset();
            pumpEnv[c] = 0.0f;
            postDiffOn[c] = false;
        }
        for (int c = 0; c < 2; ++c)
        {
            for (auto* f : { &preEmph[c], &deEmph[c], &gapLp[c], &rad[c][0], &rad[c][1], &rad[c][2], &rad[c][3] }) f->reset();
            for (auto& ap : spr[c]) ap.reset();
            hyst[c].reset(); bump[c].reset(); bbd[c].reset(); comp[c].reset();
            shHeld[c] = shAcc[c] = nzEnv[c] = 0.0f;
        }
        wowPh = 0.0f; wowRate = 1.0f; flutPh[0] = flutPh[1] = flutPh[2] = 0.0f; springPh = 0.0f; fadePh = 0.0f;
        duckEnv = 0.0f;
        wobPhase = 0.0f; walk[0] = walk[1] = 0.0f; sh[0] = sh[1] = shSmooth[0] = shSmooth[1] = 0.0f;
    }

    void process (float* left, float* right, int n) override
    {
        for (int i = 0; i < n; ++i)
        {
            if ((++engCtr & 31) == 0) updateEngineCoefs (false);
            const float ig = inGain.next(), og = outGain.next(), fb = feedback.next(), m = mix.next(), w = width.next();
            const float dryL = left[i], dryR = right[i];
            const float in = params.freeze ? 0.0f : 0.5f * (dryL + dryR) * ig;

            // Ducking: follow the input level and turn the echoes down while it is loud.
            const float lvl = std::max (std::fabs (dryL), std::fabs (dryR));
            duckEnv = lvl > duckEnv ? lvl + duckAtt * (duckEnv - lvl) : lvl + duckRel * (duckEnv - lvl);
            const float over = clampv ((gainToDb (duckEnv) + 40.0f) / 10.0f, 0.0f, 1.0f);
            duckGain.setTarget (1.0f - clampv (params.duck, 0.0f, 1.0f) * over * (1.0f - dbToGain (-18.0f)));
            const float dg = duckGain.next();

            float mod[2];
            wobble (mod);
            smearComp.next();
            const float fade = engineTick (mod);

            float eL = 0.0f, eR = 0.0f;
            switch (params.mode)
            {
                case dual:     processDual (in, fb, mod, eL, eR); break;
                case pingPong: processPingPong (in, fb, mod, eL, eR); break;
                case rhythm:   processRhythm (in, fb, mod, eL, eR); break;
                case single:
                default:       processSingle (in, fb, mod, eL, eR); break;
            }

            // Stereo width: Single widens with its L/R offset copy, the other modes narrow towards mono.
            if (params.mode == single) eR = eL + w * (eR - eL);
            else
            {
                const float mid = 0.5f * (eL + eR), side = 0.5f * (eL - eR) * w;
                eL = mid + side; eR = mid - side;
            }
            const float bal = clampv (params.balance, -1.0f, 1.0f);
            eL *= std::min (1.0f, 1.0f - bal); eR *= std::min (1.0f, 1.0f + bal);
            float e[2] = { eL, eR };
            for (int c = 0; c < 2; ++c)
            {
                e[c] = engineOut (c, e[c], fade);
                e[c] = oHigh[c].process (oMid[c].process (oLow[c].process (e[c])));
                if (! params.style.diffLoop && params.style.diffAmount > 0.001f) e[c] = diffuse (c, e[c]);
            }
            const float wetGain = og * dg;
            left[i]  = dryL * (1.0f - m) + e[0] * wetGain * m;
            right[i] = dryR * (1.0f - m) + e[1] * wetGain * m;
        }
    }

private:
    Params params;
    RhythmPattern custom;
    double sr = 44100.0, bpm = 120.0;
    DelayLine line[2];
    Smoothed mix, inGain, outGain, feedback, width, duckGain, timeSm[2];
    Biquad lc[2][2], hc[2][2], bandL[2][2], bandH[2][2], dLow[2], dMid[2], dHigh[2], oLow[2], oMid[2], oHigh[2];
    DCBlocker dc[2];
    DelayLine diff[2][4];
    bool postDiffOn[2] {};
    float pumpEnv[2] {};
    float duckEnv = 0.0f, duckAtt = 0.9f, duckRel = 0.999f;
    float wobPhase = 0.0f, walk[2] {}, sh[2] {}, shSmooth[2] {};
    std::minstd_rand rng { 12345 };
    struct LiveTap { float samples; float level; float panL, panR; };
    std::array<LiveTap, 16> taps {};
    int tapCount = 0;
    float loopSamples = 48000.0f;

    // ------------------------------------------------------------------ engine state
    EngineDef eng;
    int engApplied = -2, engCtr = 0;
    Biquad preEmph[2], deEmph[2], gapLp[2], rad[2][4];
    TapeHysteresis hyst[2];
    HeadBump bump[2];
    BbdColour bbd[2];
    Compander comp[2];
    DelayLine spr[2][4];
    Smoothed smearComp;
    NoiseSource nz[2], wowNoise;
    float shHeld[2] {}, shAcc[2] {}, fsRatio[2] { 1.0f, 1.0f }, nzEnv[2] {};
    float lastGap[2] {}, lastBump[2] {}, lastBbd[2] {}, lastFs[2] {};
    float wowPh = 0.0f, wowRate = 1.0f, flutPh[3] {}, springPh = 0.0f, fadePh = 0.0f;
    float springLen[4] { 2.0f, 2.0f, 2.0f, 2.0f };
    float noiseLin = 0.0f, envRel = 0.999f;

    // ------------------------------------------------------------------ building blocks

    void updateTimes()
    {
        const float T = echoTimeMs (0) * 0.001f * (float) sr, T2 = echoTimeMs (1) * 0.001f * (float) sr;
        timeSm[0].setTarget (T);
        timeSm[1].setTarget (T2);
        const float g = clampv (params.groove, -1.0f, 1.0f), fs = feelSamples();
        const float tA = T * (1.0f - g / 3.0f);
        ratio[0] = ratioFor (tA - fs);          // Single, first tap
        ratio[1] = ratioFor (2.0f * T - fs);    // Single, second tap
        ratio[2] = ratioFor (T - fs);           // Dual, echo 1
        ratio[3] = ratioFor (T2 - fs);          // Dual, echo 2
        ratio[4] = ratioFor (tA - fs);          // Ping-Pong, left
        ratio[5] = ratioFor (2.0f * T - tA);    // Ping-Pong, right
    }

    float maxDelay() const { return (float) line[0].capacity() - 8.0f; }

    /** Delay in samples for a base time with wobble applied. Feel and Prime Numbers are already in the base. */
    float delaySamples (float base, float mod) const { return clampv (base * (1.0f + mod) - smearComp.get(), 2.0f, maxDelay()); }

    float feelSamples() const { return clampv (params.feel, -1.0f, 1.0f) * 0.025f * (float) sr; }

    /** Moves a time to a prime number of samples when Prime Numbers is on (done when settings change, never per sample). */
    float primeSnap (float samples) const
    {
        if (! params.prime || params.freeze || samples < 2.0f) return samples;
        return (float) nearestPrime ((int) std::lround (samples));
    }

    /** How much Prime Numbers stretches each tap, as a ratio of its unsnapped length. */
    float ratio[6] { 1, 1, 1, 1, 1, 1 };
    float ratioFor (float base) const { return base > 2.0f ? primeSnap (base) / base : 1.0f; }

    void wobble (float* mod)
    {
        const auto& st = params.style;
        const float depth = params.freeze ? 0.0f : clampv (st.wobDepth, 0.0f, 1.0f) * 0.006f;
        wobPhase += clampv (st.wobRate, 0.05f, 10.0f) / (float) sr;
        bool wrapped = false;
        if (wobPhase >= 1.0f) { wobPhase -= 1.0f; wrapped = true; }
        std::uniform_real_distribution<float> uni (-1.0f, 1.0f);
        for (int c = 0; c < 2; ++c)
        {
            float ph = wobPhase + (st.wobSync || c == 0 ? 0.0f : 0.25f);
            if (ph >= 1.0f) ph -= 1.0f;
            float v;
            switch (st.wobShape)
            {
                case wobTriangle:   v = 4.0f * std::fabs (ph - 0.5f) - 1.0f; break;
                case wobSquare:     v = std::tanh (8.0f * std::sin (2.0f * kPi * ph)); break;
                case wobRandomWalk: walk[c] = clampv (walk[c] + uni (rng) * 0.002f * st.wobRate, -1.0f, 1.0f); v = walk[c]; break;
                case wobRandomSH:
                    if (wrapped || (c == 1 && ! st.wobSync && std::fabs (ph) < 1.0f / (float) sr * st.wobRate)) sh[c] = uni (rng);
                    shSmooth[c] += (sh[c] - shSmooth[c]) * 0.002f;
                    v = shSmooth[c];
                    break;
                case wobSine:
                default:            v = std::sin (2.0f * kPi * ph); break;
            }
            mod[c] = v * depth;
        }
    }

    /** Saturation types for the repeats. Small signals pass at unity. */
    float saturate (int c, float x) const
    {
        const float d = 1.0f + clampv (params.saturation, 0.0f, 1.0f) * 4.0f;
        const float v = x * d;
        float y;
        switch (params.style.satType)
        {
            case 0: return x;                                                        // Clean
            case 1: y = std::tanh (v); break;                                        // Tape
            case 2: y = v >= 0 ? std::tanh (v) : std::tanh (0.8f * v) / 0.8f; break; // Warm
            case 3: y = v / (1.0f + pumpEnv[c] * d * 2.0f); break;                   // Pump (compressing)
            case 4: y = clampv (v * 1.5f + 0.1f, -1.0f, 1.0f) / 1.5f - 0.1f / 1.5f; break; // Dirt
            case 5: y = clampv (v, -0.8f, 0.8f); break;                              // Hard Limit
            case 6: y = v / (1.0f + std::fabs (v)); break;                           // Soft Limit
            case 7: y = v >= 0 ? v / (1.0f + v) : v / (1.0f - 0.7f * v); break;     // Warm Limit
            case 8:                                                                  // Bright Limit
            default:
            {
                const float t = clampv (v, -1.5f, 1.5f);
                y = t - t * t * t / 6.75f;
                break;
            }
        }
        return y / d;
    }

    /** Never lets a recirculating signal run away: linear below 0.8, approaches 1.2. */
    static float safety (float x)
    {
        const float a = std::fabs (x);
        if (a <= 0.8f) return x;
        return std::copysign (0.8f + 0.4f * std::tanh ((a - 0.8f) / 0.4f), x);
    }

    float diffuse (int c, float x)
    {
        static constexpr int base[4] = { 142, 107, 379, 277 };
        const float g = 0.65f * clampv (params.style.diffAmount, 0.0f, 1.0f);
        const float scale = (0.3f + 1.2f * clampv (params.style.diffSize, 0.0f, 1.0f)) * (float) (sr / 48000.0);
        for (int k = 0; k < 4; ++k)
        {
            auto& d = diff[c][k];
            const int len = std::max (2, std::min ((int) (base[k] * scale), d.capacity() - 2));
            const float delayed = d.read (len);
            const float v = x + g * delayed;
            d.push (undenorm (v));
            x = delayed - g * v;
        }
        return x;
    }

    // ------------------------------------------------------------------ deep engines

    static constexpr float kSmearMs[4] = { 1.3f, 2.1f, 3.4f, 5.5f };

    float smearSumSamples() const
    {
        return (kSmearMs[0] + kSmearMs[1] + kSmearMs[2] + kSmearMs[3]) * clampv (eng.smearSize, 0.0f, 2.0f) * 0.001f * (float) sr;
    }

    /** Takes the engine of the chosen style. Only settings change here: the state of the blocks is left alone. */
    void applyEngine()
    {
        engApplied = params.styleIndex;
        eng = (engApplied >= 0 && engApplied < numStyles()) ? styles()[(size_t) engApplied].engine : EngineDef {};
        noiseLin = eng.noiseDb <= -120.0f ? 0.0f : dbToGain (eng.noiseDb);
        for (int c = 0; c < 2; ++c)
        {
            hyst[c].setDrive (eng.tapeDrive);
            bbd[c].setNoise (0.0f);
            preEmph[c].set (Biquad::Type::highShelf, 3000.0f, 0.7f, 6.0f, sr);
            deEmph[c].set (Biquad::Type::highShelf, 3000.0f, 0.7f, -6.0f, sr);
            for (int k = 0; k < 2; ++k)
            {
                rad[c][k].set (Biquad::Type::highPass, eng.loHz, 0.707f, 0.0f, sr);
                rad[c][k + 2].set (Biquad::Type::lowPass, eng.hiHz, 0.707f, 0.0f, sr);
            }
        }
        updateEngineCoefs (true);
    }

    float engineTSec (int c) const
    {
        float samples;
        if (params.mode == rhythm) samples = loopSamples;
        else if (params.mode == dual && c == 1) samples = timeSm[1].get();
        else samples = timeSm[0].get();
        return std::max (0.001f, samples / (float) sr);
    }

    /** Coefficients that follow the delay time: every 32 samples, and only when the value moved by more than 1 %. */
    void updateEngineCoefs (bool force)
    {
        auto moved = [force] (float now, float& last)
        {
            if (! force && std::fabs (now - last) <= 0.01f * std::fabs (last)) return false;
            last = now;
            return true;
        };
        for (int c = 0; c < 2; ++c)
        {
            const float t = engineTSec (c);
            switch (eng.kind)
            {
                case EngineKind::tape:
                {
                    const float speed = clampv (0.25f / t, 0.125f, 4.0f);
                    const float gap = clampv (eng.gapHz * speed, 2000.0f, 0.45f * (float) sr);
                    if (moved (gap, lastGap[c])) gapLp[c].set (Biquad::Type::lowPass, gap, 0.707f, 0.0f, sr);
                    const float hb = clampv (70.0f * speed, 25.0f, 250.0f);
                    if (moved (hb, lastBump[c])) bump[c].set (hb, eng.bumpDb);
                    break;
                }
                case EngineKind::bbd:
                {
                    const float d = t * eng.bbdScale;
                    if (moved (d, lastBbd[c])) bbd[c].setDelaySeconds (d);
                    break;
                }
                case EngineKind::digital:
                {
                    float r = 1.0f;
                    if (eng.memSamples > 0.0f) r = clampv (eng.memSamples / t, 4000.0f, (float) sr) / (float) sr;
                    if (moved (r, lastFs[c])) fsRatio[c] = r;
                    break;
                }
                default: break;
            }
        }
    }

    /** Per sample: tape wow/flutter on the read times, the smear and fade oscillators. Returns the radio fade gain. */
    float engineTick (float* mod)
    {
        auto adv = [] (float& ph, float inc) { ph += inc; if (ph >= 1.0f) ph -= 1.0f; };
        float fade = 1.0f;
        switch (eng.kind)
        {
            case EngineKind::tape:
                if (! params.freeze && (eng.wow > 0.0f || eng.flutter > 0.0f))
                {
                    wowRate = clampv (wowRate + 0.004f * std::sqrt (48000.0f / (float) sr) * wowNoise.white(), 0.5f, 1.5f);
                    adv (wowPh, wowRate / (float) sr);
                    const float fs = 1.0f / (float) sr;
                    adv (flutPh[0], 6.5f * fs); adv (flutPh[1], 9.2f * fs); adv (flutPh[2], 11.7f * fs);
                    const float w = 0.0015f * eng.wow * std::sin (2.0f * kPi * wowPh);
                    const float f = 0.0004f * eng.flutter * (0.5f * std::sin (2.0f * kPi * flutPh[0]) + 0.3f * std::sin (2.0f * kPi * flutPh[1])
                                                             + 0.2f * std::sin (2.0f * kPi * flutPh[2]));
                    mod[0] += w + f; mod[1] += w + f;
                }
                break;
            case EngineKind::spring:
                adv (springPh, clampv (eng.smearHz, 0.0f, 20.0f) / (float) sr);
                for (int k = 0; k < 4; ++k)
                {
                    const float ms = kSmearMs[k] * eng.smearSize + eng.smearModMs * std::sin (2.0f * kPi * (springPh + 0.25f * (float) k));
                    springLen[k] = clampv (ms * 0.001f * (float) sr, 1.0f, (float) spr[0][0].capacity() - 2.0f);
                }
                break;
            case EngineKind::radio:
                if (eng.fadeDepth > 0.0f)
                {
                    adv (fadePh, eng.fadeHz / (float) sr);
                    fade = 1.0f - eng.fadeDepth * 0.5f * (1.0f - std::cos (2.0f * kPi * fadePh));
                }
                break;
            default: break;
        }
        return fade;
    }

    /** One pass through the machine's colour (inside the loop). */
    float engine (int c, float x)
    {
        switch (eng.kind)
        {
            case EngineKind::tape:
            {
                float u = preEmph[c].process (x);
                if (eng.hystMix > 0.0f) u = (1.0f - eng.hystMix) * u + eng.hystMix * hyst[c].process (u * eng.tapeDrive) / eng.tapeDrive;
                u = deEmph[c].process (u);
                return bump[c].process (gapLp[c].process (u));
            }
            case EngineKind::bbd:
            {
                float u = eng.compander ? comp[c].compress (x) : x;
                u = bbd[c].post (bbd[c].pre (u));
                return eng.compander ? comp[c].expand (u) : u;
            }
            case EngineKind::radio:
            {
                float u = rad[c][3].process (rad[c][2].process (rad[c][1].process (rad[c][0].process (x))));
                return std::tanh (eng.drive * (u + eng.asym * u * u)) / eng.drive;
            }
            case EngineKind::spring:
            {
                const float g = 0.6f * clampv (eng.smearAmt, 0.0f, 1.0f);
                for (int k = 0; k < 4; ++k)
                {
                    auto& d = spr[c][k];
                    const float delayed = d.readFrac (springLen[k]);
                    const float v = x + g * delayed;
                    d.push (undenorm (v));
                    x = delayed - g * v;
                }
                return x;
            }
            case EngineKind::digital:
            default:
            {
                if (eng.memSamples > 0.0f)
                {
                    shAcc[c] += fsRatio[c];
                    if (shAcc[c] >= 1.0f) { shAcc[c] -= 1.0f; shHeld[c] = x; }
                    x = shHeld[c];
                }
                if (eng.bits > 0)
                {
                    const float q = (float) (1 << (eng.bits - 1));
                    x = std::trunc (x * q) / q;
                }
                return x;
            }
        }
    }

    /** Noise and fade go on the wet output only, never into the loop. The noise gate follows the echo level. */
    float engineOut (int c, float e, float fade)
    {
        if (noiseLin <= 0.0f && fade == 1.0f) return e;
        float n = 0.0f;
        if (noiseLin > 0.0f)
        {
            nzEnv[c] = undenorm (std::max (std::fabs (e), nzEnv[c] * envRel));
            const float gate = clampv ((nzEnv[c] - 0.0003f) / 0.0027f, 0.0f, 1.0f);
            if (gate > 0.0f)
            {
                n = noiseLin * gate * (eng.kind == EngineKind::tape ? nz[c].pink() : nz[c].white());
                if (eng.kind == EngineKind::bbd && eng.compander) n *= clampv (nzEnv[c] / 0.1f, 0.25f, 1.0f);
            }
        }
        return e * fade + n;
    }

    /** What happens to the signal each time round the loop: filters, decay EQ, engine, saturation, diffusion. */
    float chain (int c, float x)
    {
        if (params.freeze) return safety (x);
        x = lc[c][1].process (lc[c][0].process (x));
        x = hc[c][1].process (hc[c][0].process (x));
        x = bandL[c][1].process (bandL[c][0].process (x));
        x = bandH[c][1].process (bandH[c][0].process (x));
        x = dHigh[c].process (dMid[c].process (dLow[c].process (x)));
        x = engine (c, x);
        if (params.style.satType == 3)
        {
            const float a = std::fabs (x);
            pumpEnv[c] = a > pumpEnv[c] ? a : pumpEnv[c] * 0.9995f;
        }
        x = saturate (c, x);
        if (params.style.diffLoop && params.style.diffAmount > 0.001f) x = diffuse (c, x);
        return safety (dc[c].process (x));
    }

    float accentGain (float accent, bool second) const
    {
        const float a = clampv (accent, -1.0f, 1.0f) * 0.5f;
        return second ? 1.0f - a : 1.0f + a;
    }

    // Single: one echo; the loop runs every two repeats so Groove can move every other repeat.
    void processSingle (float in, float fb, const float* mod, float& eL, float& eR)
    {
        const float T = timeSm[0].next();
        timeSm[1].next();
        const float g = clampv (params.groove, -1.0f, 1.0f);
        const float fs = feelSamples();
        const float t1 = delaySamples ((T * (1.0f - g / 3.0f) - fs) * ratio[0], mod[0]);
        const float t2 = delaySamples ((2.0f * T - fs) * ratio[1], mod[0]);
        const float tapA = line[0].readFrac (t1), tapB = line[0].readFrac (t2);
        const float loop = params.freeze ? tapB : fb * fb * tapB;
        line[0].push (chain (0, in + loop));
        const float echo = tapA * accentGain (params.accent, false) + (params.freeze ? tapB : fb * tapB) * accentGain (params.accent, true);
        eL = echo;
        // Right side: the same echo a few ms later (L/R Offset) for width.
        const float off = clampv (params.lrOffsetMs, 0.0f, 25.0f) * 0.001f * (float) sr;
        const float rA = line[0].readFrac (std::min (t1 + off, maxDelay())), rB = line[0].readFrac (std::min (t2 + off, maxDelay()));
        eR = off > 0.5f ? rA * accentGain (params.accent, false) + (params.freeze ? rB : fb * rB) * accentGain (params.accent, true) : echo;
    }

    // Dual: two independent echoes (left and right) that can feed each other.
    void processDual (float in, float fb, const float* mod, float& eL, float& eR)
    {
        const float fs = feelSamples();
        const float a = line[0].readFrac (delaySamples ((timeSm[0].next() - fs) * ratio[2], mod[0]));
        const float b = line[1].readFrac (delaySamples ((timeSm[1].next() - fs) * ratio[3], mod[1]));
        const float x = clampv (params.fbMix, 0.0f, 1.0f), fbBal = clampv (params.fbBalance, -1.0f, 1.0f);
        const float fa = fb * std::min (1.0f, 1.0f - fbBal), fbb = fb * std::min (1.0f, 1.0f + fbBal);
        line[0].push (chain (0, in + fa * ((1.0f - x) * a + x * b)));
        line[1].push (chain (1, in + fbb * ((1.0f - x) * b + x * a)));
        eL = a * accentGain (params.accent, false);
        eR = b * accentGain (params.accent2, false);
    }

    // Ping-Pong: left, then right, then left... Groove moves the left repeats.
    void processPingPong (float in, float fb, const float* mod, float& eL, float& eR)
    {
        const float T = timeSm[0].next();
        timeSm[1].next();
        const float g = clampv (params.groove, -1.0f, 1.0f);
        const float tA = T * (1.0f - g / 3.0f);
        const float a = line[0].readFrac (delaySamples ((tA - feelSamples()) * ratio[4], mod[0]));
        const float b = line[1].readFrac (delaySamples ((2.0f * T - tA) * ratio[5], mod[1]));
        const float f = params.freeze ? 1.0f : fb;
        line[0].push (chain (0, in + f * b));
        line[1].push (chain (1, f * a));
        eL = a;
        eR = b;
    }

    // Rhythm: up to 16 taps on one line; the whole pattern recirculates with Feedback.
    void processRhythm (float in, float fb, const float* mod, float& eL, float& eR)
    {
        timeSm[0].next(); timeSm[1].next();
        eL = eR = 0.0f;
        for (int k = 0; k < tapCount; ++k)
        {
            const auto& t = taps[(size_t) k];
            const float v = line[0].readFrac (delaySamples (t.samples, mod[0])) * t.level;
            eL += v * t.panL;
            eR += v * t.panR;
        }
        const float one = line[0].readFrac (delaySamples (loopSamples, mod[0])); // "The One": the downbeat that recirculates
        line[0].push (chain (0, in + (params.freeze ? 1.0f : fb) * one));
        if (fb > 0.001f || params.freeze) { eL += one * (params.freeze ? 1.0f : fb); eR += one * (params.freeze ? 1.0f : fb); }
    }

    void buildTaps()
    {
        const float beatSamples = (float) (60.0 / bpm * sr);
        loopSamples = std::min (primeSnap ((float) clampv (params.rhythmBeats, 1, 8) * beatSamples - feelSamples()), maxDelay());
        std::array<float, 16> pos {}, lvl {};
        int count = 0;
        if (custom.count > 0)
            for (int i = 0; i < std::min (custom.count, 16); ++i) { pos[(size_t) count] = custom.taps[(size_t) i].pos; lvl[(size_t) count++] = custom.taps[(size_t) i].level; }
        else
        {
            const float step = gridBeats[clampv (params.grid, 0, numGrids - 1)];
            for (int i = 0; i < clampv (params.repeats, 1, 16); ++i)
            {
                const float p = step * (float) (i + 1);
                if (p > (float) params.rhythmBeats + 1.0e-4f) break;
                pos[(size_t) count] = p;
                lvl[(size_t) count++] = 1.0f;
            }
        }
        const float lenBeats = (float) clampv (params.rhythmBeats, 1, 8);
        const float warpExp = std::pow (2.0f, -clampv (params.warp, -1.0f, 1.0f));
        tapCount = 0;
        for (int i = 0; i < count && tapCount < 16; ++i)
        {
            float p = clampv (pos[(size_t) i], 0.01f, lenBeats);
            if (std::fabs (params.warp) > 0.001f) p = lenBeats * std::pow (p / lenBeats, warpExp);
            const float samples = primeSnap (p * beatSamples - feelSamples());
            if (samples > maxDelay()) continue; // out of memory: this tap is skipped (greyed out on screen)
            float pl, pr;
            panFor (i, count, pl, pr);
            taps[(size_t) tapCount++] = { samples, lvl[(size_t) i] * shapeLevel (params.shape, i, count), pl, pr };
        }
    }

    void panFor (int i, int n, float& l, float& r) const
    {
        float p = 0.0f; // -1 left .. +1 right
        const float t = n > 1 ? (float) i / (float) (n - 1) : 0.5f;
        switch (params.panShape)
        {
            case panDouble: p = (i % 2 == 0) ? -1.0f : 1.0f; break;
            case panAlt1:   p = (i % 2 == 0) ? -0.8f : 0.8f; break;
            case panAlt2:   p = ((i / 2) % 2 == 0) ? -0.8f : 0.8f; break;
            case panAlt3:   { static const float s[4] = { -0.8f, 0.0f, 0.8f, 0.0f }; p = s[i % 4]; break; }
            case panSweepL: p = 1.0f - 2.0f * t; break;
            case panSweepR: p = -1.0f + 2.0f * t; break;
            case panPan:    p = std::sin ((float) i * 2.3f); break;
            case panCenter:
            default:        p = 0.0f; break;
        }
        l = std::min (1.0f, 1.0f - p);
        r = std::min (1.0f, 1.0f + p);
    }

    void updateFilters()
    {
        const auto& s = params.style;
        const float lcf = clampv (params.lowCutHz, 20.0f, 2000.0f), hcf = clampv (params.highCutHz, 500.0f, 20000.0f);
        const float q = 0.3f + 4.7f * clampv (s.midRes, 0.0f, 1.0f);
        for (int c = 0; c < 2; ++c)
        {
            for (int k = 0; k < 2; ++k)
            {
                lc[c][k].set (Biquad::Type::highPass, lcf, 0.707f, 0.0f, sr);
                hc[c][k].set (Biquad::Type::lowPass, hcf, 0.707f, 0.0f, sr);
                bandL[c][k].set (Biquad::Type::highPass, clampv (s.bandLowHz, 20.0f, 2000.0f), 0.707f, 0.0f, sr);
                bandH[c][k].set (Biquad::Type::lowPass, clampv (s.bandHighHz, 500.0f, 20000.0f), 0.707f, 0.0f, sr);
            }
            dLow[c].set (Biquad::Type::lowShelf, s.lowF, 0.7f, clampv (s.lowDecay, -6.0f, 6.0f), sr);
            dMid[c].set (Biquad::Type::peak, s.midF, q, clampv (s.midDecay, -6.0f, 6.0f), sr);
            dHigh[c].set (Biquad::Type::highShelf, s.highF, 0.7f, clampv (s.highDecay, -6.0f, 6.0f), sr);
            oLow[c].set (Biquad::Type::lowShelf, s.lowF, 0.7f, clampv (s.lowG, -18.0f, 18.0f), sr);
            oMid[c].set (Biquad::Type::peak, s.midF, q, clampv (s.midG, -18.0f, 18.0f), sr);
            oHigh[c].set (Biquad::Type::highShelf, s.highF, 0.7f, clampv (s.highG, -18.0f, 18.0f), sr);
        }
    }
};
} // namespace ptfx

#include "EchoStyles.h"
