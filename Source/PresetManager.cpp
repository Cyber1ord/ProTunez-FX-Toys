#include "PresetManager.h"

namespace ptfx
{
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state, std::function<EffectId()> activeEffect, juce::File userRoot)
    : apvts (state), active (std::move (activeEffect)), root (std::move (userRoot))
{
    for (int i = 0; i < kNumEffects; ++i) per[i].name = "Default";
}

juce::File PresetManager::defaultUserRoot()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("ProTunez FX Toys").getChildFile ("Presets");
}

juce::File PresetManager::userFolder (EffectId e) const { return root.getChildFile (effectName (e)).getChildFile ("User"); }

juce::StringArray PresetManager::categories (EffectId e) const
{
    juce::StringArray c;
    for (auto& p : factoryPresets (e)) c.addIfNotAlreadyThere (p.category);
    c.add ("User");
    return c;
}

juce::StringArray PresetManager::presetsIn (EffectId e, const juce::String& category) const
{
    juce::StringArray names;
    if (category == "User")
    {
        for (auto& f : userFolder (e).findChildFiles (juce::File::findFiles, false, "*.ptfx"))
            names.add (f.getFileNameWithoutExtension());
        names.sort (true);
        return names;
    }
    for (auto& p : factoryPresets (e))
        if (category == p.category) names.add (p.name);
    return names;
}

PresetManager::Snapshot PresetManager::snapshot (EffectId e) const
{
    Snapshot s;
    const juce::String prefix = effectPrefix (e);
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID().startsWith (prefix)) s[rp->getParameterID()] = rp->getValue();
    return s;
}

void PresetManager::apply (const Snapshot& s)
{
    for (auto& [id, v] : s)
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (v);
            p->endChangeGesture();
        }
}

void PresetManager::resetToDefaults (EffectId e)
{
    Snapshot s = snapshot (e);
    for (auto& [id, v] : s) v = apvts.getParameter (id)->getDefaultValue();
    apply (s);
}

void PresetManager::load (EffectId e, const juce::String& category, const juce::String& name)
{
    if (category == "User")
    {
        auto xml = juce::XmlDocument::parse (userFolder (e).getChildFile (name + ".ptfx"));
        if (xml == nullptr) return;
        resetToDefaults (e);
        Snapshot s;
        for (auto* p : xml->getChildIterator())
            if (p->hasTagName ("PARAM") && p->getStringAttribute ("id").startsWith (effectPrefix (e)))
                s[p->getStringAttribute ("id")] = (float) p->getDoubleAttribute ("value");
        apply (s);
    }
    else
    {
        const FactoryPreset* found = nullptr;
        for (auto& p : factoryPresets (e))
            if (category == p.category && name == p.name) found = &p;
        if (found == nullptr) return;
        resetToDefaults (e);
        for (auto& [id, v] : found->values)
            if (auto* p = apvts.getParameter (id))
            {
                jassert (juce::String (id).startsWith (effectPrefix (e)));
                p->setValueNotifyingHost (p->convertTo0to1 (v));
            }
    }
    auto& pe = per[(int) e];
    pe.category = category;
    pe.name = name;
    pe.loaded = snapshot (e);
}

bool PresetManager::saveUser (EffectId e, const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto folder = userFolder (e);
    if (! folder.createDirectory()) return false;
    juce::XmlElement xml ("PTFXPRESET");
    xml.setAttribute ("effect", effectName (e));
    for (auto& [id, v] : snapshot (e))
    {
        auto* c = xml.createNewChildElement ("PARAM");
        c->setAttribute ("id", id);
        c->setAttribute ("value", (double) v);
    }
    if (! xml.writeTo (folder.getChildFile (clean + ".ptfx"))) return false;
    auto& pe = per[(int) e];
    pe.category = "User";
    pe.name = clean;
    pe.loaded = snapshot (e);
    return true;
}

void PresetManager::step (int direction)
{
    const auto e = active();
    std::vector<std::pair<juce::String, juce::String>> all;
    for (auto& c : categories (e))
        for (auto& n : presetsIn (e, c)) all.push_back ({ c, n });
    if (all.empty()) return;
    int idx = -1;
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i].first == per[(int) e].category && all[i].second == per[(int) e].name) idx = (int) i;
    const int n = (int) all.size();
    idx = idx < 0 ? (direction > 0 ? 0 : n - 1) : ((idx + direction) % n + n) % n;
    load (e, all[(size_t) idx].first, all[(size_t) idx].second);
}

void PresetManager::next() { step (+1); }
void PresetManager::previous() { step (-1); }

void PresetManager::revert()
{
    const auto& pe = per[(int) active()];
    if (pe.loaded.empty()) resetToDefaults (active());
    else apply (pe.loaded);
}

void PresetManager::storeAB (int slot) { per[(int) active()].ab[slot & 1] = snapshot (active()); }

void PresetManager::recallAB (int slot)
{
    const auto& s = per[(int) active()].ab[slot & 1];
    if (! s.empty()) apply (s);
}

juce::String PresetManager::currentName() const { return per[(int) active()].name; }
juce::String PresetManager::currentCategory() const { return per[(int) active()].category; }
} // namespace ptfx
