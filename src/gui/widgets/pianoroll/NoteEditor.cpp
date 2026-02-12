/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "NoteEditor.h"
#include "core/PatternModel.h"
#include <algorithm>
#include <cmath>

namespace SurgeBox
{
namespace PianoRoll
{

int NoteEditor::addNote(PatternModel* model, double beat, double duration, int pitch, int velocity)
{
    if (!model)
        return -1;

    model->addNote(beat, duration, pitch, velocity);
    return model->findNoteAt(beat, pitch);
}

void NoteEditor::removeOverlappingNotes(PatternModel* model, int pitch, double startBeat,
                                        double endBeat, int excludeIndex)
{
    if (!model)
        return;

    for (int i = model->getNumNotes() - 1; i >= 0; --i)
    {
        if (i == excludeIndex)
            continue;

        double noteStart, noteDuration;
        int notePitch, noteVelocity;
        model->getNoteAt(i, noteStart, noteDuration, notePitch, noteVelocity);

        if (notePitch != pitch)
            continue;

        double noteEnd = noteStart + noteDuration;

        if (noteStart < endBeat && noteEnd > startBeat)
        {
            model->removeNote(i);
            if (excludeIndex > i)
                excludeIndex--;
        }
    }
}

void NoteEditor::beginMove(PatternModel* model, const std::set<int>& selectedNotes)
{
    originalNotes_.clear();

    if (!model)
        return;

    for (int idx : selectedNotes)
    {
        double noteBeat, noteDur;
        int notePitch, noteVel;
        model->getNoteAt(idx, noteBeat, noteDur, notePitch, noteVel);
        originalNotes_.push_back({noteBeat, notePitch, noteDur, noteVel});
    }
}

void NoteEditor::moveNotes(PatternModel* model, std::set<int>& selectedNotes,
                           double beatDelta, int pitchDelta,
                           int lowestNote, int highestNote)
{
    if (!model || originalNotes_.empty())
        return;

    if (std::abs(beatDelta) < 0.001 && pitchDelta == 0)
        return;

    // Delete all selected notes
    std::vector<int> sortedIndices(selectedNotes.begin(), selectedNotes.end());
    std::sort(sortedIndices.rbegin(), sortedIndices.rend());
    for (int idx : sortedIndices)
    {
        model->removeNote(idx);
    }

    // Re-add at new positions
    selectedNotes.clear();
    for (const auto& origNote : originalNotes_)
    {
        double targetBeat = std::max(0.0, origNote.beat + beatDelta);
        int targetPitch = std::clamp(origNote.pitch + pitchDelta, lowestNote, highestNote - 1);

        model->addNote(targetBeat, origNote.duration, targetPitch, origNote.velocity);
        int newIdx = model->findNoteAt(targetBeat, targetPitch);
        if (newIdx >= 0)
            selectedNotes.insert(newIdx);
    }

    // Update original notes for continued dragging
    originalNotes_.clear();
    for (int idx : selectedNotes)
    {
        double noteBeat, noteDur;
        int notePitch, noteVel;
        model->getNoteAt(idx, noteBeat, noteDur, notePitch, noteVel);
        originalNotes_.push_back({noteBeat, notePitch, noteDur, noteVel});
    }
}

void NoteEditor::endMove(PatternModel* model, std::set<int>& selectedNotes)
{
    if (!model || originalNotes_.empty())
        return;

    // Remove notes that overlap with moved notes
    std::set<int> toRemove;
    for (int selectedIdx : selectedNotes)
    {
        if (selectedIdx >= model->getNumNotes())
            continue;

        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(selectedIdx, startBeat, duration, pitch, velocity);
        double noteEnd = startBeat + duration;

        for (int i = 0; i < model->getNumNotes(); ++i)
        {
            if (selectedNotes.count(i) > 0)
                continue;

            double otherStart, otherDur;
            int otherPitch, otherVel;
            model->getNoteAt(i, otherStart, otherDur, otherPitch, otherVel);

            if (otherPitch != pitch)
                continue;

            double otherEnd = otherStart + otherDur;

            if (otherStart < noteEnd && otherEnd > startBeat)
            {
                toRemove.insert(i);
            }
        }
    }

    for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it)
    {
        model->removeNote(*it);
    }

    // Update selection after removals
    selectedNotes.clear();
    for (const auto& origNote : originalNotes_)
    {
        for (int i = 0; i < model->getNumNotes(); ++i)
        {
            double noteBeat, noteDur;
            int notePitch, noteVel;
            model->getNoteAt(i, noteBeat, noteDur, notePitch, noteVel);

            if (std::abs(noteDur - origNote.duration) < 0.001 &&
                noteVel == origNote.velocity)
            {
                selectedNotes.insert(i);
                break;
            }
        }
    }

    originalNotes_.clear();
}

void NoteEditor::resizeNote(PatternModel* model, int noteIndex, double newDuration)
{
    if (!model || noteIndex < 0 || noteIndex >= model->getNumNotes())
        return;

    double startBeat, duration;
    int pitch, velocity;
    model->getNoteAt(noteIndex, startBeat, duration, pitch, velocity);

    if (newDuration != duration)
    {
        double newEnd = startBeat + newDuration;
        removeOverlappingNotes(model, pitch, startBeat, newEnd, noteIndex);
        model->resizeNote(noteIndex, newDuration);
    }
}

int NoteEditor::addNoteAtStep(PatternModel* model, double stepPosition, double stepSize,
                              int pitch, int velocity)
{
    if (!model)
        return -1;

    model->beginTransaction("Step Record Note");

    removeOverlappingNotes(model, pitch, stepPosition, stepPosition + stepSize);

    model->addNote(stepPosition, stepSize, pitch, velocity);

    return model->findNoteAt(stepPosition, pitch);
}

} // namespace PianoRoll
} // namespace SurgeBox
