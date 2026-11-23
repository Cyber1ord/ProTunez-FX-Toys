// Base for each effect's screen: draws the face, title and "like …", and owns the Tweak panel.
#pragma once

#include "../PluginProcessor.h"
#include "Widgets.h"

namespace ptfx::ui
{
/** Face area inside the 1000×600 canvas (below the header). Child positions follow the mockups. */
constexpr int kFaceX = 14, kFaceY = 68, kFaceW = 972, kFaceH = 518;

class TweakPanel : public juce::Component
{
public:
    explicit TweakPanel (juce::Colour c) : colour (c) {}
    void paint (juce::Graphics& g) override;
    juce::Colour colour;
};

class EffectView : public juce::Component
{
public:
    EffectView (ProTunezFXToysProcessor&, EffectId);
    EffectId effect() const { return id; }
    juce::Colour accentColour() const { return accent (id); }

    /** Show or hide the hidden controls. */
    void setTweakOpen (bool open);
    bool isTweakOpen() const { return tweak.isVisible(); }

    void paint (juce::Graphics&) override;
    void resized() override;

protected:
    /** Subtitle under the effect name. */
    virtual juce::String subtitle() const = 0;
    /** Paints the face background (each effect has its own colour scheme). */
    virtual void paintFace (juce::Graphics&);
    /** Extra painting on top of the background (labels, group boxes). */
    virtual void paintOverlay (juce::Graphics&) {}

    /** Height of the Tweak panel, measured up from the bottom of the face. */
    int tweakHeight = 200;

    ProTunezFXToysProcessor& proc;
    APVTS& state;
    TweakPanel tweak;

    // Helpers to add a control and keep ownership.
    Knob& knob (const juce::String& id, const juce::String& label, int cx, int cy, int d, juce::Component* parent = nullptr);
    Segments& segments (const juce::String& id, juce::StringArray labels, int x, int y, int w = 0, bool vertical = false, juce::Component* parent = nullptr);
    Toggle& toggle (const juce::String& id, const juce::String& text, int x, int y, int w = 0, juce::Component* parent = nullptr);
    Choice& choice (const juce::String& id, int x, int y, int w, int h = 26, juce::Component* parent = nullptr);
    juce::TextButton& tweakButton (int x, int y, int w = 0);

private:
    EffectId id;
    juce::OwnedArray<juce::Component> owned;
};

/** Placeholder for effects still to be built. */
class ComingSoonView : public EffectView
{
public:
    using EffectView::EffectView;
    juce::String subtitle() const override { return "Coming in a later stage"; }
    void paintOverlay (juce::Graphics&) override;
};

std::unique_ptr<EffectView> makeView (EffectId, ProTunezFXToysProcessor&);
} // namespace ptfx::ui
