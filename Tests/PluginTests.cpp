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

// ---------------------------------------------------------------- whole-processor checks (review fixes)
#include "../Source/PluginProcessor.h"

namespace
{
struct Proc
{
    juce::ScopedJuceInitialiser_GUI gui;
    ProTunezFXToysProcessor p;
    Proc() { p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512); }
    void set (const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); }
    float get (const char* id) { auto* q = p.apvts.getParameter (id); return q->convertFrom0to1 (q->getValue()); }
    juce::AudioBuffer<float> run (juce::AudioBuffer<float> buf)
    {
        juce::MidiBuffer midi;
        for (int i = 0; i < buf.getNumSamples(); i += 512)
        {
            const int n = std::min (512, buf.getNumSamples() - i);
            juce::AudioBuffer<float> sub (buf.getArrayOfWritePointers(), 2, i, n);
            p.processBlock (sub, midi);
        }
        return buf;
    }
    juce::AudioBuffer<float> silence (int n) { juce::AudioBuffer<float> b (2, n); b.clear(); return run (b); }
};

int impulseArrival (Proc& pr)
{
    juce::AudioBuffer<float> b (2, 4096);
    b.clear();
    b.setSample (0, 0, 1.0f); b.setSample (1, 0, 1.0f);
    auto out = pr.run (b);
    int best = -1; float bv = 0.0f;
    for (int i = 0; i < out.getNumSamples(); ++i) if (std::fabs (out.getSample (0, i)) > bv) { bv = std::fabs (out.getSample (0, i)); best = i; }
    return best;
}
} // namespace

TEST_CASE ("Review fix: reported latency never changes and every effect lines up with it")
{
    Proc pr;
    pr.set ("heat_mix", 0.0f); pr.set ("echo_mix", 0.0f); pr.set ("plate_mix", 0.0f);
    const int lat = pr.p.getLatencySamples();
    REQUIRE (lat == ptfx::Oversampler4x::latency());
    for (int e : { 0, 1, 2, 3 })
    {
        pr.set ("effect", (float) e);
        pr.silence (48000);
        REQUIRE (pr.p.getLatencySamples() == lat);
        REQUIRE (impulseArrival (pr) == lat);
    }
}

TEST_CASE ("Review fix: Bypass fades to the dry sound, in time with the reported latency")
{
    Proc pr;
    pr.set ("effect", 1.0f);
    pr.set ("echo_mix", 100.0f);
    pr.set ("bypass", 1.0f);
    pr.silence (48000);
    REQUIRE (impulseArrival (pr) == pr.p.getLatencySamples());
}

TEST_CASE ("Review fix: each Undo steps back one change at a time")
{
    Proc pr;
    const float start = pr.get ("heat_mix");
    pr.p.checkpoint(); pr.set ("heat_mix", 20.0f);
    pr.p.checkpoint(); pr.set ("heat_mix", 80.0f);
    REQUIRE (pr.p.undoLast());
    REQUIRE (pr.get ("heat_mix") == Approx (20.0f).margin (0.01));
    REQUIRE (pr.p.undoLast());
    REQUIRE (pr.get ("heat_mix") == Approx (start).margin (0.01));
}

TEST_CASE ("Review fix: Echo presets and A/B keep the rhythm pattern")
{
    Proc pr;
    pr.set ("effect", 1.0f);
    ptfx::RhythmPattern pat; pat.count = 3;
    for (int i = 0; i < 3; ++i) pat.taps[(size_t) i] = { 0.5f * (float) (i + 1), 0.8f };
    pr.p.setEchoPattern (pat);
    juce::TemporaryFile dir;
    ptfx::PresetManager pm (pr.p.apvts, [] { return EffectId::echo; }, dir.getFile(),
                            [&pr] (EffectId) { return pr.p.apvts.state.getProperty ("echoPattern").toString(); },
                            [&pr] (EffectId, const juce::String& s) { pr.p.setEchoPatternString (s); });
    REQUIRE (pm.saveUser (EffectId::echo, "Taps"));
    pr.p.setEchoPattern ({});
    pm.load (EffectId::echo, "User", "Taps");
    REQUIRE (pr.p.getEchoPattern().count == 3);
    const auto& f = factoryPresets (EffectId::echo).front();
    pm.load (EffectId::echo, f.category, f.name);
    REQUIRE (pr.p.getEchoPattern().count == 0); // factory presets use Repeats + Grid
}

#include "../Source/Params/EchoParams.h"

TEST_CASE ("Echo: the chosen style picks its engine")
{
    Proc pr;
    pr.set ("echo_style", 11.0f);
    REQUIRE (ptfx::params::readEcho (pr.p.apvts).styleIndex == 11);
}
