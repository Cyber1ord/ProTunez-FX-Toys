#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params/Layout.h"

using namespace ptfx;

juce::AudioProcessorValueTreeState::ParameterLayout ProTunezFXToysProcessor::createLayout() { return params::createLayout(); }

ProTunezFXToysProcessor::ProTunezFXToysProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "ProTunezFXToys", createLayout())
{
    running.store ((int) getActiveEffect());
    pending = running.load();
}

bool ProTunezFXToysProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

EffectId ProTunezFXToysProcessor::getActiveEffect() const
{
    return (EffectId) juce::jlimit (0, kNumEffects - 1, params::geti (apvts, "effect"));
}

Effect* ProTunezFXToysProcessor::engine (EffectId e)
{
    switch (e)
    {
        case EffectId::heat:  return &heat;
        case EffectId::echo:  return &echo;
        case EffectId::plate: return &plate;
        case EffectId::morph: case EffectId::wider: case EffectId::filter: case EffectId::crystal: case EffectId::space:
        default:              return nullptr; // not built yet: passes audio through
    }
}

float ProTunezFXToysProcessor::getMeter() const
{
    auto* self = const_cast<ProTunezFXToysProcessor*> (this);
    auto* e = self->engine (getRunningEffect());
    return e != nullptr ? e->meter() : 0.0f;
}

void ProTunezFXToysProcessor::pushParams (EffectId e)
{
    switch (e)
    {
        case EffectId::heat:  heat.setParams (params::readHeat (apvts)); break;
        case EffectId::echo:  echo.setParams (params::readEcho (apvts)); break;
        case EffectId::plate: plate.setParams (params::readPlate (apvts)); break;
        case EffectId::morph: case EffectId::wider: case EffectId::filter: case EffectId::crystal: case EffectId::space:
        default: break;
    }
}

void ProTunezFXToysProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (int i = 0; i < kNumEffects; ++i)
        if (auto* e = engine ((EffectId) i))
        {
            pushParams ((EffectId) i);
            e->prepare (sampleRate, samplesPerBlock);
        }
    fader.prepare (sampleRate);
    monoScratch.setSize (1, samplesPerBlock);
    running.store ((int) getActiveEffect());
    pending = running.load();
    auto* e = engine (getRunningEffect());
    setLatencySamples (e != nullptr ? e->latency() : 0);
}

void ProTunezFXToysProcessor::runEffect (EffectId e, float* l, float* r, int n)
{
    if (n <= 0) return;
    if (auto* fx = engine (e)) fx->process (l, r, n);
}

void ProTunezFXToysProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0) return;

    // Tempo for synced effects; 120 BPM when the host gives none.
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) if (*b > 0.0) bpm = *b;
    for (int i = 0; i < kNumEffects; ++i)
        if (auto* e = engine ((EffectId) i)) e->setTempo (bpm);

    // Menu changed: start a fade-out, swap at silence, fade in.
    const int wanted = (int) getActiveEffect();
    if (wanted != pending && ! fader.active()) { pending = wanted; fader.start(); }
    if (pending != running.load() && ! fader.active()) fader.start();

    float* l = buffer.getWritePointer (0);
    float* r;
    if (buffer.getNumChannels() >= 2) r = buffer.getWritePointer (1);
    else
    {
        if (monoScratch.getNumSamples() < n) monoScratch.setSize (1, n, false, false, true);
        r = monoScratch.getWritePointer (0);
        std::copy (l, l + n, r);
    }

    if (! fader.active())
    {
        pushParams (getRunningEffect());
        runEffect (getRunningEffect(), l, r, n);
    }
    else
    {
        // Find where the swap happens inside this block, process each side with its own effect.
        float gains[8192];
        const int m = std::min (n, 8192);
        int swapAt = -1;
        for (int i = 0; i < m; ++i)
        {
            fader.next();
            gains[i] = fader.gain();
            if (fader.swapNow()) swapAt = i;
        }
        const int split = swapAt < 0 ? m : swapAt;
        pushParams (getRunningEffect());
        runEffect (getRunningEffect(), l, r, split);
        if (swapAt >= 0)
        {
            running.store (pending);
            if (auto* e = engine (getRunningEffect())) { e->reset(); setLatencySamples (e->latency()); }
            else setLatencySamples (0);
            pushParams (getRunningEffect());
            runEffect (getRunningEffect(), l + split, r + split, m - split);
        }
        if (m < n) runEffect (getRunningEffect(), l + m, r + m, n - m);
        for (int i = 0; i < m; ++i) { l[i] *= gains[i]; r[i] *= gains[i]; }
    }

    if (buffer.getNumChannels() < 2)
        for (int i = 0; i < n; ++i) l[i] = 0.5f * (l[i] + r[i]);
}

void ProTunezFXToysProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void ProTunezFXToysProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.hasType (apvts.state.getType())) apvts.replaceState (state);
    }
}

juce::AudioProcessorEditor* ProTunezFXToysProcessor::createEditor() { return new ProTunezFXToysEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ProTunezFXToysProcessor(); }
