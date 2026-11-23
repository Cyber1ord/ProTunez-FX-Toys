// Controls used on every screen, each tied to a parameter. Positions are in the 1000×600 design canvas.
#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

namespace ptfx::ui
{
using APVTS = juce::AudioProcessorValueTreeState;

/** Tells the plug-in a knob turn or click is starting, so Undo can step back to just before it. */
void gestureStart (const APVTS& state);

/** Rotary knob: name and value underneath. Shift-drag = fine, double-click = default, right-click = exact value. */
class Knob : public juce::Component
{
public:
    Knob (APVTS&, const juce::String& paramId, const juce::String& label, juce::Colour colour);
    /** Centre the knob circle (diameter d) on (cx, cy); name and value go below it. */
    void place (int cx, int cy, int d);
    void setValueFormatter (std::function<juce::String (float)> f) { formatter = std::move (f); slider.updateText(); repaint(); }
    void paint (juce::Graphics&) override;
    void resized() override;

    struct FineSlider : juce::Slider
    {
        std::function<void()> onRightClick;
        std::function<void()> onGestureStart;
        void mouseDown (const juce::MouseEvent& e) override;
    } slider;

private:
    juce::String name, unit;
    juce::RangedAudioParameter* param;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
    std::function<juce::String (float)> formatter;
    int dial = 60;
    juce::String valueText() const;
};

/** A row of buttons that picks one option of a choice (or bool) parameter. */
class Segments : public juce::Component
{
public:
    Segments (APVTS&, const juce::String& paramId, juce::StringArray labels, juce::Colour colour, bool vertical = false);
    void place (int x, int y, int buttonW = 0, int buttonH = 26);
    void resized() override;
    std::function<void (int)> onChange;

private:
    juce::OwnedArray<juce::TextButton> buttons;
    APVTS& state;
    juce::RangedAudioParameter* param;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    bool vertical;
    int bw = 0, bh = 26;
    void refresh (float value);
};

/** On/off button for a bool parameter. */
class Toggle : public juce::TextButton
{
public:
    Toggle (APVTS&, const juce::String& paramId, const juce::String& text, juce::Colour colour);
    void place (int x, int y, int w = 0, int h = 26);
    void mouseDown (const juce::MouseEvent& e) override { gestureStart (state); juce::TextButton::mouseDown (e); }

private:
    APVTS& state;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

/** Dropdown for a choice parameter. */
class Choice : public juce::ComboBox
{
public:
    Choice (APVTS&, const juce::String& paramId, juce::Colour colour);
    void mouseDown (const juce::MouseEvent& e) override { gestureStart (state); juce::ComboBox::mouseDown (e); }

private:
    APVTS& state;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

/** Glowing readout, text refreshed 15× a second. */
class Led : public juce::Component, private juce::Timer
{
public:
    Led (std::function<juce::String()> text, juce::Colour colour, float fontSize = 14.0f);
    void paint (juce::Graphics&) override;

private:
    std::function<juce::String()> textFn;
    juce::Colour colour;
    float size;
    juce::String shown;
    void timerCallback() override;
};

/** Needle meter. value() returns 0..1. */
class VuMeter : public juce::Component, private juce::Timer
{
public:
    VuMeter (std::function<float()> value, juce::String label);
    void paint (juce::Graphics&) override;

private:
    std::function<float()> valueFn;
    juce::String label;
    float needle = 0.0f;
    void timerCallback() override;
};

/** IN / OUT level bars. */
class LevelMeters : public juce::Component, private juce::Timer
{
public:
    LevelMeters (std::function<float()> in, std::function<float()> out);
    void paint (juce::Graphics&) override;

private:
    std::function<float()> inFn, outFn;
    float inL = 0, outL = 0;
    void timerCallback() override;
};

/** Small spaced capitals used for section names. */
void caption (juce::Graphics&, const juce::String& text, int x, int y, juce::Colour c = col::dim, int w = 300);
/** A softly outlined group box. */
void groupBox (juce::Graphics&, juce::Rectangle<int> r, const juce::String& label = {});
/** Value text with units: "1.2 kHz", "375 ms", "-18.0 dB", "35 %". */
juce::String formatValue (float v, const juce::String& unit);
} // namespace ptfx::ui
