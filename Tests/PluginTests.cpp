// Tests that need JUCE: presets, A/B, revert. Uses the real parameter layout.
#include "../Source/Params/Layout.h"
#include "../Source/PresetManager.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace ptfx;
using Catch::Approx;

namespace
{
struct HostlessProcessor : juce::AudioProcessor
{
    HostlessProcessor() : apvts (*this, nullptr, "ProTunezFXToys", params::createLayout()) {}
    const juce::String getName() const override { return "test"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
    juce::AudioProcessorValueTreeState apvts;
};

struct Fixture
{
    juce::ScopedJuceInitialiser_GUI gui;
    HostlessProcessor proc;
    juce::TemporaryFile dir;
    EffectId active = EffectId::heat;
    PresetManager pm { proc.apvts, [this] { return active; }, dir.getFile() };

    float value (const char* id) { return proc.apvts.getParameter (id)->convertFrom0to1 (proc.apvts.getParameter (id)->getValue()); }
    void set (const char* id, float v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); }
};
} // namespace

TEST_CASE ("Presets: each built effect has the five factory folders with at least two presets each")
{
    Fixture f;
    for (auto e : { EffectId::heat, EffectId::echo, EffectId::plate })
    {
        auto cats = f.pm.categories (e);
        for (auto* c : { "Church Vocal", "Podcast Voice", "Choir", "Music", "Creative" })
        {
            REQUIRE (cats.contains (c));
            REQUIRE (f.pm.presetsIn (e, c).size() >= 2);
        }
    }
}

TEST_CASE ("Presets: loading a factory preset changes only that effect's parameters")
{
    Fixture f;
    f.set ("echo_mix", 77.0f);
    const auto& first = factoryPresets (EffectId::heat).front();
    f.pm.load (EffectId::heat, first.category, first.name);
    REQUIRE (f.value ("echo_mix") == Approx (77.0f).margin (0.1));
    REQUIRE (f.pm.currentName() == juce::String (first.name));
}

TEST_CASE ("Presets: a saved user preset loads back the same values")
{
    Fixture f;
    f.set ("heat_mix", 42.0f);
    REQUIRE (f.pm.saveUser (EffectId::heat, "My Glow"));
    f.set ("heat_mix", 90.0f);
    REQUIRE (f.pm.presetsIn (EffectId::heat, "User").contains ("My Glow"));
    f.pm.load (EffectId::heat, "User", "My Glow");
    REQUIRE (f.value ("heat_mix") == Approx (42.0f).margin (0.01));
}

TEST_CASE ("Presets: revert goes back to the values from the last load")
{
    Fixture f;
    const auto& p = factoryPresets (EffectId::heat).front();
    f.pm.load (EffectId::heat, p.category, p.name);
    const float before = f.value ("heat_mix");
    f.set ("heat_mix", before > 50 ? 10.0f : 90.0f);
    f.pm.revert();
    REQUIRE (f.value ("heat_mix") == Approx (before).margin (0.01));
}

TEST_CASE ("Presets: A/B compare stores and recalls two settings")
{
    Fixture f;
    f.set ("heat_mix", 20.0f); f.pm.storeAB (0);
    f.set ("heat_mix", 80.0f); f.pm.storeAB (1);
    f.pm.recallAB (0);
    REQUIRE (f.value ("heat_mix") == Approx (20.0f).margin (0.01));
    f.pm.recallAB (1);
    REQUIRE (f.value ("heat_mix") == Approx (80.0f).margin (0.01));
}

TEST_CASE ("Presets: next wraps from the last preset to the first")
{
    Fixture f;
    f.active = EffectId::plate;
    const auto& all = factoryPresets (EffectId::plate);
    f.pm.load (EffectId::plate, all.back().category, all.back().name);
    f.pm.next();
    REQUIRE (f.pm.currentName() == juce::String (all.front().name));
    f.pm.previous();
    REQUIRE (f.pm.currentName() == juce::String (all.back().name));
}
