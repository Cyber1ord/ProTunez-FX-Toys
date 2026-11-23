// Heat screen (like Decapitator).
#include "EffectView.h"

namespace ptfx::ui
{
class HeatView : public EffectView
{
public:
    HeatView (ProTunezFXToysProcessor& p) : EffectView (p, EffectId::heat),
        attitude ([&p] { return p.getMeter(); }, "ATTITUDE")
    {
        addAndMakeVisible (attitude);
        attitude.setBounds (360, 24, 250, 120);

        toggle ("heat_punish", "PUNISH  +20 dB", 40, 96);
        auto& drive = knob ("heat_drive", "Drive", 140, 250, 130);
        drive.setValueFormatter ([] (float v) { return juce::String (v, 1) + " / 10"; });

        knob ("heat_out", "Output", 830, 210, 86);
        toggle ("heat_auto", "AUTO", 880, 150, 56);

        auto& lc = knob ("heat_lowcut", "Low Cut", 340, 250, 72);
        lc.setValueFormatter ([] (float v) { return v < 21.0f ? juce::String ("OFF") : formatValue (v, "Hz"); });
        toggle ("heat_thump", "THUMP", 310, 332, 60);

        auto& tone = knob ("heat_tone", "Tone", 470, 250, 72);
        tone.setValueFormatter ([] (float v) { return std::abs (v) < 0.5f ? juce::String ("Neutral") : (v < 0 ? "Dark " : "Bright ") + juce::String ((int) std::abs (v)); });

        auto& hc = knob ("heat_highcut", "High Cut", 600, 250, 72);
        hc.setValueFormatter ([] (float v) { return v >= 19999.0f ? juce::String ("OFF") : formatValue (v, "Hz"); });
        toggle ("heat_steep", "STEEP", 570, 332, 60);

        knob ("heat_mix", "Mix", 830, 330, 72);
        segments ("heat_style", { "A", "E", "N", "T", "P" }, 330, 410, 36);
        tweakButton (880, 96, 70);

        tweakHeight = 120;
        auto* tips = new juce::Label ({}, u8 ("Shift-drag any knob for fine adjustment  ·  double-click to reset  ·  right-click for the exact value\n"
                                              "Styles: A tape machine · E British console · N classic preamp · T triode tube · P pentode tube\n"
                                              "Steep: High Cut 6 dB/oct (off) or 30 dB/oct (on)  ·  Thump: low boost at the Low Cut frequency"));
        tips->setFont (font (12.0f));
        tips->setColour (juce::Label::textColourId, col::dim);
        tips->setJustificationType (juce::Justification::topLeft);
        tips->setBounds (14, 28, 940, 80);
        tweak.addAndMakeVisible (tips);
        tipsOwner.reset (tips);
    }

    juce::String subtitle() const override { return "Analog saturation"; }

    void paintFace (juce::Graphics& g) override
    {
        juce::ColourGradient grad (juce::Colour (0xff3a1d14), 290.0f, 100.0f, juce::Colour (0xff120c0b), 900.0f, 520.0f, true);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 14.0f);
    }

    void paintOverlay (juce::Graphics& g) override
    {
        caption (g, "STYLE", 330, 392);
        g.setColour (col::dim);
        g.setFont (font (11.0f));
        g.drawText (u8 ("tape · console · preamp · triode · pentode"), 530, 416, 300, 14, juce::Justification::centredLeft);
    }

private:
    VuMeter attitude;
    std::unique_ptr<juce::Label> tipsOwner;
};

std::unique_ptr<EffectView> makeHeatView (ProTunezFXToysProcessor& p) { return std::make_unique<HeatView> (p); }
} // namespace ptfx::ui
