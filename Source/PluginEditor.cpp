#include "PluginEditor.h"

using namespace ptfx;

namespace Colours
{
const juce::Colour background { 0xff141519 };
const juce::Colour panel      { 0xff1f2128 };
const juce::Colour panelEdge  { 0xff2e313a };
const juce::Colour text       { 0xffe9e9ee };
const juce::Colour dimText    { 0xff8b8e99 };
const juce::Colour brand      { 0xffff5a5f };
const juce::Colour heat       { 0xffff7a3d };
const juce::Colour echo       { 0xff3dd6c8 };
const juce::Colour plate      { 0xff9b7bff };
const juce::Colour squeeze    { 0xffffc93d };
} // namespace Colours

// ------------------------------------------------------------------ look and feel

FXToysLookAndFeel::FXToysLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, Colours::background);
    setColour (juce::ComboBox::outlineColourId, Colours::panelEdge);
    setColour (juce::ComboBox::textColourId, Colours::text);
    setColour (juce::ComboBox::arrowColourId, Colours::dimText);
    setColour (juce::PopupMenu::backgroundColourId, Colours::panel);
    setColour (juce::PopupMenu::textColourId, Colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::panelEdge);
    setColour (juce::Label::textColourId, Colours::text);
    setColour (juce::TextButton::buttonColourId, Colours::background);
    setColour (juce::TextButton::textColourOffId, Colours::dimText);
    setColour (juce::Slider::textBoxTextColourId, Colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void FXToysLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float start, float end, juce::Slider& s)
{
    const auto colour = s.findColour (juce::Slider::rotarySliderFillColourId);
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const float alpha = s.isEnabled() ? 1.0f : 0.35f;

    juce::Path track;
    track.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, start, end, true);
    g.setColour (Colours::panelEdge.withAlpha (alpha));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, start, angle, true);
    g.setColour (colour.withAlpha (alpha));
    g.strokePath (value, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float knobR = r - 7.0f;
    g.setColour (juce::Colour (0xff2a2d35).withAlpha (alpha));
    g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -knobR + 3.0f, 3.0f, knobR * 0.45f, 1.5f);
    g.setColour (Colours::text.withAlpha (alpha));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void FXToysLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto area = b.getLocalBounds().toFloat().reduced (1.0f);
    const auto colour = b.findColour (juce::ToggleButton::tickColourId);
    const bool on = b.getToggleState();
    g.setColour (on ? colour.withAlpha (0.22f) : Colours::background);
    g.fillRoundedRectangle (area, 5.0f);
    g.setColour (on ? colour : (highlighted ? Colours::dimText : Colours::panelEdge));
    g.drawRoundedRectangle (area, 5.0f, 1.2f);
    g.setColour (on ? Colours::text : Colours::dimText);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (b.getButtonText(), area, juce::Justification::centred);
}

// ------------------------------------------------------------------ knob

LabelledKnob::LabelledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& id,
                            const juce::String& name, juce::Colour colour)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    slider.setDoubleClickReturnValue (true, state.getParameter (id)->convertFrom0to1 (state.getParameter (id)->getDefaultValue()));
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f));
    label.setColour (juce::Label::textColourId, Colours::dimText);
    addAndMakeVisible (label);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxTextColourId, Colours::text);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);

    // Tidy numbers with units: "1.2 kHz", "375 ms", "-18.0 dB", "35 %".
    const juce::String unit = state.getParameter (id)->getLabel();
    slider.textFromValueFunction = [unit] (double v)
    {
        const double a = std::abs (v);
        if (unit == "Hz" && a >= 1000.0) return juce::String (v / 1000.0, a >= 10000.0 ? 1 : 2) + " kHz";
        if (unit == ":1") return juce::String (v, a < 10.0 ? 1 : 0) + ":1";
        const int places = (unit == "%" || a >= 100.0) ? 0 : 1;
        return juce::String (v, places) + (unit.isEmpty() ? juce::String() : " " + unit);
    };
    slider.valueFromTextFunction = [] (const juce::String& t)
    {
        const double v = t.retainCharacters ("0123456789.-").getDoubleValue();
        return t.containsIgnoreCase ("k") ? v * 1000.0 : v;
    };
    slider.updateText();
}

void LabelledKnob::resized()
{
    auto a = getLocalBounds();
    label.setBounds (a.removeFromBottom (16));
    slider.setBounds (a);
}

// ------------------------------------------------------------------ effect panel

EffectPanel::EffectPanel (ProTunezFXToysProcessor& p, const Spec& s) : spec (s), proc (p)
{
    auto& st = proc.apvts;
    onButton.setButtonText ("ON");
    onButton.setColour (juce::ToggleButton::tickColourId, spec.colour);
    addAndMakeVisible (onButton);
    onAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, spec.onId, onButton);

    for (auto* b : { &leftButton, &rightButton }) addAndMakeVisible (b);
    leftButton.setTooltip ("Move earlier in the chain");
    rightButton.setTooltip ("Move later in the chain");
    leftButton.onClick  = [this] { proc.moveEffect (spec.effect, -1); };
    rightButton.onClick = [this] { proc.moveEffect (spec.effect, +1); };

    for (auto& [id, name] : spec.combos)
    {
        auto* box = combos.add (new juce::ComboBox());
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (st.getParameter (id)))
            box->addItemList (choice->choices, 1);
        addAndMakeVisible (box);
        comboAttachments.add (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (st, id, *box));
        auto* l = comboLabels.add (new juce::Label ({}, name));
        l->setFont (juce::FontOptions (11.0f));
        l->setColour (juce::Label::textColourId, Colours::dimText);
        addAndMakeVisible (l);
    }
    for (auto& [id, name] : spec.toggles)
    {
        auto* t = toggles.add (new juce::ToggleButton (name));
        t->setColour (juce::ToggleButton::tickColourId, spec.colour);
        addAndMakeVisible (t);
        toggleAttachments.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (st, id, *t));
    }
    for (auto& [id, name] : spec.knobs)
        addAndMakeVisible (knobs.add (new LabelledKnob (st, id, name, spec.colour)));
}

void EffectPanel::paint (juce::Graphics& g)
{
    auto a = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (Colours::panel);
    g.fillRoundedRectangle (a, 10.0f);
    g.setColour (Colours::panelEdge);
    g.drawRoundedRectangle (a, 10.0f, 1.0f);

    // Colour strip and title.
    g.setColour (spec.colour);
    g.fillRoundedRectangle (a.getX() + 12.0f, a.getY() + 12.0f, 4.0f, 20.0f, 2.0f);
    g.setColour (Colours::text);
    g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    g.drawText (spec.title, 24, 10, 120, 24, juce::Justification::centredLeft);

    if (! meterArea.isEmpty())
    {
        auto m = meterArea.toFloat();
        g.setColour (Colours::background);
        g.fillRoundedRectangle (m, 3.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, -meterValue / 24.0f);
        g.setColour (spec.colour);
        g.fillRoundedRectangle (m.withWidth (m.getWidth() * frac), 3.0f);
        g.setColour (Colours::dimText);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText ("Gain reduction  " + juce::String (meterValue, 1) + " dB", meterArea.translated (0, -15).withHeight (14),
                    juce::Justification::centredLeft);
    }

    if (! onButton.getToggleState())
    {
        g.setColour (Colours::background.withAlpha (0.45f));
        g.fillRoundedRectangle (a.withTrimmedTop (44.0f), 10.0f);
    }
}

void EffectPanel::resized()
{
    auto a = getLocalBounds().reduced (12, 10);
    auto top = a.removeFromTop (26);
    rightButton.setBounds (top.removeFromRight (26));
    top.removeFromRight (4);
    leftButton.setBounds (top.removeFromRight (26));
    top.removeFromRight (8);
    onButton.setBounds (top.removeFromRight (44));
    a.removeFromTop (12);

    if (! combos.isEmpty())
    {
        auto labels = a.removeFromTop (14);
        auto row = a.removeFromTop (24);
        const int w = row.getWidth() / combos.size();
        for (int i = 0; i < combos.size(); ++i)
        {
            comboLabels[i]->setBounds (labels.removeFromLeft (w).reduced (2, 0));
            combos[i]->setBounds (row.removeFromLeft (w).reduced (2, 0));
        }
        a.removeFromTop (8);
    }
    if (! toggles.isEmpty())
    {
        auto row = a.removeFromTop (24);
        for (auto* t : toggles) { t->setBounds (row.removeFromLeft (92).reduced (2, 0)); row.removeFromLeft (4); }
        a.removeFromTop (6);
    }

    if (meterSource)
    {
        meterArea = a.removeFromBottom (10);
        a.removeFromBottom (18);
    }

    const int cols = 3, rows = (knobs.size() + cols - 1) / cols;
    const int cellW = a.getWidth() / cols, cellH = std::min (100, a.getHeight() / std::max (1, rows));
    for (int i = 0; i < knobs.size(); ++i)
        knobs[i]->setBounds (a.getX() + (i % cols) * cellW, a.getY() + (i / cols) * cellH, cellW, cellH);
}

void EffectPanel::refresh()
{
    const auto order = proc.getOrder();
    leftButton.setEnabled (order.front() != spec.effect);
    rightButton.setEnabled (order.back() != spec.effect);

    if (spec.effect == Rack::echo)
    {
        // With Sync on, the note value sets the time; otherwise the Time knob does.
        const bool sync = proc.apvts.getRawParameterValue ("echo_sync")->load() > 0.5f;
        for (auto* k : knobs) if (k->label.getText() == "Time") k->slider.setEnabled (! sync);
        for (int i = 0; i < combos.size(); ++i) if (spec.combos[(size_t) i].first == "echo_div") combos[i]->setEnabled (sync);
    }

    if (meterSource)
    {
        const float v = meterSource();
        meterValue = v < meterValue ? v : meterValue + 0.25f * (v - meterValue); // fast fall, slow return
        repaint (meterArea.expanded (2, 16));
    }
    if (onButton.getToggleState() != lastOn) { lastOn = onButton.getToggleState(); repaint(); } // dim when off
}

// ------------------------------------------------------------------ editor

ProTunezFXToysEditor::ProTunezFXToysEditor (ProTunezFXToysProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto& st = proc.apvts;

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    inKnob  = std::make_unique<LabelledKnob> (st, "in", "Input", Colours::brand);
    outKnob = std::make_unique<LabelledKnob> (st, "out", "Output", Colours::brand);
    mixKnob = std::make_unique<LabelledKnob> (st, "mix", "Rack Mix", Colours::brand);
    for (auto* k : { inKnob.get(), outKnob.get(), mixKnob.get() }) addAndMakeVisible (k);

    const std::vector<EffectPanel::Spec> specs = {
        { Rack::heat, "Heat", "heat_on", Colours::heat,
          { { "heat_style", "Style" } }, { { "heat_auto", "Auto Gain" } },
          { { "heat_drive", "Drive" }, { "heat_tone", "Tone" }, { "heat_mix", "Mix" },
            { "heat_lowcut", "Low Cut" }, { "heat_highcut", "High Cut" }, { "heat_out", "Output" } } },
        { Rack::echo, "Echo", "echo_on", Colours::echo,
          { { "echo_style", "Style" }, { "echo_mode", "Mode" }, { "echo_div", "Note" } }, { { "echo_sync", "Sync" } },
          { { "echo_time", "Time" }, { "echo_fb", "Feedback" }, { "echo_mix", "Mix" },
            { "echo_lowcut", "Low Cut" }, { "echo_highcut", "High Cut" }, { "echo_width", "Width" },
            { "echo_wobble", "Wobble" }, { "echo_sat", "Grit" } } },
        { Rack::plate, "Plate", "plate_on", Colours::plate, {}, {},
          { { "plate_decay", "Decay" }, { "plate_size", "Size" }, { "plate_mix", "Mix" },
            { "plate_pre", "Pre-Delay" }, { "plate_damp", "Damping" }, { "plate_mod", "Movement" },
            { "plate_lowcut", "Low Cut" }, { "plate_highcut", "High Cut" }, { "plate_width", "Width" } } },
        { Rack::squeeze, "Squeeze", "squeeze_on", Colours::squeeze,
          { { "sq_char", "Character" } }, {},
          { { "sq_thresh", "Threshold" }, { "sq_ratio", "Ratio" }, { "sq_mix", "Mix" },
            { "sq_attack", "Attack" }, { "sq_release", "Release" }, { "sq_makeup", "Makeup" },
            { "sq_knee", "Knee" }, { "sq_schp", "Bass Ignore" } } },
    };
    for (const auto& s : specs)
    {
        auto* panel = panels.add (new EffectPanel (proc, s));
        if (s.effect == Rack::squeeze) panel->meterSource = [this] { return proc.getGainReductionDb(); };
        addAndMakeVisible (panel);
    }

    shownOrder = proc.getOrder();
    setSize (1120, 560);
    startTimerHz (20);
}

ProTunezFXToysEditor::~ProTunezFXToysEditor() { setLookAndFeel (nullptr); }

void ProTunezFXToysEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::background);
    g.setColour (Colours::text);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText ("ProTunez", 20, 14, 130, 30, juce::Justification::centredLeft);
    g.setColour (Colours::brand);
    g.drawText ("FX Toys", 140, 14, 120, 30, juce::Justification::centredLeft);
    g.setColour (Colours::dimText);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("The Revivalists Media", 22, 44, 200, 16, juce::Justification::centredLeft);

    // Signal flow arrows between panels.
    g.setColour (Colours::panelEdge);
    for (int i = 0; i + 1 < Rack::numEffects; ++i)
    {
        auto* a = panels[shownOrder[(size_t) i]];
        const float x = (float) a->getRight() + 3.0f, y = (float) a->getBounds().getCentreY();
        juce::Path arrow;
        arrow.addTriangle (x, y - 6.0f, x, y + 6.0f, x + 6.0f, y);
        g.fillPath (arrow);
    }
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Signal flows left to right. Use < > to change the order.", 20, getHeight() - 22, 500, 16,
                juce::Justification::centredLeft);
}

void ProTunezFXToysEditor::resized()
{
    auto a = getLocalBounds().reduced (16, 10);
    auto header = a.removeFromTop (70);
    for (auto* k : { mixKnob.get(), outKnob.get(), inKnob.get() }) { k->setBounds (header.removeFromRight (78)); }
    header.removeFromRight (16);
    presetBox.setBounds (header.removeFromRight (220).withSizeKeepingCentre (220, 28));

    a.removeFromTop (6);
    a.removeFromBottom (18);
    const int gap = 12, w = (a.getWidth() - gap * (Rack::numEffects - 1)) / Rack::numEffects;
    for (int slot = 0; slot < Rack::numEffects; ++slot)
        panels[shownOrder[(size_t) slot]]->setBounds (a.getX() + slot * (w + gap), a.getY(), w, a.getHeight());
}

void ProTunezFXToysEditor::timerCallback()
{
    const auto order = proc.getOrder();
    if (order != shownOrder)
    {
        shownOrder = order;
        resized();
        repaint();
    }
    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    for (auto* p : panels) p->refresh();
}
