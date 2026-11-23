#pragma once

#include "PluginProcessor.h"
#include "UI/EffectView.h"
#include "UI/Header.h"

/** Header + the selected effect's screen, drawn on a 1000×600 canvas and scaled to the window. */
class ProTunezFXToysEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ProTunezFXToysEditor (ProTunezFXToysProcessor&);
    ~ProTunezFXToysEditor() override;
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    ProTunezFXToysProcessor& proc;
    ptfx::ui::LookAndFeel lnf;
    juce::Component canvas;
    ptfx::ui::Header header;
    std::unique_ptr<ptfx::ui::EffectView> view;
    ptfx::EffectId shown = ptfx::EffectId::heat;

    void showEffect (ptfx::EffectId);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProTunezFXToysEditor)
};
