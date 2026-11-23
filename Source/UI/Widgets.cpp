#include "Widgets.h"
#include "../UndoHistory.h"

namespace ptfx::ui
{
void gestureStart (const APVTS& state)
{
    if (auto* h = dynamic_cast<HasUndo*> (&state.processor)) h->checkpoint();
}

juce::String formatValue (float v, const juce::String& unit)
{
    const float a = std::abs (v);
    if (unit == "Hz" && a >= 1000.0f) return juce::String (v / 1000.0f, a >= 10000.0f ? 1 : 2) + " kHz";
    if (unit == "ms" && a >= 1000.0f) return juce::String (v / 1000.0f, 2) + " s";
    if (unit == ":1") return juce::String (v, a < 10.0f ? 1 : 0) + ":1";
    const int places = (unit == "%" || a >= 100.0f) ? 0 : 1;
    return juce::String (v, places) + (unit.isEmpty() ? juce::String() : " " + unit);
}

void caption (juce::Graphics& g, const juce::String& text, int x, int y, juce::Colour c, int w)
{
    g.setColour (c);
    g.setFont (font (10.0f, true).withExtraKerningFactor (0.15f));
    g.drawText (text.toUpperCase(), x, y, w, 14, juce::Justification::centredLeft);
}

void groupBox (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& label)
{
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.fillRoundedRectangle (r.toFloat(), 10.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f, 1.0f);
    if (label.isNotEmpty()) caption (g, label, r.getX() + 12, r.getY() + 8);
}

// ---------------------------------------------------------------- Knob

void Knob::FineSlider::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
    if (onGestureStart) onGestureStart();
    setMouseDragSensitivity (e.mods.isShiftDown() ? 1200 : 250);
    juce::Slider::mouseDown (e);
}

Knob::Knob (APVTS& s, const juce::String& id, const juce::String& label, juce::Colour colour) : name (label)
{
    param = s.getParameter (id);
    jassert (param != nullptr);
    unit = param->getLabel();
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
    slider.onGestureStart = [&s] { gestureStart (s); };
    slider.onRightClick = [this]
    {
        juce::PopupMenu m;
        m.addSectionHeader (name);
        m.addItem (1, "Exact value: " + juce::String (slider.getValue(), 3) + " " + unit, false);
        m.addItem (2, "Reset to default");
        juce::Component::SafePointer<Knob> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
        {
            if (safe != nullptr && r == 2) safe->slider.setValue (safe->slider.getDoubleClickReturnValue());
        });
    };
    addAndMakeVisible (slider);
    attachment = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
}

juce::String Knob::valueText() const
{
    const float v = (float) slider.getValue();
    return formatter ? formatter (v) : formatValue (v, unit);
}

void Knob::place (int cx, int cy, int d)
{
    dial = d;
    const int w = std::max (d + 24, 92);
    setBounds (cx - w / 2, cy - d / 2, w, d + (name.isEmpty() ? 4 : 34));
}

void Knob::resized() { slider.setBounds ((getWidth() - dial) / 2, 0, dial, dial); }

void Knob::paint (juce::Graphics& g)
{
    if (name.isEmpty()) return;
    g.setColour (col::text.withAlpha (isEnabled() ? 1.0f : 0.4f));
    g.setFont (font (11.0f, true).withExtraKerningFactor (0.08f));
    g.drawText (name.toUpperCase(), 0, dial + 3, getWidth(), 14, juce::Justification::centred);
    g.setColour (col::dim);
    g.setFont (font (11.0f));
    g.drawText (valueText(), 0, dial + 17, getWidth(), 14, juce::Justification::centred);
}

// ---------------------------------------------------------------- Segments

Segments::Segments (APVTS& s, const juce::String& id, juce::StringArray labels, juce::Colour colour, bool vert) : state (s), vertical (vert)
{
    param = s.getParameter (id);
    jassert (param != nullptr);
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new juce::TextButton (labels[i]));
        b->setColour (juce::TextButton::buttonOnColourId, colour);
        b->onClick = [this, i]
        {
            gestureStart (state);
            if (attachment) attachment->setValueAsCompleteGesture ((float) i);
            if (onChange) onChange (i);
        };
        addAndMakeVisible (b);
    }
    attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v) { refresh (v); }, s.undoManager);
    attachment->sendInitialUpdate();
}

void Segments::refresh (float v)
{
    const int idx = (int) std::lround (v);
    for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState (i == idx, juce::dontSendNotification);
}

void Segments::place (int x, int y, int buttonW, int buttonH)
{
    bw = buttonW;
    bh = buttonH;
    int total = 0;
    for (auto* b : buttons) total += (bw > 0 ? bw : (int) textWidth (font (11.0f, true), b->getButtonText()) + 22) + 4;
    if (vertical) setBounds (x, y, bw > 0 ? bw : 140, buttons.size() * (bh + 4));
    else setBounds (x, y, total, bh);
}

void Segments::resized()
{
    int x = 0, y = 0;
    for (auto* b : buttons)
    {
        const int w = vertical ? getWidth() : (bw > 0 ? bw : (int) textWidth (font (11.0f, true), b->getButtonText()) + 22);
        b->setBounds (x, y, w, bh);
        if (vertical) y += bh + 4; else x += w + 4;
    }
}

// ---------------------------------------------------------------- Toggle / Choice

Toggle::Toggle (APVTS& s, const juce::String& id, const juce::String& label, juce::Colour colour) : juce::TextButton (label), state (s)
{
    setClickingTogglesState (true);
    setColour (juce::TextButton::buttonOnColourId, colour);
    attachment = std::make_unique<APVTS::ButtonAttachment> (s, id, *this);
}

void Toggle::place (int x, int y, int w, int h)
{
    setBounds (x, y, w > 0 ? w : (int) textWidth (font (11.0f, true), getButtonText()) + 24, h);
}

Choice::Choice (APVTS& s, const juce::String& id, juce::Colour colour) : state (s)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id))) addItemList (c->choices, 1);
    setColour (juce::ComboBox::textColourId, colour);
    setColour (juce::ComboBox::backgroundColourId, col::led);
    attachment = std::make_unique<APVTS::ComboBoxAttachment> (s, id, *this);
}

// ---------------------------------------------------------------- Led

Led::Led (std::function<juce::String()> t, juce::Colour c, float fs) : textFn (std::move (t)), colour (c), size (fs)
{
    setInterceptsMouseClicks (false, false);
    shown = textFn();
    startTimerHz (15);
}

void Led::timerCallback()
{
    auto t = textFn();
    if (t != shown) { shown = t; repaint(); }
}

void Led::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (col::led);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain)));
    g.setColour (colour.withAlpha (0.25f));
    g.drawText (shown, getLocalBounds().translated (0, 1), juce::Justification::centred);
    g.setColour (colour);
    g.drawText (shown, getLocalBounds(), juce::Justification::centred);
}

// ---------------------------------------------------------------- VU

VuMeter::VuMeter (std::function<float()> v, juce::String l) : valueFn (std::move (v)), label (std::move (l)) { startTimerHz (30); }

void VuMeter::timerCallback()
{
    const float target = juce::jlimit (0.0f, 1.0f, valueFn());
    needle += (target - needle) * (target > needle ? 0.35f : 0.12f); // quick rise, slower fall like a real meter
    repaint();
}

void VuMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0a0a0a));
    g.fillRoundedRectangle (r, 9.0f);
    auto face = r.reduced (3.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4ecd8), 0, face.getY(), juce::Colour (0xffe3d6b6), 0, face.getBottom(), false));
    g.fillRoundedRectangle (face, 7.0f);
    const float cx = face.getCentreX(), cy = face.getBottom() + face.getHeight() * 0.05f, R = face.getHeight() * 0.8f;
    for (int i = 0; i <= 10; ++i)
    {
        const float a = juce::degreesToRadians (-50.0f + i * 10.0f);
        g.setColour (i > 7 ? juce::Colour (0xffc0392b) : juce::Colour (0xff222222));
        g.drawLine (cx + R * std::sin (a), cy - R * std::cos (a), cx + (R - 9) * std::sin (a), cy - (R - 9) * std::cos (a), 2.0f);
    }
    g.setColour (juce::Colour (0xff333333));
    g.setFont (font (12.0f, true));
    g.drawText (label, face.withTrimmedTop (face.getHeight() * 0.45f).withHeight (18), juce::Justification::centred);
    const float a = juce::degreesToRadians (-50.0f + needle * 100.0f);
    g.setColour (juce::Colour (0xff111111));
    g.drawLine (cx, cy, cx + R * 1.05f * std::sin (a), cy - R * 1.05f * std::cos (a), 2.0f);
}

// ---------------------------------------------------------------- Level meters

LevelMeters::LevelMeters (std::function<float()> in, std::function<float()> out) : inFn (std::move (in)), outFn (std::move (out)) { startTimerHz (30); }

void LevelMeters::timerCallback()
{
    auto toBar = [] (float peak) { return juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (peak, -60.0f) + 60.0f) / 60.0f); };
    const float i = toBar (inFn()), o = toBar (outFn());
    inL = i > inL ? i : inL * 0.9f + i * 0.1f;
    outL = o > outL ? o : outL * 0.9f + o * 0.1f;
    repaint();
}

void LevelMeters::paint (juce::Graphics& g)
{
    caption (g, "IN", 0, 0, col::dim, 30);
    caption (g, "OUT", 26, 0, col::dim, 40);
    const int h = getHeight() - 18;
    auto bar = [&] (int x, float lvl)
    {
        juce::Rectangle<float> r ((float) x, 18.0f, 8.0f, (float) h);
        g.setColour (juce::Colour (0xff0a0a0a));
        g.fillRoundedRectangle (r, 4.0f);
        auto fill = r.withTop (r.getBottom() - r.getHeight() * lvl);
        // Green to −6 dB, yellow to clip, red at clip.
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff4d4d), 0, r.getY(), juce::Colour (0xff5ad67a), 0, r.getBottom(), false));
        g.fillRoundedRectangle (fill, 4.0f);
    };
    bar (2, inL);
    bar (30, outL);
}
} // namespace ptfx::ui
