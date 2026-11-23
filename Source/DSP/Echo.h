// ProTunez FX Toys — "Echo": tape, analog and digital style delays.
#pragma once

#include "Common.h"

namespace ptfx
{
class Echo : public Effect
{
public:
    enum Mode  { single = 0, pingPong, wide, numModes };
    enum Style { digital = 0, tape, analog, loFi, numStyles };

    /** Note lengths for tempo sync, in beats (quarter note = 1). */
    static constexpr int kNumDivisions = 12;
    static constexpr const char* divisionNames[kNumDivisions] = {
        "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1 bar"
    };
    static constexpr float divisionBeats[kNumDivisions] = {
        0.125f, 1.0f / 6.0f, 0.25f, 0.375f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f, 1.0f, 1.5f, 2.0f, 4.0f
    };

    static constexpr float kMaxTimeMs = 2500.0f;

    struct Params
    {
        float timeMs     = 375.0f; // 1..2000
        bool  sync       = false;
        int   division   = 8;      // index into divisionBeats
        float feedback   = 0.35f;  // 0..1
        int   mode       = single;
        int   style      = tape;
        float lowCutHz   = 120.0f; // in the repeats
        float highCutHz  = 6000.0f;
        float wobble     = 0.2f;   // 0..1 wow & flutter
        float saturation = 0.2f;   // 0..1 grit in the repeats
        float width      = 1.0f;   // 0..1
        float mix        = 0.3f;   // 0..1
    };

    void setParams (const Params& p)
    {
        params = p;
        feedback.setTarget (clampv (p.feedback, 0.0f, 1.0f) * 0.98f);
        mix.setTarget (clampv (p.mix, 0.0f, 1.0f));
        width.setTarget (clampv (p.width, 0.0f, 1.0f));
        updateTime();
        updateFilters();
    }

    void setTempo (double newBpm) override
    {
        if (newBpm > 20.0 && newBpm < 400.0 && std::fabs (newBpm - bpm) > 0.01) { bpm = newBpm; updateTime(); }
    }

    /** Delay time actually in use, in ms (after tempo sync). */
    float currentTimeMs() const
    {
        if (params.sync) return clampv ((float) (60000.0 / bpm) * divisionBeats[clampv (params.division, 0, kNumDivisions - 1)], 1.0f, kMaxTimeMs);
        return clampv (params.timeMs, 1.0f, 2000.0f);
    }

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        for (auto& d : line) d.allocate ((int) (sr * (kMaxTimeMs * 1.6f) / 1000.0f) + 64);
        feedback.prepare (sr, 30.0f); mix.prepare (sr, 30.0f); width.prepare (sr, 30.0f);
        feedback.snap (clampv (params.feedback, 0.0f, 1.0f) * 0.98f);
        mix.snap (params.mix);
        width.snap (params.width);
        timeSmooth[0].prepare (sr, 120.0f); // slow glide = tape-style pitch bend when time changes
        timeSmooth[1].prepare (sr, 120.0f);
        updateTime();
        timeSmooth[0].snap (timeSmooth[0].getTarget());
        timeSmooth[1].snap (timeSmooth[1].getTarget());
        updateFilters();
        reset();
    }

    void reset() override
    {
        for (int c = 0; c < 2; ++c)
        {
            line[c].reset(); lowCut[c].reset(); highCut[c].reset(); highCut2[c].reset(); dc[c].reset();
            fb[c] = 0.0f;
        }
        wowPhase = flutterPhase = 0.0f;
        crushHold[0] = crushHold[1] = 0.0f;
        crushCount = 0;
    }

    void process (float* left, float* right, int n) override
    {
        const float wowRate = 0.6f, flutterRate = 7.3f;
        const float wobbleAmt = clampv (params.wobble, 0.0f, 1.0f);
        // Style shapes how much each repeat degrades.
        const float styleWobble = params.style == tape ? 1.0f : params.style == analog ? 0.5f : params.style == loFi ? 0.8f : 0.25f;
        const float sat = clampv (params.saturation, 0.0f, 1.0f);
        const float drive = 1.0f + sat * (params.style == digital ? 2.0f : 5.0f);

        for (int i = 0; i < n; ++i)
        {
            const float fbk = feedback.next(), m = mix.next(), w = width.next();

            wowPhase += wowRate / (float) sr;       if (wowPhase >= 1.0f) wowPhase -= 1.0f;
            flutterPhase += flutterRate / (float) sr; if (flutterPhase >= 1.0f) flutterPhase -= 1.0f;
            const float mod = wobbleAmt * styleWobble
                            * (0.004f * std::sin (2.0f * kPi * wowPhase) + 0.0008f * std::sin (2.0f * kPi * flutterPhase));

            const float inL = left[i], inR = right[i];
            float wet[2];

            for (int c = 0; c < 2; ++c)
            {
                const float t = timeSmooth[c].next();
                // Right side wobbles slightly differently so stereo feels alive.
                const float d = t * (1.0f + (c == 0 ? mod : mod * 0.8f));
                wet[c] = line[c].readFrac (std::max (1.0f, d));
            }

            // Shape what goes back into the delay (the "sound" of each repeat).
            float back[2];
            for (int c = 0; c < 2; ++c)
            {
                float y = lowCut[c].process (wet[c]);
                y = highCut[c].process (y);
                if (params.style == analog) y = highCut2[c].process (y);
                if (sat > 0.0f || params.style != digital)
                    y = std::tanh (y * drive) / drive * (params.style == digital ? 1.0f : 1.02f);
                y = dc[c].process (y);
                fb[c] = undenorm (y);
                back[c] = fb[c] * fbk;
            }

            if (params.style == loFi)
            {
                // Reduce the sample rate of the repeats for a crunchy, old-sampler sound.
                if (++crushCount >= 3) { crushCount = 0; crushHold[0] = back[0]; crushHold[1] = back[1]; }
                for (int c = 0; c < 2; ++c) back[c] = std::round (crushHold[c] * 256.0f) / 256.0f;
            }

            const float mono = 0.5f * (inL + inR);
            switch (params.mode)
            {
                case pingPong: // input enters on the left, repeats bounce side to side
                    line[0].push (mono + back[1]);
                    line[1].push (back[0]);
                    break;
                case wide:
                case single:
                default:
                    line[0].push (inL + back[0]);
                    line[1].push (inR + back[1]);
                    break;
            }

            // Stereo width on the echoes only.
            const float mid = 0.5f * (wet[0] + wet[1]), side = 0.5f * (wet[0] - wet[1]) * w;
            const float eL = mid + side, eR = mid - side;
            left[i]  = inL * (1.0f - m) + eL * m;
            right[i] = inR * (1.0f - m) + eR * m;
        }
    }

private:
    Params params;
    double sr = 44100.0, bpm = 120.0;
    DelayLine line[2];
    Smoothed feedback, mix, width, timeSmooth[2];
    OnePoleHP lowCut[2];
    Biquad highCut[2], highCut2[2];
    DCBlocker dc[2];
    float fb[2] {};
    float wowPhase = 0.0f, flutterPhase = 0.0f;
    float crushHold[2] {};
    int crushCount = 0;

    void updateTime()
    {
        const float samples = currentTimeMs() * 0.001f * (float) sr;
        // "Wide" mode offsets the right side for a big stereo slap.
        const float right = params.mode == wide ? samples * 1.5f : samples;
        timeSmooth[0].setTarget (samples);
        timeSmooth[1].setTarget (std::min (right, (float) line[1].capacity() - 8.0f));
    }

    void updateFilters()
    {
        float hi = clampv (params.highCutHz, 500.0f, 20000.0f);
        if (params.style == tape)   hi = std::min (hi, 9000.0f);
        if (params.style == analog) hi = std::min (hi, 4500.0f);
        if (params.style == loFi)   hi = std::min (hi, 3200.0f);
        float lo = clampv (params.lowCutHz, 20.0f, 2000.0f);
        if (params.style == loFi) lo = std::max (lo, 300.0f);
        for (int c = 0; c < 2; ++c)
        {
            lowCut[c].setCutoff (lo, sr);
            highCut[c].set (Biquad::Type::lowPass, hi, 0.707f, 0.0f, sr);
            highCut2[c].set (Biquad::Type::lowPass, hi * 1.2f, 0.6f, 0.0f, sr);
        }
    }
};
} // namespace ptfx
