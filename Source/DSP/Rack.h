// ProTunez FX Toys — the rack: effects chained in any order, each switchable.
#pragma once

#include "Echo.h"
#include "Heat.h"
#include "Plate.h"
#include "Squeeze.h"
#include <array>

namespace ptfx
{
class Rack
{
public:
    enum EffectId { heat = 0, echo, plate, squeeze, numEffects };
    static constexpr const char* effectNames[numEffects] = { "Heat", "Echo", "Plate", "Squeeze" };

    Heat    heatFx;
    Echo    echoFx;
    Plate   plateFx;
    Squeeze squeezeFx;

    Rack()
    {
        setOrder ({ squeeze, heat, echo, plate });
        inGain.snap (1.0f); outGain.snap (1.0f); globalMix.snap (1.0f);
    }

    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        maxBlock = std::max (1, maxBlockSize);
        for (int e = 0; e < numEffects; ++e)
        {
            fx (e).prepare (sr, maxBlock);
            onSmooth[e].prepare (sr, 15.0f);
            onSmooth[e].snap (enabled[(size_t) e] ? 1.0f : 0.0f);
            for (auto& d : slotDry[e]) d.allocate (std::max (1, fx (e).latency()) + 4);
        }
        totalLatency = 0;
        for (int e = 0; e < numEffects; ++e) totalLatency += fx (e).latency();
        for (auto& d : globalDry) d.allocate (totalLatency + 4);
        for (auto& b : scratch) b.assign ((size_t) maxBlock, 0.0f);
        inGain.prepare (sr, 20.0f); outGain.prepare (sr, 20.0f); globalMix.prepare (sr, 20.0f);
        inGain.snap (inGain.getTarget()); outGain.snap (outGain.getTarget()); globalMix.snap (globalMix.getTarget());
        reset();
    }

    void reset()
    {
        for (int e = 0; e < numEffects; ++e) { fx (e).reset(); slotDry[e][0].reset(); slotDry[e][1].reset(); }
        for (auto& d : globalDry) d.reset();
    }

    /** Fixed delay the rack adds; the plugin tells the host so tracks stay in time. */
    int latency() const { return totalLatency; }

    void setEnabled (int e, bool on) { enabled[(size_t) e] = on; onSmooth[e].setTarget (on ? 1.0f : 0.0f); }
    bool isEnabled (int e) const { return enabled[(size_t) e]; }

    void setOrder (const std::array<int, numEffects>& newOrder)
    {
        // Only accept a proper arrangement of all four effects.
        std::array<bool, numEffects> seen {};
        for (int e : newOrder) { if (e < 0 || e >= numEffects || seen[(size_t) e]) return; seen[(size_t) e] = true; }
        order = newOrder;
    }
    const std::array<int, numEffects>& getOrder() const { return order; }

    void setInputDb (float db)  { inGain.setTarget (dbToGain (clampv (db, -24.0f, 24.0f))); }
    void setOutputDb (float db) { outGain.setTarget (dbToGain (clampv (db, -24.0f, 24.0f))); }
    void setMix (float m)       { globalMix.setTarget (clampv (m, 0.0f, 1.0f)); }
    void setTempo (double bpm)  { for (int e = 0; e < numEffects; ++e) fx (e).setTempo (bpm); }

    void process (float* left, float* right, int numSamples)
    {
        int done = 0;
        while (done < numSamples)
        {
            const int n = std::min (maxBlock, numSamples - done);
            processChunk (left + done, right + done, n);
            done += n;
        }
    }

private:
    double sr = 44100.0;
    int maxBlock = 512, totalLatency = 0;
    std::array<int, numEffects> order {};
    std::array<bool, numEffects> enabled { true, true, true, true };
    Smoothed onSmooth[numEffects];
    DelayLine slotDry[numEffects][2];
    DelayLine globalDry[2];
    std::array<std::vector<float>, 4> scratch; // dryL, dryR, rack-dry L, rack-dry R
    Smoothed inGain, outGain, globalMix;

    Effect& fx (int e)
    {
        switch (e)
        {
            case heat: return heatFx;
            case echo: return echoFx;
            case plate: return plateFx;
            case squeeze: default: return squeezeFx;
        }
    }
    const Effect& fx (int e) const { return const_cast<Rack*> (this)->fx (e); }

    void processChunk (float* L, float* R, int n)
    {
        float* rdL = scratch[2].data(); float* rdR = scratch[3].data();
        for (int i = 0; i < n; ++i)
        {
            const float g = inGain.next();
            L[i] *= g; R[i] *= g;
            // The untouched signal, delayed to line up with the processed one.
            globalDry[0].push (L[i]); globalDry[1].push (R[i]);
            rdL[i] = globalDry[0].read (totalLatency + 1);
            rdR[i] = globalDry[1].read (totalLatency + 1);
        }

        float* dL = scratch[0].data(); float* dR = scratch[1].data();
        for (int slot = 0; slot < numEffects; ++slot)
        {
            const int e = order[(size_t) slot];
            Effect& f = fx (e);
            const int lat = f.latency();
            const bool fullyOff = ! enabled[(size_t) e] && onSmooth[e].get() < 1.0e-4f;

            for (int i = 0; i < n; ++i)
            {
                slotDry[e][0].push (L[i]); slotDry[e][1].push (R[i]);
                dL[i] = lat > 0 ? slotDry[e][0].read (lat + 1) : L[i];
                dR[i] = lat > 0 ? slotDry[e][1].read (lat + 1) : R[i];
            }

            // Effects keep running while switched off so their tails don't stop dead,
            // and switching back on is smooth.
            f.process (L, R, n);

            if (fullyOff && lat == 0)
            {
                std::copy (dL, dL + n, L); std::copy (dR, dR + n, R);
                continue;
            }
            for (int i = 0; i < n; ++i)
            {
                const float on = onSmooth[e].next();
                L[i] = dL[i] + on * (L[i] - dL[i]);
                R[i] = dR[i] + on * (R[i] - dR[i]);
            }
        }

        for (int i = 0; i < n; ++i)
        {
            const float m = globalMix.next(), o = outGain.next();
            L[i] = (rdL[i] + m * (L[i] - rdL[i])) * o;
            R[i] = (rdR[i] + m * (R[i] - rdR[i])) * o;
        }
    }

};
} // namespace ptfx
