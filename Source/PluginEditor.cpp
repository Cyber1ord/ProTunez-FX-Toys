#include "PluginEditor.h"

using namespace ptfx;

static constexpr int kW = 1000, kH = 600;

ProTunezFXToysEditor::ProTunezFXToysEditor (ProTunezFXToysProcessor& p) : AudioProcessorEditor (&p), proc (p), header (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (canvas);
    canvas.setSize (kW, kH);
    canvas.addAndMakeVisible (header);
    header.setBounds (0, 0, kW, 54);
    header.onScaleChosen = [this] (float s) { setSize ((int) (kW * s), (int) (kH * s)); };

    showEffect (proc.getActiveEffect());

    setResizable (true, true);
    setResizeLimits ((int) (kW * 0.7f), (int) (kH * 0.7f), (int) (kW * 1.6f), (int) (kH * 1.6f));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kW / kH);
    const float scale = (float) proc.apvts.state.getProperty ("uiScale", 1.0f);
    setSize ((int) (kW * scale), (int) (kH * scale));
    startTimerHz (10);
}

ProTunezFXToysEditor::~ProTunezFXToysEditor() { setLookAndFeel (nullptr); }

void ProTunezFXToysEditor::showEffect (EffectId e)
{
    shown = e;
    view = ui::makeView (e, proc);
    canvas.addAndMakeVisible (*view);
    view->setBounds (ui::kFaceX, ui::kFaceY, ui::kFaceW, ui::kFaceH);
}

void ProTunezFXToysEditor::timerCallback()
{
    if (proc.getActiveEffect() != shown) showEffect (proc.getActiveEffect());
}

void ProTunezFXToysEditor::paint (juce::Graphics& g) { g.fillAll (ui::col::bg); }

void ProTunezFXToysEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    canvas.setTransform (juce::AffineTransform::scale (s));
    proc.apvts.state.setProperty ("uiScale", s, nullptr);
}
