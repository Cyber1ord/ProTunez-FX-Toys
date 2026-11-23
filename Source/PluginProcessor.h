#pragma once

#include "DSP/Echo.h"
#include "DSP/Heat.h"
#include "DSP/Plate.h"
#include "EffectIds.h"
#include "PresetManager.h"
#include "UndoHistory.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

class ProTunezFXToysProcessor : public juce::AudioProcessor, public ptfx::HasUndo
{
public:
    ProTunezFXToysProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ProTunez FX Toys"; }
    bool acceptsMidi() const override { return true; }   // Voice Morph and Filter take MIDI later
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 60.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** The effect the menu has selected. */
    ptfx::EffectId getActiveEffect() const;
    /** The effect currently producing sound (lags the menu by the 20 ms crossfade). */
    ptfx::EffectId getRunningEffect() const { return (ptfx::EffectId) running.load(); }
    /** The running effect's meter value (0..1). */
    float getMeter() const;
    /** Peak levels for the IN/OUT meters. */
    std::atomic<float> inPeak { 0.0f }, outPeak { 0.0f };
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    /** Echo's custom rhythm pattern (from the Rhythm editor). Message thread only. */
    void setEchoPattern (const ptfx::RhythmPattern& p);
    ptfx::RhythmPattern getEchoPattern() const;
    /** Tempo Echo is using right now (host or its own Tap tempo). */
    double getEchoBpm() const { return echoBpm.load(); }

    /** Echo's pattern as text ("beats:level;..."), used by presets. Message thread only. */
    void setEchoPatternString (const juce::String& s);

    // Undo (HasUndo): the screen calls checkpoint() when a knob turn or click starts.
    void checkpoint() override { history.checkpoint(); }
    bool undoLast() override { return history.undo(); }
    bool canUndo() const override { return history.canUndo(); }

    /** Every effect reports this latency, so switching never shifts the track in time. */
    static constexpr int kLatency = ptfx::Oversampler2x::latency();

    juce::AudioProcessorValueTreeState apvts;
    ptfx::PresetManager presets { apvts, [this] { return getActiveEffect(); }, ptfx::PresetManager::defaultUserRoot(),
                                  [this] (ptfx::EffectId e) { return e == ptfx::EffectId::echo ? apvts.state.getProperty ("echoPattern").toString() : juce::String(); },
                                  [this] (ptfx::EffectId e, const juce::String& s) { if (e == ptfx::EffectId::echo) setEchoPatternString (s); } };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    ptfx::Effect* engine (ptfx::EffectId e);
    void pushParams (ptfx::EffectId e);
    void runEffect (ptfx::EffectId e, float* l, float* r, int n);
    void processChunk (float* l, float* r, int n);

    ptfx::Heat heat;
    ptfx::Echo echo;
    ptfx::Plate plate;
    ptfx::Crossfader fader;
    std::atomic<int> running { 0 };
    int pending = 0;
    juce::SpinLock patternLock;
    ptfx::RhythmPattern pendingPattern;
    std::atomic<bool> patternDirty { false };
    std::atomic<double> echoBpm { 120.0 };
    double sr = 44100.0;
    juce::AudioBuffer<float> monoScratch;
    static constexpr int kChunk = 2048;
    float gains[kChunk] {}, dryL[kChunk] {}, dryR[kChunk] {};
    ptfx::DelayLine dryDelay[2], comp[2];
    ptfx::Smoothed bypassMix;
    ptfx::UndoHistory history { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProTunezFXToysProcessor)
};
