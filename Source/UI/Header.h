// The bar across the top of every screen: logo, effect menu, presets, A/B, undo, settings, bypass.
#pragma once

#include "../PluginProcessor.h"
#include "Widgets.h"

namespace ptfx::ui
{
class Header : public juce::Component, private juce::Timer
{
public:
    explicit Header (ProTunezFXToysProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void (float)> onScaleChosen; // 0.75, 1.0, 1.25, 1.5

private:
    ProTunezFXToysProcessor& proc;
    juce::ComboBox effectBox;
    juce::TextButton prev { "<" }, next { ">" }, presetName, save { "Save" }, revert { "Revert" }, undoBtn { "Undo" },
        a { "A" }, b { "B" }, settings { "Size" };
    Toggle bypass;
    std::unique_ptr<juce::ParameterAttachment> effectAttachment;
    int abSlot = 0;

    void showPresetMenu();
    void askSaveName();
    void timerCallback() override;
};
} // namespace ptfx::ui
