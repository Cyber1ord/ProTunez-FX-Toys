#pragma once

#include "DSP/Rack.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

/** One factory preset: parameter values (in real units) plus the effect order. */
struct FactoryPreset
{
    juce::String name;
    std::array<int, ptfx::Rack::numEffects> order;
    std::vector<std::pair<juce::String, float>> values;
};

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
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Effect order, shared with the screen. Changing it is safe while playing. */
    std::array<int, ptfx::Rack::numEffects> getOrder() const;
    void setOrder (const std::array<int, ptfx::Rack::numEffects>& order);
    void moveEffect (int effect, int direction); // -1 = left, +1 = right

    float getGainReductionDb() const { return rack.squeezeFx.gainReductionDb.load(); }

    static const std::vector<FactoryPreset>& factoryPresets();

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pushParamsToRack();
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    ptfx::Rack rack;
    std::atomic<int> orderCode { 0 }; // packed order, read on the audio thread
    int currentPreset = 0;
    juce::AudioBuffer<float> monoScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProTunezFXToysProcessor)
};
