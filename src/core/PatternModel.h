/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "GrooveboxProject.h"

namespace SurgeBox
{

// ValueTree type identifiers
namespace IDs
{
inline const juce::Identifier Pattern{"Pattern"};
inline const juce::Identifier Note{"Note"};
inline const juce::Identifier bars{"bars"};
inline const juce::Identifier swing{"swing"};
inline const juce::Identifier startBeat{"startBeat"};
inline const juce::Identifier duration{"duration"};
inline const juce::Identifier pitch{"pitch"};
inline const juce::Identifier velocity{"velocity"};
} // namespace IDs

/**
 * PatternModel wraps pattern data in a ValueTree for automatic undo/redo support.
 * All note operations go through the UndoManager when provided.
 */
class PatternModel : public juce::ValueTree::Listener
{
  public:
    PatternModel();
    explicit PatternModel(juce::UndoManager *undoManager);
    ~PatternModel() override;

    // Set the undo manager (can be shared across multiple patterns)
    void setUndoManager(juce::UndoManager *um) { undoManager_ = um; }
    juce::UndoManager *getUndoManager() const { return undoManager_; }

    // Pattern properties
    int getBars() const;
    void setBars(int bars);
    double getSwing() const;
    void setSwing(double swing);
    double lengthInBeats() const { return getBars() * 4.0; }

    // Note operations (all undoable when UndoManager is set)
    void addNote(double startBeat, double duration, int pitch, int velocity);
    void removeNote(int index);
    void removeNoteAt(double beat, int pitch, double tolerance = 0.01);
    void moveNote(int index, double newStartBeat, int newPitch);
    void resizeNote(int index, double newDuration);
    void setNoteVelocity(int index, int velocity);
    void clear();

    // Batch operations (grouped as single undo)
    void beginTransaction(const juce::String &name);

    // Query
    int getNumNotes() const;
    bool getNoteAt(int index, double &startBeat, double &duration, int &pitch, int &velocity) const;
    int findNoteAt(double beat, int pitch, double tolerance = 0.01) const;
    int findNoteContaining(double beat, int pitch, double tolerance = 0.01) const;

    // For sequencer playback - get notes starting in range
    std::vector<std::tuple<double, double, int, int>> getNotesStartingInRange(double startBeat,
                                                                               double endBeat) const;

    // Convert to/from legacy Pattern struct
    void loadFromPattern(const Pattern &pattern);
    void saveToPattern(Pattern &pattern) const;

    // Set a pattern to auto-sync on every change (for sequencer playback)
    void setAutoSyncPattern(Pattern *pattern) { autoSyncPattern_ = pattern; }

    // Direct ValueTree access (for listeners)
    juce::ValueTree &getValueTree() { return tree_; }
    const juce::ValueTree &getValueTree() const { return tree_; }

    // Listener callback for UI updates
    std::function<void()> onPatternChanged;

  protected:
    // ValueTree::Listener
    void valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child) override;
    void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                               int index) override;
    void valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property) override;

  private:
    juce::ValueTree tree_;
    juce::UndoManager *undoManager_{nullptr};
    Pattern *autoSyncPattern_{nullptr};

    // Helper to create a note ValueTree
    static juce::ValueTree createNoteTree(double startBeat, double duration, int pitch,
                                          int velocity);

    // Sort notes by start beat
    void sortNotes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternModel)
};

} // namespace SurgeBox
