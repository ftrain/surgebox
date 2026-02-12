/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "NoteSelection.h"
#include "PianoRollRenderer.h"
#include "core/PatternModel.h"
#include <algorithm>
#include <limits>

namespace SurgeBox
{
namespace PianoRoll
{

static std::vector<std::tuple<double, double, int, int>> g_clipboard;

std::vector<std::tuple<double, double, int, int>>& getClipboard()
{
    return g_clipboard;
}

void NoteSelection::selectAll(PatternModel* model)
{
    if (!model)
        return;

    selectedNotes_.clear();
    for (int i = 0; i < model->getNumNotes(); ++i)
        selectedNotes_.insert(i);
}

void NoteSelection::clear()
{
    selectedNotes_.clear();
}

void NoteSelection::validateSelection(PatternModel* model)
{
    if (!model)
    {
        selectedNotes_.clear();
        return;
    }

    std::set<int> validSelection;
    for (int idx : selectedNotes_)
    {
        if (idx < model->getNumNotes())
            validSelection.insert(idx);
    }
    selectedNotes_ = validSelection;
}

void NoteSelection::selectInRect(const juce::Rectangle<int>& screenRect,
                                 const juce::Rectangle<int>& gridArea,
                                 PatternModel* model, const RenderParams& params)
{
    if (!model)
        return;

    for (int i = 0; i < model->getNumNotes(); ++i)
    {
        auto noteRect = noteToScreen(i, gridArea, model, params);
        if (screenRect.intersects(noteRect))
            selectedNotes_.insert(i);
    }
}

void NoteSelection::deleteSelected(PatternModel* model)
{
    if (!model || selectedNotes_.empty())
        return;

    model->beginTransaction("Delete Notes");

    std::vector<int> toDelete(selectedNotes_.begin(), selectedNotes_.end());
    std::sort(toDelete.rbegin(), toDelete.rend());

    for (int idx : toDelete)
        model->removeNote(idx);

    selectedNotes_.clear();
}

void NoteSelection::copy(PatternModel* model)
{
    if (!model || selectedNotes_.empty())
        return;

    g_clipboard.clear();

    double minBeat = std::numeric_limits<double>::max();
    for (int idx : selectedNotes_)
    {
        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(idx, startBeat, duration, pitch, velocity);
        minBeat = std::min(minBeat, startBeat);
    }

    for (int idx : selectedNotes_)
    {
        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(idx, startBeat, duration, pitch, velocity);
        g_clipboard.emplace_back(startBeat - minBeat, duration, pitch, velocity);
    }
}

void NoteSelection::cut(PatternModel* model)
{
    copy(model);
    deleteSelected(model);
}

void NoteSelection::paste(PatternModel* model, double pastePosition)
{
    if (!model || g_clipboard.empty())
        return;

    model->beginTransaction("Paste Notes");

    selectedNotes_.clear();

    for (const auto& [relBeat, duration, pitch, velocity] : g_clipboard)
    {
        double startBeat = pastePosition + relBeat;

        // Remove overlapping notes
        for (int i = model->getNumNotes() - 1; i >= 0; --i)
        {
            double noteStart, noteDur;
            int notePitch, noteVel;
            model->getNoteAt(i, noteStart, noteDur, notePitch, noteVel);

            if (notePitch == pitch)
            {
                double noteEnd = noteStart + noteDur;
                double newEnd = startBeat + duration;
                if (noteStart < newEnd && noteEnd > startBeat)
                {
                    model->removeNote(i);
                }
            }
        }

        model->addNote(startBeat, duration, pitch, velocity);

        int newIndex = model->findNoteAt(startBeat, pitch);
        if (newIndex >= 0)
            selectedNotes_.insert(newIndex);
    }
}

bool NoteSelection::hasClipboard() const
{
    return !g_clipboard.empty();
}

} // namespace PianoRoll
} // namespace SurgeBox
