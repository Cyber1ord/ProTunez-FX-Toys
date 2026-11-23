// Echo screen (like EchoBoy): front panel, plus Tweak tabs for the mode, Style Edit and the Rhythm editor.
#include "../Params/EchoParams.h"
#include "EffectView.h"

namespace ptfx::ui
{
namespace
{
const juce::StringArray kNotes { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars" };

/** Small picture of where the repeats land and how loud they are. */
class EchoDisplay : public juce::Component, private juce::Timer
{
public:
    explicit EchoDisplay (juce::AudioProcessorValueTreeState& s, juce::Colour c) : state (s), colour (c) { startTimerHz (10); }
    void timerCallback() override { repaint(); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff06110f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (colour.withAlpha (0.35f));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        const float fb = juce::jlimit (0.0f, 1.0f, params::get (state, "echo_fb") * 0.01f);
        const float groove = params::get (state, "echo_groove") * 0.01f;
        const int mode = params::geti (state, "echo_mode");
        for (int k = 0; k < 8; ++k)
        {
            float pos = (float) (k + 1) - ((k % 2 == 0) ? groove / 3.0f : 0.0f);
            const float level = std::pow (fb, (float) k);
            const float x = r.getX() + 6.0f + (pos - 0.6f) / 8.0f * (r.getWidth() - 12.0f);
            const float h = (r.getHeight() - 10.0f) * level;
            g.setColour (mode == 2 && k % 2 == 1 ? colour.withMultipliedBrightness (0.7f) : colour);
            g.fillRect (x, r.getBottom() - 5.0f - h, 7.0f, h);
        }
    }

private:
    juce::AudioProcessorValueTreeState& state;
    juce::Colour colour;
};

/** Draw the rhythm taps: click to add, Alt-click to delete, drag to move (Cmd = off the grid) and up/down for level. */
class RhythmEditor : public juce::Component, private juce::Timer
{
public:
    RhythmEditor (ProTunezFXToysProcessor& p, juce::Colour c) : proc (p), colour (c) { pattern = proc.getEchoPattern(); startTimerHz (10); }

    void timerCallback() override { repaint(); }

    float beats() const { return (float) juce::jlimit (1, 8, params::geti (proc.apvts, "echo_rbeats")); }
    float gridStep() const { return Echo::gridBeats[juce::jlimit (0, Echo::numGrids - 1, params::geti (proc.apvts, "echo_grid"))]; }

    /** Taps shown: the custom pattern, or the ones made from Repeats and Grid. */
    RhythmPattern shown() const
    {
        if (pattern.count > 0) return pattern;
        RhythmPattern p;
        const int reps = params::geti (proc.apvts, "echo_repeats");
        for (int i = 0; i < reps && p.count < 16; ++i)
        {
            const float pos = gridStep() * (float) (i + 1);
            if (pos > beats() + 1.0e-4f) break;
            p.taps[(size_t) p.count++] = { pos, 1.0f };
        }
        return p;
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff07090b));
        g.fillRoundedRectangle (r, 6.0f);
        const float len = beats(), step = gridStep();
        for (float b = step; b < len - 1.0e-4f; b += step)
        {
            const float x = xFor (b);
            const bool beat = std::fabs (b - std::round (b)) < 1.0e-4f;
            g.setColour (juce::Colours::white.withAlpha (beat ? 0.18f : 0.06f));
            g.drawVerticalLine ((int) x, r.getY() + 4.0f, r.getBottom() - 4.0f);
        }
        const auto p = shown();
        const auto shapeIdx = params::geti (proc.apvts, "echo_shape");
        const float maxSec = Echo::kMaxSeconds * 0.98f;
        const float beatSec = (float) (60.0 / proc.getEchoBpm());
        for (int i = 0; i < p.count; ++i)
        {
            const auto& t = p.taps[(size_t) i];
            const float lvl = t.level * Echo::shapeLevel (shapeIdx, i, p.count);
            const float x = xFor (t.pos), h = (r.getHeight() - 12.0f) * lvl;
            const bool fits = t.pos * beatSec < maxSec;
            g.setColour (fits ? colour : col::dim.withAlpha (0.4f)); // greyed out when out of delay memory
            g.fillRoundedRectangle (x - 4.0f, r.getBottom() - 6.0f - h, 8.0f, h, 2.0f);
        }
        if (params::get (proc.apvts, "echo_fb") > 0.5f)
        {
            g.setColour (juce::Colour (0xffffcf3d));
            g.fillEllipse (xFor (len) - 5.0f, r.getY() + 4.0f, 10.0f, 10.0f); // "The One"
        }
        g.setColour (col::dim);
        g.setFont (font (10.0f));
        g.drawText (pattern.count > 0 ? "custom pattern" : "from Repeats + Grid", r.reduced (6).toNearestInt(), juce::Justification::topLeft);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (pattern.count == 0) pattern = shown();
        dragging = hit (e.position);
        if (e.mods.isAltDown())
        {
            if (dragging >= 0)
            {
                for (int i = dragging; i < pattern.count - 1; ++i) pattern.taps[(size_t) i] = pattern.taps[(size_t) i + 1];
                --pattern.count;
                commit();
            }
            dragging = -1;
            return;
        }
        if (dragging < 0 && pattern.count < 16)
        {
            pattern.taps[(size_t) pattern.count] = { snap (beatFor (e.position.x), e.mods.isCommandDown()), levelFor (e.position.y) };
            dragging = pattern.count++;
            commit();
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging < 0) return;
        auto& t = pattern.taps[(size_t) dragging];
        t.pos = snap (beatFor (e.position.x), e.mods.isCommandDown());
        t.level = levelFor (e.position.y);
        commit();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        // Keep taps in time order.
        std::sort (pattern.taps.begin(), pattern.taps.begin() + pattern.count, [] (auto& a, auto& b) { return a.pos < b.pos; });
        commit();
        dragging = -1;
    }

    void clearCustom() { pattern = {}; commit(); }

    static juce::File folder()
    {
        return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("ProTunez FX Toys").getChildFile ("Patterns");
    }

    void save()
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Save rhythm pattern", folder().getChildFile ("My Pattern.ptpat"), "*.ptpat");
        folder().createDirectory();
        juce::Component::SafePointer<RhythmEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [safe, chooser] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (safe == nullptr || f == juce::File()) return;
            const auto p = safe->shown();
            juce::StringArray lines;
            for (int i = 0; i < p.count; ++i) lines.add (juce::String (p.taps[(size_t) i].pos, 4) + " " + juce::String (p.taps[(size_t) i].level, 3));
            f.withFileExtension ("ptpat").replaceWithText (lines.joinIntoString ("\n"));
        });
    }

    void load()
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Load rhythm pattern", folder(), "*.ptpat");
        juce::Component::SafePointer<RhythmEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe, chooser] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (safe == nullptr || ! f.existsAsFile()) return;
            RhythmPattern p;
            for (auto& line : juce::StringArray::fromLines (f.loadFileAsString()))
            {
                const auto bits = juce::StringArray::fromTokens (line, " ", "");
                if (bits.size() == 2 && p.count < 16) p.taps[(size_t) p.count++] = { bits[0].getFloatValue(), juce::jlimit (0.0f, 1.0f, bits[1].getFloatValue()) };
            }
            safe->pattern = p;
            safe->commit();
        });
    }

private:
    ProTunezFXToysProcessor& proc;
    juce::Colour colour;
    RhythmPattern pattern;
    int dragging = -1;

    float xFor (float beat) const { return 8.0f + beat / beats() * ((float) getWidth() - 16.0f); }
    float beatFor (float x) const { return juce::jlimit (0.01f, beats(), (x - 8.0f) / ((float) getWidth() - 16.0f) * beats()); }
    float levelFor (float y) const { return juce::jlimit (0.05f, 1.0f, ((float) getHeight() - 6.0f - y) / ((float) getHeight() - 12.0f)); }
    float snap (float beat, bool free) const
    {
        if (free) return beat;
        const float step = gridStep();
        return juce::jlimit (step, beats(), std::round (beat / step) * step);
    }
    int hit (juce::Point<float> p) const
    {
        for (int i = 0; i < pattern.count; ++i)
            if (std::fabs (xFor (pattern.taps[(size_t) i].pos) - p.x) < 7.0f) return i;
        return -1;
    }
    void commit() { proc.setEchoPattern (pattern); repaint(); }
};
} // namespace

class EchoView : public EffectView, private juce::Timer
{
public:
    explicit EchoView (ProTunezFXToysProcessor& p) : EffectView (p, EffectId::echo),
        display (p.apvts, accent (EffectId::echo)),
        meters ([&p] { return p.inPeak.load(); }, [&p] { return p.outPeak.load(); }),
        bpmLed ([&p] { return juce::String (p.getEchoBpm(), 1) + " bpm"; }, accent (EffectId::echo), 13.0f)
    {
        // Left: the main echo controls.
        knob ("echo_mix", "Mix", 70, 150, 64);
        timeMsKnob = &knob ("echo_time", "Echo Time", 180, 150, 74);
        timeNoteKnob = &knob ("echo_note", "Echo Time", 180, 150, 74);
        timeNoteKnob->setValueFormatter ([] (float v) { return kNotes[juce::jlimit (0, 7, (int) std::lround (v))]; });
        knob ("echo_fb", "Feedback", 290, 150, 64);
        segments ("echo_timemode", { "TIME", "NOTE", "DOT", "TRIP" }, 40, 226, 62);
        auto& lc = knob ("echo_lowcut", "Low Cut", 70, 300, 60);
        lc.setValueFormatter ([] (float v) { return v < 21.0f ? juce::String ("OFF") : formatValue (v, "Hz"); });
        auto& hc = knob ("echo_highcut", "High Cut", 290, 300, 60);
        hc.setValueFormatter ([] (float v) { return v >= 19999.0f ? juce::String ("OFF") : formatValue (v, "Hz"); });
        addAndMakeVisible (display);
        display.setBounds (118, 272, 120, 60);
        toggle ("echo_prime", "PRIME #", 118, 338, 120);
        knob ("echo_duck", "Duck", 70, 440, 50);
        toggle ("echo_freeze", "FREEZE", 130, 428, 90);

        // Middle: tempo, groove and mode.
        tap.onClick = [this] { tapTempo(); };
        addAndMakeVisible (tap);
        tap.setBounds (400, 112, 44, 28);
        addAndMakeVisible (bpmLed);
        bpmLed.setBounds (452, 110, 120, 30);
        toggle ("echo_usehost", "HOST", 582, 112, 70);
        knob ("echo_groove", "Groove", 450, 186, 48).setValueFormatter ([] (float v) { return std::abs (v) < 0.5f ? juce::String ("Straight") : (v < 0 ? "Shuffle " : "Swing ") + juce::String ((int) std::abs (v)); });
        knob ("echo_feel", "Feel", 600, 186, 48).setValueFormatter ([] (float v) { return std::abs (v) < 0.5f ? juce::String ("On beat") : (v < 0 ? "Drag " : "Rush ") + juce::String ((int) std::abs (v)); });
        segments ("echo_mode", { "SINGLE", "DUAL", "PING-PONG", "RHYTHM" }, 400, 244, 124, true);
        knob ("echo_repeats", "Repeats", 610, 300, 48).setValueFormatter ([] (float v) { return juce::String ((int) std::lround (v)); });

        // Right: levels, saturation and style.
        addAndMakeVisible (meters);
        meters.setBounds (726, 100, 60, 96);
        knob ("echo_in", "Input", 870, 130, 52);
        knob ("echo_out", "Output", 870, 232, 52);
        knob ("echo_sat", "Saturation", 758, 232, 46);
        for (int i = 0; i < Echo::numStyles(); ++i) styleBox.addItem (Echo::styles()[(size_t) i].name, i + 1);
        styleBox.setColour (juce::ComboBox::textColourId, accent (EffectId::echo));
        styleBox.setColour (juce::ComboBox::backgroundColourId, col::led);
        styleBox.onChange = [this] { proc.checkpoint(); params::applyEchoStyle (state, styleBox.getSelectedId() - 1); };
        addAndMakeVisible (styleBox);
        styleBox.setBounds (716, 306, 150, 26);
        editBtn.onClick = [this] { showTab (1); setTweakOpen (true); tweakBtn->setToggleState (true, juce::dontSendNotification); };
        addAndMakeVisible (editBtn);
        editBtn.setBounds (874, 306, 64, 26);
        tweakBtn = &tweakButton (874, 344, 64);

        buildTweak();
        tweakHeight = 300;
        syncFromParams();
        startTimerHz (10);
    }

    juce::String subtitle() const override { return "Echo machine"; }

    void paintFace (juce::Graphics& g) override
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff10292a), 0, 0, juce::Colour (0xff0b1112), (float) getWidth(), (float) getHeight(), false));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 14.0f);
    }

    void paintOverlay (juce::Graphics& g) override
    {
        groupBox (g, { 380, 86, 300, 300 }, "TEMPO, GROOVE & MODE");
        groupBox (g, { 700, 86, 252, 300 });
        caption (g, "STYLE", 716, 290);
    }

private:
    EchoDisplay display;
    LevelMeters meters;
    Led bpmLed;
    juce::TextButton tap { "TAP" }, editBtn { "EDIT" };
    juce::ComboBox styleBox;
    Knob* timeMsKnob = nullptr;
    Knob* timeNoteKnob = nullptr;
    juce::TextButton* tweakBtn = nullptr;
    std::vector<double> taps;

    // Tweak tabs
    juce::TextButton tabMode { "MODE" }, tabStyle { "STYLE EDIT" }, tabRhythm { "RHYTHM EDITOR" };
    juce::Component modePage, stylePage, rhythmPage;
    juce::Component modeSingle, modeDual, modePing, modeRhythm;
    std::unique_ptr<RhythmEditor> rhythmEditor;
    juce::TextButton useGenerated { "USE REPEATS + GRID" }, savePat { "SAVE PATTERN" }, loadPat { "LOAD PATTERN" };
    std::unique_ptr<juce::Label> rhythmHelp;

    void buildTweak()
    {
        const auto c = accent (EffectId::echo);
        juce::TextButton* tabs[3] = { &tabMode, &tabStyle, &tabRhythm };
        for (int i = 0; i < 3; ++i)
        {
            tabs[i]->setColour (juce::TextButton::buttonOnColourId, c);
            tabs[i]->onClick = [this, i] { showTab (i); };
            tweak.addAndMakeVisible (tabs[i]);
            tabs[i]->setBounds (260 + i * 130, 4, 124, 24);
        }
        for (auto* page : { &modePage, &stylePage, &rhythmPage })
        {
            tweak.addChildComponent (page);
            page->setBounds (0, 34, kFaceW, tweakHeightFor());
        }

        // MODE page: one set of controls per mode, shown for the current mode.
        for (auto* m : { &modeSingle, &modeDual, &modePing, &modeRhythm })
        {
            modePage.addChildComponent (m);
            m->setBounds (0, 0, kFaceW, 260);
        }
        knob ("echo_accent", "Accent", 120, 70, 56, &modeSingle);
        knob ("echo_width", "Width", 260, 70, 56, &modeSingle);
        knob ("echo_lroffset", "L/R Offset", 400, 70, 56, &modeSingle);

        knob ("echo_time2", "Time 2", 90, 70, 50, &modeDual);
        knob ("echo_note2", "Note 2", 200, 70, 50, &modeDual).setValueFormatter ([] (float v) { return kNotes[juce::jlimit (0, 7, (int) std::lround (v))]; });
        knob ("echo_accent", "Accent 1", 310, 70, 50, &modeDual);
        knob ("echo_accent2", "Accent 2", 420, 70, 50, &modeDual);
        knob ("echo_fbmix", "Feedback Mix", 530, 70, 50, &modeDual);
        knob ("echo_balance", "Balance", 640, 70, 50, &modeDual);
        knob ("echo_fbbal", "Feedback Bal", 750, 70, 50, &modeDual);
        knob ("echo_width", "Width", 860, 70, 50, &modeDual);

        knob ("echo_width", "Width", 120, 70, 56, &modePing);
        knob ("echo_balance", "Balance", 260, 70, 56, &modePing);

        knob ("echo_rbeats", "Length (beats)", 90, 70, 50, &modeRhythm).setValueFormatter ([] (float v) { return juce::String ((int) std::lround (v)); });
        knob ("echo_grid", "Grid", 200, 70, 50, &modeRhythm).setValueFormatter ([] (float v) { static const char* g[] = { "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" }; return juce::String (g[juce::jlimit (0, 5, (int) std::lround (v))]); });
        knob ("echo_warp", "Warp", 310, 70, 50, &modeRhythm);
        knob ("echo_width", "Width", 420, 70, 50, &modeRhythm);
        knob ("echo_shape", "Shape", 530, 70, 50, &modeRhythm).setValueFormatter ([] (float v) { static const char* s[] = { "Decay", "Reverse", "Swell", "Fade", "Nonlin", "Flat" }; return juce::String (s[juce::jlimit (0, 5, (int) std::lround (v))]); });
        knob ("echo_lroffset", "L/R Offset", 640, 70, 50, &modeRhythm);
        segments ("echo_pan", { "DOUBLE", "CENTER", "ALT 1", "ALT 2", "ALT 3", "SWEEP L", "SWEEP R", "PAN" }, 80, 150, 0, false, &modeRhythm);

        // STYLE EDIT page: the echo's tone, saturation, diffusion and wobble.
        const int r1 = 50, r2 = 150;
        knob ("echo_s_bandlo", "Band Low", 60, r1, 40, &stylePage);
        knob ("echo_s_bandhi", "Band High", 60, r2, 40, &stylePage);
        knob ("echo_s_lowf", "Low Freq", 160, r1, 40, &stylePage);
        knob ("echo_s_lowg", "Low Gain", 250, r1, 40, &stylePage);
        knob ("echo_s_lowdec", "Low Decay", 205, r2, 40, &stylePage);
        knob ("echo_s_midf", "Mid Freq", 340, r1, 40, &stylePage);
        knob ("echo_s_midg", "Mid Gain", 430, r1, 40, &stylePage);
        knob ("echo_s_middec", "Mid Decay", 340, r2, 40, &stylePage);
        knob ("echo_s_midres", "Mid Res", 430, r2, 40, &stylePage);
        knob ("echo_s_highf", "High Freq", 520, r1, 40, &stylePage);
        knob ("echo_s_highg", "High Gain", 610, r1, 40, &stylePage);
        knob ("echo_s_highdec", "High Decay", 565, r2, 40, &stylePage);
        addCaption (stylePage, "SATURATION TYPE", 690, 8);
        choice ("echo_s_sat", 690, 24, 130, 24, &stylePage);
        addCaption (stylePage, "WOBBLE SHAPE", 836, 8);
        choice ("echo_s_wobshape", 836, 24, 120, 24, &stylePage);
        knob ("echo_s_diff", "Diffusion", 720, 110, 40, &stylePage);
        knob ("echo_s_diffsize", "Diff Size", 810, 110, 40, &stylePage);
        segments ("echo_s_diffloop", { "POST", "LOOP" }, 870, 80, 64, true, &stylePage);
        knob ("echo_s_wobdepth", "Wobble", 720, 196, 40, &stylePage);
        knob ("echo_s_wobrate", "Wob Rate", 810, 196, 40, &stylePage);
        toggle ("echo_s_wobsync", "SYNC", 878, 180, 56, &stylePage);

        // RHYTHM EDITOR page.
        rhythmEditor = std::make_unique<RhythmEditor> (proc, c);
        rhythmPage.addAndMakeVisible (*rhythmEditor);
        rhythmEditor->setBounds (20, 10, 740, 180);
        for (auto* b : { &useGenerated, &savePat, &loadPat }) rhythmPage.addAndMakeVisible (b);
        useGenerated.setBounds (780, 10, 170, 28);
        savePat.setBounds (780, 46, 170, 28);
        loadPat.setBounds (780, 82, 170, 28);
        useGenerated.onClick = [this] { rhythmEditor->clearCustom(); };
        savePat.onClick = [this] { rhythmEditor->save(); };
        loadPat.onClick = [this] { rhythmEditor->load(); };
        rhythmHelp = std::make_unique<juce::Label> (juce::String(), u8 ("Click to add a tap  ·  Alt-click to delete · drag to move (Cmd = off the grid) · drag up/down for level. "
                                                       "The yellow dot is the downbeat that repeats when Feedback is up."));
        rhythmHelp->setFont (font (11.0f));
        rhythmHelp->setColour (juce::Label::textColourId, col::dim);
        rhythmHelp->setBounds (20, 196, 930, 36);
        rhythmPage.addAndMakeVisible (*rhythmHelp);

        showTab (0);
    }

    int tweakHeightFor() const { return 300 - 34; }

    juce::OwnedArray<juce::Label> captions;
    void addCaption (juce::Component& parent, const juce::String& text, int x, int y)
    {
        auto* l = captions.add (new juce::Label ({}, text));
        l->setFont (font (10.0f, true).withExtraKerningFactor (0.15f));
        l->setColour (juce::Label::textColourId, col::dim);
        l->setBorderSize ({});
        l->setBounds (x, y, 150, 14);
        parent.addAndMakeVisible (l);
    }

    void showTab (int i)
    {
        juce::TextButton* tabs[3] = { &tabMode, &tabStyle, &tabRhythm };
        juce::Component* pages[3] = { &modePage, &stylePage, &rhythmPage };
        for (int k = 0; k < 3; ++k)
        {
            tabs[k]->setToggleState (k == i, juce::dontSendNotification);
            pages[k]->setVisible (k == i);
        }
    }

    void syncFromParams()
    {
        const int tm = params::geti (state, "echo_timemode");
        const int mode = params::geti (state, "echo_mode");
        timeMsKnob->setVisible (tm == Echo::timeMs);
        timeNoteKnob->setVisible (tm != Echo::timeMs);
        modeSingle.setVisible (mode == Echo::single);
        modeDual.setVisible (mode == Echo::dual);
        modePing.setVisible (mode == Echo::pingPong);
        modeRhythm.setVisible (mode == Echo::rhythm);
        const int style = params::geti (state, "echo_style") + 1;
        if (styleBox.getSelectedId() != style) styleBox.setSelectedId (style, juce::dontSendNotification);
    }

    void timerCallback() override { syncFromParams(); }

    void tapTempo()
    {
        proc.checkpoint();
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (! taps.empty() && now - taps.back() > 2000.0) taps.clear();
        taps.push_back (now);
        if (taps.size() > 5) taps.erase (taps.begin());
        if (taps.size() >= 2)
        {
            const double bpm = 60000.0 * (double) (taps.size() - 1) / (taps.back() - taps.front());
            auto* p = state.getParameter ("echo_bpm");
            p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (30.0, 300.0, bpm)));
            auto* h = state.getParameter ("echo_usehost");
            h->setValueNotifyingHost (0.0f);
        }
    }
};

std::unique_ptr<EffectView> makeEchoView (ProTunezFXToysProcessor& p) { return std::make_unique<EchoView> (p); }
} // namespace ptfx::ui
