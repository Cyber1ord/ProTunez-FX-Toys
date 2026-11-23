// Colours and the look-and-feel shared by every screen.
#pragma once

#include "../EffectIds.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ptfx::ui
{
namespace col
{
const juce::Colour bg { 0xff141519 };
const juce::Colour headerBg { 0xff0f1013 };
const juce::Colour panel { 0xff1f2128 };
const juce::Colour edge { 0xff2e313a };
const juce::Colour text { 0xffe9e9ee };
const juce::Colour dim { 0xff8b8e99 };
const juce::Colour brand { 0xffff5a5f };
const juce::Colour led { 0xff050506 };
} // namespace col

inline juce::Colour accent (EffectId e)
{
    static const juce::uint32 c[kNumEffects] = { 0xffff7a3d, 0xff3dd6c8, 0xff9b7bff, 0xffff5aa8,
                                                 0xff5aa8ff, 0xff7be05a, 0xffbfe9ff, 0xff3d7bff };
    return juce::Colour (c[(int) e]);
}

inline juce::Font font (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

/** Text containing non-ASCII characters (·, ↔, ∞, ±). */
inline juce::String u8 (const char* t) { return juce::String::fromUTF8 (t); }

/** Width of text in a font (JUCE 8 way). */
inline float textWidth (const juce::Font& f, const juce::String& t) { return juce::GlyphArrangement::getStringWidth (f, t); }

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (13.0f, true); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (11.0f, true); }
    juce::Font getPopupMenuFont() override { return font (13.0f); }
};
} // namespace ptfx::ui
