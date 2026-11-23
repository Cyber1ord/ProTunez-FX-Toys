// Simple undo: a snapshot of every parameter is taken when a gesture starts on screen.
// Automation never creates undo steps, and each knob turn or click is one step.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <deque>

namespace ptfx
{
struct HasUndo
{
    virtual ~HasUndo() = default;
    virtual void checkpoint() = 0;
    virtual bool undoLast() = 0;
    virtual bool canUndo() const = 0;
};

class UndoHistory
{
public:
    explicit UndoHistory (juce::AudioProcessor& p) : proc (p) {}

    void checkpoint()
    {
        std::vector<float> snap;
        for (auto* q : proc.getParameters()) snap.push_back (q->getValue());
        if (! steps.empty() && steps.back() == snap) return; // nothing changed since the last step
        steps.push_back (std::move (snap));
        if (steps.size() > 100) steps.pop_front();
    }

    bool undo()
    {
        if (steps.empty()) return false;
        const auto snap = steps.back();
        steps.pop_back();
        auto& ps = proc.getParameters();
        for (size_t i = 0; i < snap.size() && i < (size_t) ps.size(); ++i)
            if (std::abs (ps[(int) i]->getValue() - snap[i]) > 1.0e-7f)
            {
                ps[(int) i]->beginChangeGesture();
                ps[(int) i]->setValueNotifyingHost (snap[i]);
                ps[(int) i]->endChangeGesture();
            }
        return true;
    }

    bool canUndo() const { return ! steps.empty(); }

private:
    juce::AudioProcessor& proc;
    std::deque<std::vector<float>> steps;
};
} // namespace ptfx
