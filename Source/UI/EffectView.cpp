#include "EffectView.h"

namespace ptfx::ui
{
void TweakPanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff101115));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
    g.setColour (colour.withAlpha (0.5f));
    g.drawLine (12.0f, 0.5f, (float) getWidth() - 12.0f, 0.5f, 1.0f);
    caption (g, u8 ("TWEAK  ·  hidden controls"), 14, 8, colour);
}

EffectView::EffectView (ProTunezFXToysProcessor& p, EffectId e) : proc (p), state (p.apvts), tweak (accent (e)), id (e)
{
    addChildComponent (tweak);
}

void EffectView::setTweakOpen (bool open)
{
    tweak.setVisible (open);
    if (open) tweak.toFront (false);
}

void EffectView::resized() { tweak.setBounds (0, getHeight() - tweakHeight, getWidth(), tweakHeight); }

void EffectView::paintFace (juce::Graphics& g)
{
    const auto c = accentColour();
    juce::ColourGradient grad (c.darker (2.2f).withMultipliedSaturation (0.7f), 0, 0, juce::Colour (0xff0d0e12), (float) getWidth(), (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 14.0f);
}

void EffectView::paint (juce::Graphics& g)
{
    paintFace (g);
    g.setColour (col::edge);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 14.0f, 1.0f);

    g.setColour (accentColour());
    g.setFont (font (30.0f, true).withExtraKerningFactor (0.03f));
    g.drawText (juce::String (effectName (id)).toUpperCase(), 24, 14, 600, 36, juce::Justification::centredLeft);

    const juce::String sub = subtitle().toUpperCase();
    g.setFont (font (12.0f).withExtraKerningFactor (0.15f));
    const int subW = (int) textWidth (g.getCurrentFont(), sub);
    g.setColour (col::text.withAlpha (0.75f));
    g.drawText (sub, 26, 50, subW + 4, 16, juce::Justification::centredLeft);
    g.setColour (accentColour());
    g.drawText (u8 ("  ·  LIKE ") + juce::String (effectLike (id)).toUpperCase(), 26 + subW, 50, 400, 16, juce::Justification::centredLeft);

    paintOverlay (g);
}

Knob& EffectView::knob (const juce::String& pid, const juce::String& label, int cx, int cy, int d, juce::Component* parent)
{
    auto* k = new Knob (state, pid, label, accentColour());
    owned.add (k);
    (parent != nullptr ? parent : this)->addAndMakeVisible (k);
    k->place (cx, cy, d);
    return *k;
}

Segments& EffectView::segments (const juce::String& pid, juce::StringArray labels, int x, int y, int w, bool vertical, juce::Component* parent)
{
    auto* s = new Segments (state, pid, labels, accentColour(), vertical);
    owned.add (s);
    (parent != nullptr ? parent : this)->addAndMakeVisible (s);
    s->place (x, y, w);
    return *s;
}

Toggle& EffectView::toggle (const juce::String& pid, const juce::String& text, int x, int y, int w, juce::Component* parent)
{
    auto* t = new Toggle (state, pid, text, accentColour());
    owned.add (t);
    (parent != nullptr ? parent : this)->addAndMakeVisible (t);
    t->place (x, y, w);
    return *t;
}

Choice& EffectView::choice (const juce::String& pid, int x, int y, int w, int h, juce::Component* parent)
{
    auto* c = new Choice (state, pid, accentColour());
    owned.add (c);
    (parent != nullptr ? parent : this)->addAndMakeVisible (c);
    c->setBounds (x, y, w, h);
    return *c;
}

juce::TextButton& EffectView::tweakButton (int x, int y, int w)
{
    auto* b = new juce::TextButton ("TWEAK");
    owned.add (b);
    b->setClickingTogglesState (true);
    b->setColour (juce::TextButton::buttonOnColourId, accentColour());
    b->onClick = [this, b] { setTweakOpen (b->getToggleState()); };
    addAndMakeVisible (b);
    b->setBounds (x, y, w > 0 ? w : 80, 26);
    return *b;
}

void ComingSoonView::paintOverlay (juce::Graphics& g)
{
    g.setColour (col::dim);
    g.setFont (font (16.0f));
    g.drawText ("This effect is planned and will appear here in a later update.", getLocalBounds().withTrimmedTop (120), juce::Justification::centredTop);
    g.setFont (font (13.0f));
    g.drawText ("Audio passes through unchanged while it is selected.", getLocalBounds().withTrimmedTop (150), juce::Justification::centredTop);
}
} // namespace ptfx::ui
