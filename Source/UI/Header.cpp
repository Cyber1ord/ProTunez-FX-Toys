#include "Header.h"

namespace ptfx::ui
{
Header::Header (ProTunezFXToysProcessor& p) : proc (p), bypass (p.apvts, "bypass", "BYPASS", col::brand)
{
    // Effect menu: all 8, the unbuilt ones greyed out with "coming soon".
    for (int i = 0; i < kNumEffects; ++i)
    {
        const auto e = (EffectId) i;
        effectBox.addItem (juce::String (effectName (e)) + "      like " + effectLike (e) + (isBuilt (e) ? "" : "  (coming soon)"), i + 1);
        effectBox.setItemEnabled (i + 1, isBuilt (e));
    }
    effectBox.onChange = [this]
    {
        if (effectAttachment) effectAttachment->setValueAsCompleteGesture ((float) (effectBox.getSelectedId() - 1));
    };
    effectAttachment = std::make_unique<juce::ParameterAttachment> (
        *p.apvts.getParameter ("effect"),
        [this] (float v)
        {
            const int id = (int) std::lround (v) + 1;
            effectBox.setSelectedId (id, juce::dontSendNotification);
            effectBox.setColour (juce::ComboBox::outlineColourId, accent ((EffectId) (id - 1)));
        },
        nullptr);
    effectAttachment->sendInitialUpdate();
    addAndMakeVisible (effectBox);

    prev.onClick = [this] { proc.presets.previous(); };
    next.onClick = [this] { proc.presets.next(); };
    presetName.onClick = [this] { showPresetMenu(); };
    presetName.setColour (juce::TextButton::buttonColourId, col::led);
    save.onClick = [this] { askSaveName(); };
    revert.onClick = [this] { proc.presets.revert(); };
    undoBtn.onClick = [this] { proc.undo.undo(); };
    // A/B: clicking the other slot stores the current sound in this one and recalls the other.
    for (auto* btn : { &a, &b }) btn->setClickingTogglesState (false);
    a.setToggleState (true, juce::dontSendNotification);
    a.onClick = [this] { if (abSlot != 0) { proc.presets.storeAB (1); proc.presets.recallAB (0); abSlot = 0; } a.setToggleState (true, juce::dontSendNotification); b.setToggleState (false, juce::dontSendNotification); };
    b.onClick = [this] { if (abSlot != 1) { proc.presets.storeAB (0); proc.presets.recallAB (1); abSlot = 1; } b.setToggleState (true, juce::dontSendNotification); a.setToggleState (false, juce::dontSendNotification); };
    settings.onClick = [this]
    {
        juce::PopupMenu m;
        m.addSectionHeader ("Window size");
        const float sizes[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        for (int i = 0; i < 4; ++i) m.addItem (i + 1, juce::String ((int) (sizes[i] * 100)) + " %");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (settings), [this, sizes] (int r)
                         { if (r > 0 && onScaleChosen) onScaleChosen (sizes[r - 1]); });
    };
    for (auto* c : std::initializer_list<juce::Component*> { &prev, &next, &presetName, &save, &revert, &undoBtn, &a, &b, &settings, &bypass })
        addAndMakeVisible (c);
    startTimerHz (8);
}

void Header::timerCallback()
{
    const auto name = proc.presets.currentName();
    if (presetName.getButtonText() != name) presetName.setButtonText (name);
    presetName.setColour (juce::TextButton::textColourOffId, accent (proc.getActiveEffect()));
    undoBtn.setEnabled (proc.undo.canUndo());
}

void Header::showPresetMenu()
{
    const auto e = proc.getActiveEffect();
    juce::PopupMenu m;
    int id = 1;
    std::vector<std::pair<juce::String, juce::String>> map;
    for (auto& cat : proc.presets.categories (e))
    {
        juce::PopupMenu sub;
        for (auto& n : proc.presets.presetsIn (e, cat))
        {
            sub.addItem (id++, n, true, cat == proc.presets.currentCategory() && n == proc.presets.currentName());
            map.push_back ({ cat, n });
        }
        if (cat == "User" && map.empty() == false && sub.getNumItems() == 0) sub.addItem (-1, "(none saved yet)", false);
        m.addSubMenu (cat, sub);
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetName), [this, e, map] (int r)
                     { if (r > 0 && r <= (int) map.size()) proc.presets.load (e, map[(size_t) r - 1].first, map[(size_t) r - 1].second); });
}

void Header::askSaveName()
{
    auto* w = new juce::AlertWindow ("Save preset", "Name for your " + juce::String (effectName (proc.getActiveEffect())) + " preset:",
                                     juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.presets.currentName());
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
    {
        if (r == 1) proc.presets.saveUser (proc.getActiveEffect(), w->getTextEditorContents ("name"));
    }), true);
}

void Header::paint (juce::Graphics& g)
{
    g.fillAll (col::headerBg);
    g.setColour (col::edge);
    g.drawLine (0, (float) getHeight() - 0.5f, (float) getWidth(), (float) getHeight() - 0.5f);
    g.setFont (font (18.0f, true));
    g.setColour (col::text);
    g.drawText ("ProTunez", 14, 0, 90, getHeight(), juce::Justification::centredLeft);
    g.setColour (col::brand);
    g.drawText ("FX Toys", 96, 0, 80, getHeight(), juce::Justification::centredLeft);
}

void Header::resized()
{
    const int y = 12, h = 30;
    effectBox.setBounds (178, y - 1, 250, h + 2);
    prev.setBounds (440, y, 28, h);
    presetName.setBounds (472, y, 200, h);
    next.setBounds (676, y, 28, h);
    save.setBounds (710, y, 46, h);
    revert.setBounds (760, y, 54, h);
    undoBtn.setBounds (818, y, 46, h);
    a.setBounds (868, y, 22, h);
    b.setBounds (892, y, 22, h);
    settings.setBounds (918, y, 36, h);
    bypass.setBounds (958, y, 34, h);
    bypass.setButtonText ("BYP");
}
} // namespace ptfx::ui
