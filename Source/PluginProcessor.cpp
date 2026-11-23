#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace ptfx;
using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;
using Range = juce::NormalisableRange<float>;

namespace
{
int packOrder (const std::array<int, Rack::numEffects>& o)
{
    int code = 0;
    for (int i = 0; i < Rack::numEffects; ++i) code |= (o[(size_t) i] & 0xF) << (4 * i);
    return code;
}

std::array<int, Rack::numEffects> unpackOrder (int code)
{
    std::array<int, Rack::numEffects> o {};
    for (int i = 0; i < Rack::numEffects; ++i) o[(size_t) i] = (code >> (4 * i)) & 0xF;
    return o;
}

Range logRange (float lo, float hi, float centre)
{
    Range r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}

juce::StringArray divisionNames()
{
    juce::StringArray s;
    for (auto* n : Echo::divisionNames) s.add (n);
    return s;
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout ProTunezFXToysProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout L;
    auto f = [&] (const char* id, const char* name, Range r, float def, const char* unit = "")
    { L.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def, juce::AudioParameterFloatAttributes().withLabel (unit))); };
    auto pct = [&] (const char* id, const char* name, float def)
    { L.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, Range (0.0f, 100.0f, 0.1f), def, juce::AudioParameterFloatAttributes().withLabel ("%"))); };
    auto c = [&] (const char* id, const char* name, juce::StringArray items, int def)
    { L.add (std::make_unique<APC> (juce::ParameterID { id, 1 }, name, items, def)); };
    auto b = [&] (const char* id, const char* name, bool def)
    { L.add (std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def)); };

    // Rack
    f ("in", "Input", Range (-24.0f, 24.0f, 0.1f), 0.0f, "dB");
    f ("out", "Output", Range (-24.0f, 24.0f, 0.1f), 0.0f, "dB");
    pct ("mix", "Rack Mix", 100.0f);
    b ("heat_on", "Heat On", true);
    b ("echo_on", "Echo On", true);
    b ("plate_on", "Plate On", true);
    b ("squeeze_on", "Squeeze On", true);

    // Heat
    f ("heat_drive", "Heat Drive", Range (0.0f, 36.0f, 0.1f), 6.0f, "dB");
    c ("heat_style", "Heat Style", { "Warm Tube", "Tape", "Transistor", "Diode", "Fuzz" }, 0);
    f ("heat_tone", "Heat Tone", Range (-100.0f, 100.0f, 1.0f), 0.0f);
    f ("heat_lowcut", "Heat Low Cut", logRange (20.0f, 1000.0f, 120.0f), 20.0f, "Hz");
    f ("heat_highcut", "Heat High Cut", logRange (1000.0f, 20000.0f, 6000.0f), 20000.0f, "Hz");
    pct ("heat_mix", "Heat Mix", 100.0f);
    f ("heat_out", "Heat Output", Range (-24.0f, 24.0f, 0.1f), 0.0f, "dB");
    b ("heat_auto", "Heat Auto Gain", true);

    // Echo
    f ("echo_time", "Echo Time", logRange (1.0f, 2000.0f, 300.0f), 375.0f, "ms");
    b ("echo_sync", "Echo Sync", false);
    c ("echo_div", "Echo Note", divisionNames(), 8);
    pct ("echo_fb", "Echo Feedback", 35.0f);
    c ("echo_mode", "Echo Mode", { "Single", "Ping-Pong", "Wide" }, 0);
    c ("echo_style", "Echo Style", { "Digital", "Tape", "Analog", "Lo-Fi" }, 1);
    f ("echo_lowcut", "Echo Low Cut", logRange (20.0f, 2000.0f, 200.0f), 120.0f, "Hz");
    f ("echo_highcut", "Echo High Cut", logRange (500.0f, 20000.0f, 5000.0f), 6000.0f, "Hz");
    pct ("echo_wobble", "Echo Wobble", 20.0f);
    pct ("echo_sat", "Echo Saturation", 20.0f);
    pct ("echo_width", "Echo Width", 100.0f);
    pct ("echo_mix", "Echo Mix", 25.0f);

    // Plate
    f ("plate_pre", "Plate Pre-Delay", Range (0.0f, 250.0f, 0.1f), 20.0f, "ms");
    pct ("plate_decay", "Plate Decay", 55.0f);
    pct ("plate_size", "Plate Size", 70.0f);
    pct ("plate_damp", "Plate Damping", 40.0f);
    f ("plate_lowcut", "Plate Low Cut", logRange (20.0f, 1000.0f, 150.0f), 150.0f, "Hz");
    f ("plate_highcut", "Plate High Cut", logRange (1000.0f, 20000.0f, 7000.0f), 12000.0f, "Hz");
    pct ("plate_mod", "Plate Modulation", 30.0f);
    pct ("plate_width", "Plate Width", 100.0f);
    pct ("plate_mix", "Plate Mix", 20.0f);

    // Squeeze
    f ("sq_thresh", "Squeeze Threshold", Range (-60.0f, 0.0f, 0.1f), -18.0f, "dB");
    f ("sq_ratio", "Squeeze Ratio", logRange (1.0f, 20.0f, 4.0f), 3.0f, ":1");
    f ("sq_attack", "Squeeze Attack", logRange (0.1f, 100.0f, 10.0f), 10.0f, "ms");
    f ("sq_release", "Squeeze Release", logRange (10.0f, 1000.0f, 120.0f), 120.0f, "ms");
    f ("sq_knee", "Squeeze Knee", Range (0.0f, 12.0f, 0.1f), 6.0f, "dB");
    f ("sq_makeup", "Squeeze Makeup", Range (0.0f, 24.0f, 0.1f), 0.0f, "dB");
    f ("sq_schp", "Squeeze Bass Ignore", logRange (20.0f, 500.0f, 100.0f), 80.0f, "Hz");
    c ("sq_char", "Squeeze Character", { "Clean", "Vintage", "Smash" }, 0);
    pct ("sq_mix", "Squeeze Mix", 100.0f);
    return L;
}

ProTunezFXToysProcessor::ProTunezFXToysProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ProTunezFXToys", createLayout())
{
    orderCode.store (packOrder (rack.getOrder()));
}

bool ProTunezFXToysProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ProTunezFXToysProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    pushParamsToRack();
    rack.prepare (sampleRate, samplesPerBlock);
    monoScratch.setSize (1, samplesPerBlock);
    setLatencySamples (rack.latency());
}

void ProTunezFXToysProcessor::pushParamsToRack()
{
    rack.setInputDb (value ("in"));
    rack.setOutputDb (value ("out"));
    rack.setMix (value ("mix") * 0.01f);
    rack.setEnabled (Rack::heat, value ("heat_on") > 0.5f);
    rack.setEnabled (Rack::echo, value ("echo_on") > 0.5f);
    rack.setEnabled (Rack::plate, value ("plate_on") > 0.5f);
    rack.setEnabled (Rack::squeeze, value ("squeeze_on") > 0.5f);
    rack.setOrder (unpackOrder (orderCode.load()));

    Heat::Params h;
    h.driveDb = value ("heat_drive");
    h.style = (int) value ("heat_style");
    h.tone = value ("heat_tone") * 0.01f;
    h.lowCutHz = value ("heat_lowcut");
    h.highCutHz = value ("heat_highcut");
    h.mix = value ("heat_mix") * 0.01f;
    h.outputDb = value ("heat_out");
    h.autoGain = value ("heat_auto") > 0.5f;
    rack.heatFx.setParams (h);

    Echo::Params e;
    e.timeMs = value ("echo_time");
    e.sync = value ("echo_sync") > 0.5f;
    e.division = (int) value ("echo_div");
    e.feedback = value ("echo_fb") * 0.01f;
    e.mode = (int) value ("echo_mode");
    e.style = (int) value ("echo_style");
    e.lowCutHz = value ("echo_lowcut");
    e.highCutHz = value ("echo_highcut");
    e.wobble = value ("echo_wobble") * 0.01f;
    e.saturation = value ("echo_sat") * 0.01f;
    e.width = value ("echo_width") * 0.01f;
    e.mix = value ("echo_mix") * 0.01f;
    rack.echoFx.setParams (e);

    Plate::Params p;
    p.preDelayMs = value ("plate_pre");
    p.decay = value ("plate_decay") * 0.01f;
    p.size = value ("plate_size") * 0.01f;
    p.damping = value ("plate_damp") * 0.01f;
    p.lowCutHz = value ("plate_lowcut");
    p.highCutHz = value ("plate_highcut");
    p.modulation = value ("plate_mod") * 0.01f;
    p.width = value ("plate_width") * 0.01f;
    p.mix = value ("plate_mix") * 0.01f;
    rack.plateFx.setParams (p);

    Squeeze::Params s;
    s.thresholdDb = value ("sq_thresh");
    s.ratio = value ("sq_ratio");
    s.attackMs = value ("sq_attack");
    s.releaseMs = value ("sq_release");
    s.kneeDb = value ("sq_knee");
    s.makeupDb = value ("sq_makeup");
    s.sidechainHpHz = value ("sq_schp");
    s.character = (int) value ("sq_char");
    s.mix = value ("sq_mix") * 0.01f;
    rack.squeezeFx.setParams (s);
}

void ProTunezFXToysProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0) return;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                rack.setTempo (*bpm);

    pushParamsToRack();

    if (buffer.getNumChannels() >= 2)
    {
        rack.process (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
    }
    else
    {
        // Mono track: run the rack in stereo and fold back down.
        if (monoScratch.getNumSamples() < n) monoScratch.setSize (1, n, false, false, true);
        float* l = buffer.getWritePointer (0);
        float* r = monoScratch.getWritePointer (0);
        std::copy (l, l + n, r);
        rack.process (l, r, n);
        for (int i = 0; i < n; ++i) l[i] = 0.5f * (l[i] + r[i]);
    }
}

std::array<int, Rack::numEffects> ProTunezFXToysProcessor::getOrder() const { return unpackOrder (orderCode.load()); }

void ProTunezFXToysProcessor::setOrder (const std::array<int, Rack::numEffects>& order)
{
    Rack check;
    check.setOrder (order);
    if (check.getOrder() != order) return; // not a valid arrangement
    orderCode.store (packOrder (order));
}

void ProTunezFXToysProcessor::moveEffect (int effect, int direction)
{
    auto o = getOrder();
    for (int i = 0; i < Rack::numEffects; ++i)
        if (o[(size_t) i] == effect)
        {
            const int j = i + direction;
            if (j >= 0 && j < Rack::numEffects) std::swap (o[(size_t) i], o[(size_t) j]);
            break;
        }
    setOrder (o);
}

// ---------------------------------------------------------------- state & presets

void ProTunezFXToysProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("order", orderCode.load(), nullptr);
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void ProTunezFXToysProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.hasType (apvts.state.getType())) return;
        if (state.hasProperty ("order")) setOrder (unpackOrder ((int) state.getProperty ("order")));
        currentPreset = (int) state.getProperty ("preset", 0);
        apvts.replaceState (state);
    }
}

const std::vector<FactoryPreset>& ProTunezFXToysProcessor::factoryPresets()
{
    using R = Rack;
    static const std::vector<FactoryPreset> presets = {
        { "Default", { R::squeeze, R::heat, R::echo, R::plate }, {} },
        { "Church Lead Vocal", { R::squeeze, R::heat, R::echo, R::plate },
          { { "sq_thresh", -22 }, { "sq_ratio", 3 }, { "sq_attack", 8 }, { "sq_release", 150 }, { "sq_makeup", 4 }, { "sq_char", 1 },
            { "heat_drive", 5 }, { "heat_style", 0 }, { "heat_tone", 15 }, { "heat_mix", 60 },
            { "echo_sync", 1 }, { "echo_div", 6 }, { "echo_fb", 22 }, { "echo_mix", 14 }, { "echo_style", 1 }, { "echo_highcut", 4500 },
            { "plate_decay", 60 }, { "plate_pre", 35 }, { "plate_mix", 22 }, { "plate_lowcut", 250 } } },
        { "Podcast Voice", { R::squeeze, R::heat, R::echo, R::plate },
          { { "sq_thresh", -24 }, { "sq_ratio", 4 }, { "sq_attack", 5 }, { "sq_release", 100 }, { "sq_makeup", 6 }, { "sq_schp", 100 },
            { "heat_drive", 4 }, { "heat_style", 1 }, { "heat_lowcut", 80 }, { "heat_mix", 50 },
            { "echo_on", 0 }, { "plate_on", 0 } } },
        { "Choir Space", { R::squeeze, R::plate, R::echo, R::heat },
          { { "sq_thresh", -16 }, { "sq_ratio", 2 }, { "heat_on", 0 }, { "echo_on", 0 },
            { "plate_decay", 80 }, { "plate_size", 95 }, { "plate_pre", 45 }, { "plate_mix", 30 }, { "plate_damp", 50 } } },
        { "Warm Mix Bus", { R::squeeze, R::heat, R::echo, R::plate },
          { { "sq_thresh", -12 }, { "sq_ratio", 2 }, { "sq_attack", 30 }, { "sq_release", 300 }, { "sq_knee", 10 }, { "sq_makeup", 2 },
            { "heat_drive", 8 }, { "heat_style", 1 }, { "heat_mix", 70 }, { "echo_on", 0 }, { "plate_on", 0 } } },
        { "Slapback Rock", { R::heat, R::echo, R::squeeze, R::plate },
          { { "heat_drive", 14 }, { "heat_style", 2 }, { "echo_time", 110 }, { "echo_fb", 10 }, { "echo_mix", 30 }, { "echo_style", 1 },
            { "plate_on", 0 }, { "squeeze_on", 0 } } },
        { "Big Echo Throw", { R::squeeze, R::heat, R::echo, R::plate },
          { { "echo_sync", 1 }, { "echo_div", 9 }, { "echo_fb", 55 }, { "echo_mode", 1 }, { "echo_mix", 35 }, { "echo_style", 2 },
            { "heat_on", 0 }, { "plate_mix", 25 }, { "plate_decay", 70 } } },
        { "Lo-Fi Dream", { R::heat, R::echo, R::plate, R::squeeze },
          { { "heat_drive", 18 }, { "heat_style", 3 }, { "heat_highcut", 5000 }, { "echo_style", 3 }, { "echo_fb", 50 }, { "echo_wobble", 70 },
            { "echo_mix", 40 }, { "plate_mix", 30 }, { "plate_mod", 80 }, { "squeeze_on", 0 } } },
        { "Drum Smash", { R::squeeze, R::heat, R::echo, R::plate },
          { { "sq_thresh", -30 }, { "sq_ratio", 10 }, { "sq_attack", 1 }, { "sq_release", 60 }, { "sq_char", 2 }, { "sq_mix", 50 }, { "sq_makeup", 8 },
            { "heat_drive", 12 }, { "heat_style", 4 }, { "heat_mix", 40 }, { "echo_on", 0 }, { "plate_on", 0 } } },
    };
    return presets;
}

int ProTunezFXToysProcessor::getNumPrograms() { return (int) factoryPresets().size(); }

const juce::String ProTunezFXToysProcessor::getProgramName (int index)
{
    const auto& p = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? p[(size_t) index].name : juce::String();
}

void ProTunezFXToysProcessor::setCurrentProgram (int index)
{
    const auto& presets = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentPreset = index;

    // Start from defaults, then apply the preset's own values.
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    const auto& preset = presets[(size_t) index];
    for (const auto& [id, v] : preset.values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    setOrder (preset.order);
}

juce::AudioProcessorEditor* ProTunezFXToysProcessor::createEditor() { return new ProTunezFXToysEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ProTunezFXToysProcessor(); }
