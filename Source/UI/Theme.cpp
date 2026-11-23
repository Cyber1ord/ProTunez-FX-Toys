#include "Theme.h"

namespace ptfx::ui
{
LookAndFeel::LookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, col::panel);
    setColour (juce::ComboBox::outlineColourId, col::edge);
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::arrowColourId, col::dim);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b1d23));
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::headerTextColourId, col::dim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2a2d35));
    setColour (juce::PopupMenu::highlightedTextColourId, col::text);
    setColour (juce::Label::textColourId, col::text);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0e0f12));
    setColour (juce::TextButton::buttonOnColourId, col::brand);
    setColour (juce::TextButton::textColourOffId, col::dim);
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff111111));
    setColour (juce::AlertWindow::backgroundColourId, col::panel);
    setColour (juce::AlertWindow::textColourId, col::text);
    setColour (juce::TextEditor::backgroundColourId, col::led);
    setColour (juce::TextEditor::textColourId, col::text);
    setColour (juce::TextEditor::outlineColourId, col::edge);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto colour = s.findColour (juce::Slider::rotarySliderFillColourId);
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);
    const float alpha = s.isEnabled() ? 1.0f : 0.35f;
    const float thick = std::max (2.5f, r * 0.09f);

    juce::Path track, value;
    track.addCentredArc (c.x, c.y, r - thick, r - thick, 0.0f, start, end, true);
    value.addCentredArc (c.x, c.y, r - thick, r - thick, 0.0f, start, angle, true);
    g.setColour (col::edge.withAlpha (alpha));
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colour.withAlpha (alpha));
    g.strokePath (value, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float kr = r - thick * 2.6f;
    juce::ColourGradient grad (juce::Colour (0xff4a4d57), c.x - kr * 0.3f, c.y - kr * 0.4f, juce::Colour (0xff202228), c.x + kr, c.y + kr, true);
    g.setGradientFill (grad);
    g.setOpacity (alpha);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (juce::Colour (0xff0a0a0c));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.5f);

    juce::Path pointer;
    const float pw = std::max (2.5f, kr * 0.1f);
    pointer.addRoundedRectangle (-pw * 0.5f, -kr + 2.0f, pw, kr * 0.85f, pw * 0.5f);
    g.setColour (col::text.withAlpha (alpha));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const auto accentCol = b.findColour (juce::TextButton::buttonOnColourId);
    g.setColour (on ? accentCol : b.findColour (juce::TextButton::buttonColourId).brighter (down ? 0.2f : over ? 0.08f : 0.0f));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? accentCol : juce::Colour (0xff33363f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                     .withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.4f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 0), juce::Justification::centred, 2);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 3);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}
} // namespace ptfx::ui
