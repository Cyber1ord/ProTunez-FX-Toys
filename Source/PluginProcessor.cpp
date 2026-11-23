#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params/Layout.h"

using namespace ptfx;

juce::AudioProcessorValueTreeState::ParameterLayout ProTunezFXToysProcessor::createLayout() { return params::createLayout(); }

ProTunezFXToysProcessor::ProTunezFXToysProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ProTunezFXToys", createLayout())
{
    running.store ((int) getActiveEffect());
    pending = running.load();
    setLatencySamples (kLatency);
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
    monoScratch.setSize (1, std::max (samplesPerBlock, 8192));
    for (auto* d : { &dryDelay[0], &dryDelay[1], &comp[0], &comp[1] }) d->allocate (kLatency + 8);
    bypassMix.prepare (sampleRate, 20.0f);
    bypassMix.snap (params::getb (apvts, "bypass") ? 1.0f : 0.0f);
    running.store ((int) getActiveEffect());
    pending = running.load();
    setLatencySamples (kLatency);
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
    // Echo can run from its own Tap tempo instead of the host.
    const double eb = params::getb (apvts, "echo_usehost") ? bpm : (double) params::get (apvts, "echo_bpm");
    echo.setTempo (eb);
    echoBpm.store (eb);
    if (patternDirty.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (patternLock);
        if (lock.isLocked()) { echo.setPattern (pendingPattern); patternDirty.store (false); }
    }

    // Menu changed: fade out, swap at silence, fade in.
    const int wanted = (int) getActiveEffect();
    if (wanted != pending && ! fader.active()) { pending = wanted; fader.start(); }
    if (pending != running.load() && ! fader.active()) fader.start();
    bypassMix.setTarget (params::getb (apvts, "bypass") ? 1.0f : 0.0f);

    float* l = buffer.getWritePointer (0);
    float* r;
    if (buffer.getNumChannels() >= 2) r = buffer.getWritePointer (1);
    else
    {
        if (monoScratch.getNumSamples() < n) monoScratch.setSize (1, n, false, false, true);
        r = monoScratch.getWritePointer (0);
        std::copy (l, l + n, r);
    }

    float pin = 0.0f;
    for (int i = 0; i < n; ++i) pin = std::max (pin, std::max (std::fabs (l[i]), std::fabs (r[i])));
    inPeak.store (std::max (pin, inPeak.load() * 0.9f));

    pushParams (getRunningEffect());
    for (int done = 0; done < n; done += kChunk) processChunk (l + done, r + done, std::min (kChunk, n - done));

    float pout = 0.0f;
    for (int i = 0; i < n; ++i) pout = std::max (pout, std::max (std::fabs (l[i]), std::fabs (r[i])));
    outPeak.store (std::max (pout, outPeak.load() * 0.9f));

    if (buffer.getNumChannels() < 2)
        for (int i = 0; i < n; ++i) l[i] = 0.5f * (l[i] + r[i]);
}

void ProTunezFXToysProcessor::processChunk (float* l, float* r, int n)
{
    // The untouched input, delayed by the reported latency, for Bypass.
    for (int i = 0; i < n; ++i)
    {
        dryDelay[0].push (l[i]); dryDelay[1].push (r[i]);
        dryL[i] = dryDelay[0].read (kLatency + 1);
        dryR[i] = dryDelay[1].read (kLatency + 1);
    }

    // Run the effect; if the menu changed, swap effects at the silent point of the crossfade.
    int swapAt = -1;
    const bool fading = fader.active();
    if (fading)
        for (int i = 0; i < n; ++i)
        {
            fader.next();
            gains[i] = fader.gain();
            if (fader.swapNow() && swapAt < 0) swapAt = i;
        }
    const int split = swapAt < 0 ? n : swapAt;
    auto runWithComp = [this] (float* cl, float* cr, int m)
    {
        if (m <= 0) return;
        runEffect (getRunningEffect(), cl, cr, m);
        // Effects with less latency than kLatency are delayed so everything lines up.
        auto* fx = engine (getRunningEffect());
        const int extra = kLatency - (fx != nullptr ? fx->latency() : 0);
        if (extra > 0)
            for (int i = 0; i < m; ++i)
            {
                comp[0].push (cl[i]); comp[1].push (cr[i]);
                cl[i] = comp[0].read (extra + 1); cr[i] = comp[1].read (extra + 1);
            }
    };
    runWithComp (l, r, split);
    if (swapAt >= 0)
    {
        running.store (pending);
        if (auto* e = engine (getRunningEffect())) e->reset();
        comp[0].reset(); comp[1].reset();
        pushParams (getRunningEffect());
        runWithComp (l + split, r + split, n - split);
    }
    for (int i = 0; i < n; ++i)
    {
        const float g = fading ? gains[i] : 1.0f;
        const float b = bypassMix.next();
        l[i] = l[i] * g * (1.0f - b) + dryL[i] * b;
        r[i] = r[i] * g * (1.0f - b) + dryR[i] * b;
    }
}

static juce::String patternToString (const RhythmPattern& p)
{
    juce::StringArray parts;
    for (int i = 0; i < p.count; ++i) parts.add (juce::String (p.taps[(size_t) i].pos, 4) + ":" + juce::String (p.taps[(size_t) i].level, 3));
    return parts.joinIntoString (";");
}

static RhythmPattern patternFromString (const juce::String& s)
{
    RhythmPattern p;
    for (auto& t : juce::StringArray::fromTokens (s, ";", ""))
    {
        if (p.count >= 16) break;
        const auto bits = juce::StringArray::fromTokens (t, ":", "");
        if (bits.size() == 2) p.taps[(size_t) p.count++] = { bits[0].getFloatValue(), juce::jlimit (0.0f, 1.0f, bits[1].getFloatValue()) };
    }
    return p;
}

void ProTunezFXToysProcessor::setEchoPatternString (const juce::String& s) { setEchoPattern (patternFromString (s)); }

void ProTunezFXToysProcessor::setEchoPattern (const RhythmPattern& p)
{
    {
        const juce::SpinLock::ScopedLockType lock (patternLock);
        pendingPattern = p;
    }
    patternDirty.store (true);
    apvts.state.setProperty ("echoPattern", patternToString (p), nullptr);
}

RhythmPattern ProTunezFXToysProcessor::getEchoPattern() const
{
    return patternFromString (apvts.state.getProperty ("echoPattern").toString());
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
        if (state.hasType (apvts.state.getType()))
        {
            apvts.replaceState (state);
            setEchoPattern (getEchoPattern());
        }
    }
}

juce::AudioProcessorEditor* ProTunezFXToysProcessor::createEditor() { return new ProTunezFXToysEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ProTunezFXToysProcessor(); }
