/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Note editing operations (add, move, resize).
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <set>
#include <vector>

namespace SurgeBox
{

class PatternModel;

namespace PianoRoll
{

struct OriginalNoteData
{
    double beat;
    int pitch;
    double duration;
    int velocity;
};

class NoteEditor
{
  public:
    NoteEditor() = default;

    // Add a note at a specific position
    int addNote(PatternModel* model, double beat, double duration, int pitch, int velocity);

    // Remove overlapping notes before adding
    void removeOverlappingNotes(PatternModel* model, int pitch, double startBeat, double endBeat,
                                int excludeIndex = -1);

    // Move multiple selected notes by delta
    void beginMove(PatternModel* model, const std::set<int>& selectedNotes);
    void moveNotes(PatternModel* model, std::set<int>& selectedNotes,
                   double beatDelta, int pitchDelta,
                   int lowestNote, int highestNote);
    void endMove(PatternModel* model, std::set<int>& selectedNotes);

    // Resize a note
    void resizeNote(PatternModel* model, int noteIndex, double newDuration);

    // Step recording
    int addNoteAtStep(PatternModel* model, double stepPosition, double stepSize,
                      int pitch, int velocity);

  private:
    std::vector<OriginalNoteData> originalNotes_;
};

} // namespace PianoRoll
} // namespace SurgeBox
