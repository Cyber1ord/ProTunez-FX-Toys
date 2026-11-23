#pragma once
#include "PluginProcessor.h"

// Temporary editor until the app frame (Task 3) lands.
class ProTunezFXToysEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit ProTunezFXToysEditor (ProTunezFXToysProcessor& p) : juce::GenericAudioProcessorEditor (p) { setSize (600, 500); }
};
