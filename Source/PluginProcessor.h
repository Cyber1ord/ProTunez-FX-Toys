#pragma once

#include "DSP/Echo.h"
#include "DSP/Heat.h"
#include "DSP/Plate.h"
#include "EffectIds.h"
#include "PresetManager.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

class ProTunezFXToysProcessor : public juce::AudioProcessor
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

    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;
    ptfx::PresetManager presets { apvts, [this] { return getActiveEffect(); } };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    ptfx::Effect* engine (ptfx::EffectId e);
    void pushParams (ptfx::EffectId e);
    void runEffect (ptfx::EffectId e, float* l, float* r, int n);

    ptfx::Heat heat;
    ptfx::Echo echo;
    ptfx::Plate plate;
    ptfx::Crossfader fader;
    std::atomic<int> running { 0 };
    int pending = 0;
    double sr = 44100.0;
    juce::AudioBuffer<float> monoScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProTunezFXToysProcessor)
};
