// Plate screen (like SuperPlate): plate models, preamps, a big Decay dial, and a draggable EQ in the Tweak panel.
#include "../Params/PlateParams.h"
#include "EffectView.h"

namespace ptfx::ui
{
namespace
{
/** Output EQ picture: drag the two dots for frequency and gain, Alt + mouse wheel for width (Q). */
class EqGraph : public juce::Component, private juce::Timer
{
public:
    EqGraph (juce::AudioProcessorValueTreeState& s, juce::Colour c) : state (s), colour (c) { startTimerHz (15); }
    void timerCallback() override { repaint(); }

    float xFor (float hz) const { return (float) getWidth() * (std::log10 (hz / 20.0f) / 3.0f); }
    float hzFor (float x) const { return 20.0f * std::pow (10.0f, 3.0f * juce::jlimit (0.0f, 1.0f, x / (float) getWidth())); }
    float yFor (float db) const { return (float) getHeight() * 0.5f - db / 20.0f * (float) getHeight() * 0.5f; }
    float dbFor (float y) const { return juce::jlimit (-18.0f, 18.0f, ((float) getHeight() * 0.5f - y) / ((float) getHeight() * 0.5f) * 20.0f); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0a0910));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f }) g.drawVerticalLine ((int) xFor (f), 0.0f, r.getHeight());
        g.drawHorizontalLine ((int) yFor (0.0f), 0.0f, r.getWidth());

        // Build the same filters the reverb uses and draw their combined response.
        const double sr = 48000.0;
        Biquad lo, lo2, hi, e1, e2;
        const float low = params::get (state, "plate_low"), high = params::get (state, "plate_high");
        const int slope = params::geti (state, "plate_lowslope");
        lo.set (Biquad::Type::highPass, low, 0.707f, 0, sr);
        lo2.set (Biquad::Type::highPass, low, 0.707f, 0, sr);
        hi.set (Biquad::Type::lowPass, high, 0.707f, 0, sr);
        e1.set (Biquad::Type::peak, params::get (state, "plate_eq1f"), params::get (state, "plate_eq1q"), params::get (state, "plate_eq1g"), sr);
        e2.set (Biquad::Type::peak, params::get (state, "plate_eq2f"), params::get (state, "plate_eq2q"), params::get (state, "plate_eq2g"), sr);
        juce::Path curve;
        for (int px = 0; px <= getWidth(); px += 2)
        {
            const float f = hzFor ((float) px);
            float db = e1.magnitudeDb (f, sr) + e2.magnitudeDb (f, sr);
            if (low > 21.0f)
            {
                if (slope == 0) db += 10.0f * std::log10 (1.0f / (1.0f + (low / f) * (low / f)));
                else db += lo.magnitudeDb (f, sr) + (slope == 2 ? lo2.magnitudeDb (f, sr) : 0.0f);
            }
            if (high < 19999.0f) db += hi.magnitudeDb (f, sr);
            const float y = juce::jlimit (2.0f, r.getHeight() - 2.0f, yFor (db));
            if (px == 0) curve.startNewSubPath ((float) px, y); else curve.lineTo ((float) px, y);
        }
        g.setColour (colour);
        g.strokePath (curve, juce::PathStrokeType (2.0f));
        for (int b = 0; b < 2; ++b)
        {
            const auto pt = handle (b);
            g.setColour (b == dragging ? juce::Colours::white : colour);
            g.fillEllipse (pt.x - 6, pt.y - 6, 12, 12);
            g.setColour (juce::Colours::black);
            g.setFont (font (9.0f, true));
            g.drawText (juce::String (b + 1), juce::Rectangle<float> (pt.x - 6, pt.y - 6, 12, 12), juce::Justification::centred);
        }
        g.setColour (col::dim);
        g.setFont (font (10.0f));
        g.drawText (u8 ("drag the dots  ·  Alt + wheel = width"), r.reduced (8, 4), juce::Justification::topRight);
    }

    juce::Point<float> handle (int b) const
    {
        const juce::String pre = b == 0 ? "plate_eq1" : "plate_eq2";
        return { xFor (params::get (state, (pre + "f").toRawUTF8())), yFor (params::get (state, (pre + "g").toRawUTF8())) };
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragging = -1;
        for (int b = 0; b < 2; ++b) if (handle (b).getDistanceFrom (e.position) < 12.0f) dragging = b;
        if (dragging >= 0) gestureStart (state);
        if (dragging >= 0) for (auto* id : ids (dragging, false)) state.getParameter (id)->beginChangeGesture();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging < 0) return;
        auto ps = ids (dragging, false);
        set (ps[0], hzFor (e.position.x));
        set (ps[1], dbFor (e.position.y));
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging >= 0) for (auto* id : ids (dragging, false)) state.getParameter (id)->endChangeGesture();
        dragging = -1;
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (! e.mods.isAltDown()) return;
        for (int b = 0; b < 2; ++b)
            if (handle (b).getDistanceFrom (e.position) < 40.0f)
            {
                auto* qid = ids (b, true)[0];
                set (qid, params::get (state, qid) * (w.deltaY > 0 ? 1.15f : 1.0f / 1.15f));
            }
    }

private:
    juce::AudioProcessorValueTreeState& state;
    juce::Colour colour;
    int dragging = -1;

    static std::vector<const char*> ids (int b, bool q)
    {
        if (q) return { b == 0 ? "plate_eq1q" : "plate_eq2q" };
        return b == 0 ? std::vector<const char*> { "plate_eq1f", "plate_eq1g" } : std::vector<const char*> { "plate_eq2f", "plate_eq2g" };
    }
    void set (const char* id, float v) { auto* p = state.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); }
};
} // namespace

class PlateView : public EffectView
{
public:
    explicit PlateView (ProTunezFXToysProcessor& p) : EffectView (p, EffectId::plate),
        meters ([&p] { return p.inPeak.load(); }, [&p] { return p.outPeak.load(); }),
        decayLed ([&p] { const float d = params::get (p.apvts, "plate_decay"); return d >= Plate::kInfinite - 0.01f ? u8 ("∞") : juce::String (d, d < 10 ? 2 : 1) + " s"; }, accent (EffectId::plate), 14.0f),
        eq (p.apvts, accent (EffectId::plate))
    {
        juce::StringArray models;
        for (int i = 0; i < Plate::numModels; ++i) models.add (Plate::model (i).name);
        segments ("plate_model", models, 40, 114, 160, true);
        segments ("plate_preamp", { "TUBE", "SOLID", "CLEAN" }, 40, 290, 0);

        knob ("plate_decay", "", 375, 205, 180).setValueFormatter ([] (float v) { return juce::String (v, 2); });
        addAndMakeVisible (decayLed);
        decayLed.setBounds (330, 282, 90, 26);
        tweakButton (335, 318, 80);

        knob ("plate_pre", "Pre-Delay", 600, 150, 76);
        knob ("plate_mod", "Modulation", 720, 150, 70);
        auto& lo = knob ("plate_low", "Low", 600, 280, 60);
        lo.setValueFormatter ([] (float v) { return v < 21.0f ? juce::String ("OFF") : formatValue (v, "Hz"); });
        auto& hi = knob ("plate_high", "High", 720, 280, 60);
        hi.setValueFormatter ([] (float v) { return v >= 19999.0f ? juce::String ("OFF") : formatValue (v, "Hz"); });

        addAndMakeVisible (meters);
        meters.setBounds (806, 96, 60, 130);
        knob ("plate_in", "Input", 912, 130, 50);
        knob ("plate_out", "Output", 912, 232, 50);
        knob ("plate_mix", "Mix", 862, 352, 60);
        knob ("plate_duck", "Duck", 600, 420, 50);
        toggle ("plate_freeze", "FREEZE", 670, 408, 90);

        // Tweak: EQ graph on the left, the rest on the right.
        tweakHeight = 240;
        tweak.addAndMakeVisible (eq);
        eq.setBounds (14, 30, 560, 196);
        knob ("plate_modrate", "Mod Rate", 640, 66, 44, &tweak);
        knob ("plate_width", "Width", 732, 66, 44, &tweak);
        knob ("plate_balance", "Balance", 824, 66, 44, &tweak);
        knob ("plate_eq1q", "EQ 1 Width", 916, 66, 44, &tweak);
        knob ("plate_eq2q", "EQ 2 Width", 916, 160, 44, &tweak);
        segments ("plate_lowslope", { "6 dB", "12 dB", "24 dB" }, 600, 176, 0, false, &tweak);
        slopeLabel.setText ("LOW CUT SLOPE", juce::dontSendNotification);
        slopeLabel.setFont (font (10.0f, true).withExtraKerningFactor (0.15f));
        slopeLabel.setColour (juce::Label::textColourId, col::dim);
        slopeLabel.setBorderSize ({});
        slopeLabel.setBounds (600, 158, 150, 14);
        tweak.addAndMakeVisible (slopeLabel);
    }

    juce::String subtitle() const override { return "Plate reverb"; }

    void paintFace (juce::Graphics& g) override
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1d1730), 0, 0, juce::Colour (0xff0f0d17), (float) getWidth(), (float) getHeight(), false));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 14.0f);
    }

    void paintOverlay (juce::Graphics& g) override
    {
        caption (g, "PLATE", 40, 96);
        caption (g, "PREAMP", 40, 272);
        g.setColour (juce::Colour (0xff0a0910));
        g.fillEllipse (250.0f, 80.0f, 250.0f, 250.0f);
        g.setColour (juce::Colour (0xff3a3456));
        g.drawEllipse (250.0f, 80.0f, 250.0f, 250.0f, 2.0f);
        caption (g, u8 ("DECAY  0.5 s – ∞"), 322, 92);
        caption (g, "FILTER", 572, 226);
    }

private:
    LevelMeters meters;
    Led decayLed;
    EqGraph eq;
    juce::Label slopeLabel;
};

std::unique_ptr<EffectView> makePlateView (ProTunezFXToysProcessor& p) { return std::make_unique<PlateView> (p); }
} // namespace ptfx::ui
