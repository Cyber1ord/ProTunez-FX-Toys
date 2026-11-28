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
        const bool styleChanged = p.style != configuredStyle;
        params = p;
        const float driveDb = clampv (p.drive, 0.0f, 10.0f) * 3.6f + (p.punish ? 20.0f : 0.0f);
        drive.setTarget (dbToGain (driveDb));
        mix.setTarget (clampv (p.mix, 0.0f, 1.0f));
        output.setTarget (dbToGain (clampv (p.outputDb, -24.0f, 24.0f)));
        if (styleChanged) configureStyle (false);
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
        envCoeffFast = (float) std::exp (-1.0 / (0.02 * sr)); // 20 ms right after a reset or style change
        fastLength = (int) (0.15 * sr);
        meterCoeff = (float) std::exp (-1.0 / (0.15 * sr));
        for (auto& d : dryDelay) d.allocate (Oversampler4x::latency() + 4);
        configureStyle (true);
        updateFilters();
        reset();
    }

    void reset() override
    {
        for (int c = 0; c < 2; ++c)
        {
            os[c].reset(); dc[c].reset(); lowCut[c].reset(); thumpEq[c].reset(); tilt[c].reset();
            hc1[c].reset(); hc2[c].reset(); hcPole[c].reset(); dryDelay[c].reset();
            tIn[c].reset(); tOut[c].reset(); tube[c].reset(); tape[c].reset(); bump[c].reset(); hfLoss[c].reset();
        }
        fastLeft = fastLength;
        inEnv = outEnv = 0.0f;
        attitude = 0.0f;
    }

    int latency() const override { return Oversampler4x::latency(); }
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
                dry[c] = dryDelay[c].read (Oversampler4x::latency() + 1);

                float pre = lowCutOn ? lowCut[c].process (x) : x;
                if (params.thump) pre = thumpEq[c].process (pre);
                pre *= g;
                hot = std::max (hot, std::fabs (pre));

                float up[4];
                os[c].up (pre, up);
                for (auto& v : up) v = core (c, v);
                float y = os[c].down (up);
                if (params.style == styleA) y = hfLoss[c].process (bump[c].process (y));
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
            float ec = envCoeff;
            if (fastLeft > 0) { ec = envCoeffFast; --fastLeft; }
            inEnv = inPow + ec * (inEnv - inPow);
            outEnv = outPow + ec * (outEnv - outPow);
            float comp = 1.0f;
            if (params.autoGain && outEnv > 1.0e-10f) comp = clampv (std::sqrt ((inEnv + 1.0e-12f) / outEnv), 0.03f, 2.0f);
            autoComp.setTarget (comp);
            const float ac = autoComp.next();

            for (int c = 0; c < 2; ++c) ch[c][i] = (dry[c] * (1.0f - m) + wet[c] * ac * m) * out;
        }
        attitudeOut.store (clampv ((gainToDb (attitude) + 12.0f) / 24.0f, 0.0f, 1.0f));
    }

    /** The style core, run at 4x the sample rate. Every style has a slope of 1 for small signals.
        Each style has its own internal headroom (k): lower = cleaner, higher = breaks up sooner. */
    float core (int c, float x)
    {
        const float u = x * kHeadroom[configuredStyle];
        float y;
        if (configuredStyle == styleA)
            y = tape[c].process (tIn[c].process (u * tLevel) / tLevel) / 0.65f;
        else
        {
            y = tube[c].process (tIn[c].process (u * tLevel) / tLevel) * tubeNorm;
            if (hasOutTransformer) y = tOut[c].process (y);
        }
        return y / kHeadroom[configuredStyle];
    }

private:
    Params params;
    double sr = 44100.0;
    Smoothed drive, mix, output, autoComp;
    Oversampler4x os[2];
    OnePoleHP dc[2];
    Transformer tIn[2], tOut[2];
    TubeStage tube[2];
    TapeHysteresis tape[2];
    HeadBump bump[2];
    Biquad hfLoss[2];
    Biquad lowCut[2], thumpEq[2], tilt[2], hc1[2], hc2[2];
    OnePoleLP hcPole[2];
    DelayLine dryDelay[2];
    static constexpr float kHeadroom[numStyles] = { 0.6f, 1.4f, 1.0f, 1.6f, 2.0f };
    int configuredStyle = -1, fastLeft = 0, fastLength = 0;
    float tLevel = 1.0f, tubeNorm = 1.0f, envCoeffFast = 0.99f;
    bool hasOutTransformer = false;
    float inEnv = 0.0f, outEnv = 0.0f, envCoeff = 0.999f, meterCoeff = 0.999f, attitude = 0.0f;
    std::atomic<float> attitudeOut { 0.0f };

    /** Sets up the blocks for the current style. `full` also prepares them for the sample rate (prepare() only);
        a style change during playback only changes settings, so no state is reset and nothing clicks. */
    void configureStyle (bool full)
    {
        const int st = clampv (params.style, 0, (int) numStyles - 1);
        configuredStyle = st;
        fastLeft = fastLength; // quick loudness re-match after a style change

        struct Cfg { float inHz, inAmt, tl, outHz, outAmt; float stageDrive, bias, biasShift, sag, soft, neg; };
        static constexpr Cfg cfg[numStyles] = {
            {  60.0f, 0.3f, 1.0f,  0.0f, 0.0f, 0.0f, 0.00f, 0.0f, 0.0f, 0.00f, 0.0f },   // A tape (stage = hysteresis)
            { 120.0f, 1.2f, 1.0f, 60.0f, 0.4f, 0.5f, 0.05f, 0.2f, 0.0f, 0.00f, 0.8f },   // E console
            { 150.0f, 1.0f, 1.0f, 60.0f, 0.5f, 1.0f, 0.08f, 0.4f, 0.1f, 0.00f, 0.8f },   // N preamp
            {  80.0f, 0.3f, 0.2f,  0.0f, 0.0f, 1.0f, 0.25f, 0.8f, 0.5f, 1.50f, 0.4f },   // T triode
            {  80.0f, 0.3f, 0.2f,  0.0f, 0.0f, 1.0f, 0.00f, 0.2f, 0.5f, 0.25f, 1.0f },   // P pentode
        };
        const Cfg& k = cfg[st];
        tLevel = k.tl;
        hasOutTransformer = k.outAmt > 0.0f;
        for (int c = 0; c < 2; ++c)
        {
            tIn[c].prepare (4.0 * sr, k.inHz, k.inAmt);
            if (hasOutTransformer) tOut[c].prepare (4.0 * sr, k.outHz, k.outAmt);
            if (st == styleA)
            {
                if (full) tape[c].prepare (4.0 * sr);
                tape[c].setDrive (1.5f);
                bump[c].prepare (sr);
                bump[c].set (80.0f, 2.0f);
            }
            else
            {
                if (full) tube[c].prepare (4.0 * sr);
                tube[c].setDrive (k.stageDrive); tube[c].setBias (k.bias); tube[c].setBiasShift (k.biasShift);
                tube[c].setSag (k.sag); tube[c].setCurve (k.soft, k.neg);
            }
        }
        if (st != styleA)
        {
            const float e = 1.0e-3f;
            const float slope = (tube[0].curve (k.bias + e) - tube[0].curve (k.bias - e)) / (2.0f * e);
            tubeNorm = 1.0f / (k.stageDrive * slope);
        }
    }

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
            dc[c].setCutoff (10.0f, sr);
            hfLoss[c].set (Biquad::Type::highShelf, 10000.0f, 0.7f, -0.4f * clampv (params.drive, 0.0f, 10.0f), sr);
            hc1[c].set (Biquad::Type::lowPass, hc, 0.707f, 0.0f, sr);
            hc2[c].set (Biquad::Type::lowPass, hc, 0.707f, 0.0f, sr);
        }
    }
};
} // namespace ptfx
