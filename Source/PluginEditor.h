#pragma once

#include "PluginProcessor.h"

/** Dark theme with coloured knob rings. */
class FXToysLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FXToysLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

/** A knob with its name underneath. */
struct LabelledKnob : public juce::Component
{
    LabelledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& name, juce::Colour colour);
    void resized() override;

    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

/** One effect in the rack: title bar with on/off and move buttons, then its controls. */
class EffectPanel : public juce::Component
{
public:
    struct Spec
    {
        int effect;
        juce::String title, onId;
        juce::Colour colour;
        std::vector<std::pair<juce::String, juce::String>> combos;  // id, label
        std::vector<std::pair<juce::String, juce::String>> toggles; // id, label
        std::vector<std::pair<juce::String, juce::String>> knobs;   // id, label
    };

    EffectPanel (ProTunezFXToysProcessor&, const Spec&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh(); // dims controls that don't apply right now

    const Spec spec;
    std::function<float()> meterSource; // optional gain-reduction meter

private:
    ProTunezFXToysProcessor& proc;
    juce::ToggleButton onButton;
    juce::TextButton leftButton { "<" }, rightButton { ">" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttachment;
    juce::OwnedArray<juce::ComboBox> combos;
    juce::OwnedArray<juce::Label> comboLabels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttachments;
    juce::OwnedArray<juce::ToggleButton> toggles;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> toggleAttachments;
    juce::OwnedArray<LabelledKnob> knobs;
    juce::Rectangle<int> meterArea;
    float meterValue = 0.0f;
    bool lastOn = true;
};

class ProTunezFXToysEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ProTunezFXToysEditor (ProTunezFXToysProcessor&);
    ~ProTunezFXToysEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ProTunezFXToysProcessor& proc;
    FXToysLookAndFeel lnf;
    juce::ComboBox presetBox;
    std::unique_ptr<LabelledKnob> inKnob, outKnob, mixKnob;
    juce::OwnedArray<EffectPanel> panels; // indexed by effect id
    std::array<int, ptfx::Rack::numEffects> shownOrder {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProTunezFXToysEditor)
};
