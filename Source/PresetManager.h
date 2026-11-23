// Presets for the active effect: factory folders, user presets on disk, A/B compare, revert.
#pragma once
#include "EffectIds.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <map>
#include <vector>

namespace ptfx
{
struct FactoryPreset
{
    const char* category;
    const char* name;
    std::vector<std::pair<const char*, float>> values; // parameter id -> value in real units
};

/** Factory presets for one effect, in menu order. */
const std::vector<FactoryPreset>& factoryPresets (EffectId e);

class PresetManager
{
public:
    using Snapshot = std::map<juce::String, float>; // parameter id -> normalised value

    PresetManager (juce::AudioProcessorValueTreeState& state, std::function<EffectId()> activeEffect,
                   juce::File userRoot = defaultUserRoot());

    static juce::File defaultUserRoot(); // ~/Music/ProTunez FX Toys/Presets

    juce::StringArray categories (EffectId e) const;               // factory folders, then "User"
    juce::StringArray presetsIn (EffectId e, const juce::String& category) const;
    void load (EffectId e, const juce::String& category, const juce::String& name);
    bool saveUser (EffectId e, const juce::String& name);
    void next();
    void previous();
    void revert();
    void storeAB (int slot);
    void recallAB (int slot);
    juce::String currentName() const;
    juce::String currentCategory() const;

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::function<EffectId()> active;
    juce::File root;
    struct PerEffect { juce::String category, name; Snapshot loaded, ab[2]; };
    PerEffect per[kNumEffects];

    juce::File userFolder (EffectId e) const;
    Snapshot snapshot (EffectId e) const;
    void apply (const Snapshot& s);
    void resetToDefaults (EffectId e);
    void step (int direction);
};
} // namespace ptfx
