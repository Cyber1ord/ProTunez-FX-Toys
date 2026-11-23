// Renders every effect screen to PNG exactly as the plug-in draws it (used for checks and progress shots).
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File out (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    out.createDirectory();
    ProTunezFXToysProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    for (int i = 0; i < ptfx::kNumEffects; ++i)
    {
        auto* p = proc.apvts.getParameter ("effect");
        p->setValueNotifyingHost (p->convertTo0to1 ((float) i));
        for (int tweak = 0; tweak < 2; ++tweak)
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
            ed->setSize (1000, 600);
            if (tweak)
            {
                bool any = false;
                std::function<void (juce::Component&)> openTweak = [&] (juce::Component& c)
                {
                    if (auto* v = dynamic_cast<ptfx::ui::EffectView*> (&c)) { v->setTweakOpen (true); any = true; }
                    for (auto* ch : c.getChildren()) openTweak (*ch);
                };
                openTweak (*ed);
                if (! any) continue;
            }
            const auto base = juce::String (i + 1) + "-" + juce::String (ptfx::effectName ((ptfx::EffectId) i)).removeCharacters (" ").toLowerCase();
            auto shoot = [&] (const juce::String& suffix)
            {
                auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
                juce::FileOutputStream os (out.getChildFile (base + suffix + ".png"));
                os.setPosition (0); os.truncate();
                juce::PNGImageFormat().writeImageToStream (img, os);
            };
            shoot (tweak ? "-tweak" : "");
            if (tweak)
            {
                // Extra shots for each Tweak tab, if the screen has tabs.
                std::function<void (juce::Component&)> tabs = [&] (juce::Component& c)
                {
                    for (auto* ch : c.getChildren())
                    {
                        if (auto* b = dynamic_cast<juce::TextButton*> (ch))
                            if (b->getButtonText() == "STYLE EDIT" || b->getButtonText() == "RHYTHM EDITOR" || b->getButtonText() == "EQ")
                            {
                                if (b->onClick) b->onClick();
                                shoot ("-tweak-" + b->getButtonText().removeCharacters (" ").toLowerCase());
                            }
                        tabs (*ch);
                    }
                };
                tabs (*ed);
            }
        }
    }
    return 0;
}
