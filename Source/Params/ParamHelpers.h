// Small helpers so each effect's parameter list reads like a table.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ptfx::params
{
using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
using APVTS = juce::AudioProcessorValueTreeState;

inline juce::NormalisableRange<float> range (float lo, float hi, float centre = 0.0f, float step = 0.0f)
{
    juce::NormalisableRange<float> r (lo, hi, step);
    if (centre > lo && centre < hi) r.setSkewForCentre (centre);
    return r;
}

inline void addFloat (Layout& L, const juce::String& id, const juce::String& name, juce::NormalisableRange<float> r, float def, const juce::String& unit = {})
{
    L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
                                                        juce::AudioParameterFloatAttributes().withLabel (unit)));
}

inline void addPercent (Layout& L, const juce::String& id, const juce::String& name, float def)
{
    addFloat (L, id, name, range (0.0f, 100.0f, 0.0f, 0.1f), def, "%");
}

inline void addChoice (Layout& L, const juce::String& id, const juce::String& name, const juce::StringArray& items, int def)
{
    L.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def));
}

inline void addBool (Layout& L, const juce::String& id, const juce::String& name, bool def)
{
    L.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
}

inline float get (const APVTS& s, const char* id)
{
    auto* p = s.getRawParameterValue (id);
    jassert (p != nullptr);
    return p != nullptr ? p->load() : 0.0f;
}
inline int geti (const APVTS& s, const char* id) { return (int) std::lround (get (s, id)); }
inline bool getb (const APVTS& s, const char* id) { return get (s, id) > 0.5f; }
} // namespace ptfx::params
