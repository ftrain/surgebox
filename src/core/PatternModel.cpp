/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "PatternModel.h"
#include <algorithm>

namespace SurgeBox
{

PatternModel::PatternModel() : tree_(IDs::Pattern)
{
    tree_.setProperty(IDs::bars, 4, nullptr);
    tree_.setProperty(IDs::swing, 0.0, nullptr);
    tree_.addListener(this);
}

PatternModel::PatternModel(juce::UndoManager *undoManager)
    : tree_(IDs::Pattern), undoManager_(undoManager)
{
    tree_.setProperty(IDs::bars, 4, nullptr);
    tree_.setProperty(IDs::swing, 0.0, nullptr);
    tree_.addListener(this);
}

PatternModel::~PatternModel() { tree_.removeListener(this); }

int PatternModel::getBars() const { return tree_.getProperty(IDs::bars, 4); }

void PatternModel::setBars(int bars) { tree_.setProperty(IDs::bars, bars, undoManager_); }

double PatternModel::getSwing() const { return tree_.getProperty(IDs::swing, 0.0); }

void PatternModel::setSwing(double swing) { tree_.setProperty(IDs::swing, swing, undoManager_); }

juce::ValueTree PatternModel::createNoteTree(double startBeat, double duration, int pitch,
                                              int velocity)
{
    juce::ValueTree note(IDs::Note);
    note.setProperty(IDs::startBeat, startBeat, nullptr);
    note.setProperty(IDs::duration, duration, nullptr);
    note.setProperty(IDs::pitch, pitch, nullptr);
    note.setProperty(IDs::velocity, velocity, nullptr);
    return note;
}

void PatternModel::addNote(double startBeat, double duration, int pitch, int velocity)
{
    auto note = createNoteTree(startBeat, duration, pitch, velocity);
    tree_.appendChild(note, undoManager_);
    sortNotes();
}

void PatternModel::removeNote(int index)
{
    if (index >= 0 && index < tree_.getNumChildren())
    {
        tree_.removeChild(index, undoManager_);
    }
}

void PatternModel::removeNoteAt(double beat, int pitch, double tolerance)
{
    for (int i = tree_.getNumChildren() - 1; i >= 0; --i)
    {
        auto note = tree_.getChild(i);
        double noteStart = note.getProperty(IDs::startBeat);
        int notePitch = note.getProperty(IDs::pitch);

        if (notePitch == pitch && std::abs(noteStart - beat) < tolerance)
        {
            tree_.removeChild(i, undoManager_);
        }
    }
}

void PatternModel::moveNote(int index, double newStartBeat, int newPitch)
{
    if (index >= 0 && index < tree_.getNumChildren())
    {
        auto note = tree_.getChild(index);
        note.setProperty(IDs::startBeat, newStartBeat, undoManager_);
        note.setProperty(IDs::pitch, newPitch, undoManager_);
        sortNotes();
    }
}

void PatternModel::resizeNote(int index, double newDuration)
{
    if (index >= 0 && index < tree_.getNumChildren())
    {
        auto note = tree_.getChild(index);
        note.setProperty(IDs::duration, std::max(0.0625, newDuration), undoManager_);
    }
}

void PatternModel::setNoteVelocity(int index, int velocity)
{
    if (index >= 0 && index < tree_.getNumChildren())
    {
        auto note = tree_.getChild(index);
        note.setProperty(IDs::velocity, std::clamp(velocity, 1, 127), undoManager_);
    }
}

void PatternModel::clear()
{
    // Remove all notes
    while (tree_.getNumChildren() > 0)
    {
        tree_.removeChild(0, undoManager_);
    }
}

void PatternModel::beginTransaction(const juce::String &name)
{
    if (undoManager_)
        undoManager_->beginNewTransaction(name);
}

int PatternModel::getNumNotes() const { return tree_.getNumChildren(); }

bool PatternModel::getNoteAt(int index, double &startBeat, double &duration, int &pitch,
                              int &velocity) const
{
    if (index < 0 || index >= tree_.getNumChildren())
        return false;

    auto note = tree_.getChild(index);
    startBeat = note.getProperty(IDs::startBeat);
    duration = note.getProperty(IDs::duration);
    pitch = note.getProperty(IDs::pitch);
    velocity = note.getProperty(IDs::velocity);
    return true;
}

int PatternModel::findNoteAt(double beat, int pitch, double tolerance) const
{
    for (int i = 0; i < tree_.getNumChildren(); ++i)
    {
        auto note = tree_.getChild(i);
        double noteStart = note.getProperty(IDs::startBeat);
        int notePitch = note.getProperty(IDs::pitch);

        if (notePitch == pitch && std::abs(noteStart - beat) < tolerance)
            return i;
    }
    return -1;
}

int PatternModel::findNoteContaining(double beat, int pitch, double tolerance) const
{
    for (int i = 0; i < tree_.getNumChildren(); ++i)
    {
        auto note = tree_.getChild(i);
        double noteStart = note.getProperty(IDs::startBeat);
        double noteDuration = note.getProperty(IDs::duration);
        int notePitch = note.getProperty(IDs::pitch);

        // Tolerance only applies to start (so you can click slightly before a note to select it)
        // No tolerance on the end - prevents matching adjacent notes
        if (notePitch == pitch && beat >= noteStart - tolerance &&
            beat < noteStart + noteDuration)
        {
            return i;
        }
    }
    return -1;
}

std::vector<std::tuple<double, double, int, int>>
PatternModel::getNotesStartingInRange(double startBeat, double endBeat) const
{
    std::vector<std::tuple<double, double, int, int>> result;

    for (int i = 0; i < tree_.getNumChildren(); ++i)
    {
        auto note = tree_.getChild(i);
        double noteStart = note.getProperty(IDs::startBeat);

        if (noteStart >= startBeat && noteStart < endBeat)
        {
            double duration = note.getProperty(IDs::duration);
            int pitch = note.getProperty(IDs::pitch);
            int velocity = note.getProperty(IDs::velocity);
            result.emplace_back(noteStart, duration, pitch, velocity);
        }
    }
    return result;
}

void PatternModel::loadFromPattern(const Pattern &pattern)
{
    // Clear existing notes without undo
    while (tree_.getNumChildren() > 0)
        tree_.removeChild(0, nullptr);

    tree_.setProperty(IDs::bars, pattern.bars, nullptr);
    tree_.setProperty(IDs::swing, pattern.swing, nullptr);

    for (const auto &note : pattern.notes)
    {
        auto noteTree =
            createNoteTree(note.startBeat, note.duration, note.pitch, note.velocity);
        tree_.appendChild(noteTree, nullptr);
    }
}

void PatternModel::saveToPattern(Pattern &pattern) const
{
    pattern.bars = getBars();
    pattern.swing = getSwing();
    pattern.notes.clear();

    for (int i = 0; i < tree_.getNumChildren(); ++i)
    {
        auto note = tree_.getChild(i);
        MIDINote midiNote;
        midiNote.startBeat = note.getProperty(IDs::startBeat);
        midiNote.duration = note.getProperty(IDs::duration);
        midiNote.pitch = static_cast<uint8_t>(static_cast<int>(note.getProperty(IDs::pitch)));
        midiNote.velocity =
            static_cast<uint8_t>(static_cast<int>(note.getProperty(IDs::velocity)));
        pattern.notes.push_back(midiNote);
    }
    pattern.sortNotes();
}

void PatternModel::sortNotes()
{
    // Collect notes with their start beats
    std::vector<std::pair<double, juce::ValueTree>> notesWithBeats;
    for (int i = 0; i < tree_.getNumChildren(); ++i)
    {
        auto note = tree_.getChild(i);
        double start = note.getProperty(IDs::startBeat);
        notesWithBeats.emplace_back(start, note);
    }

    // Sort by start beat
    std::sort(notesWithBeats.begin(), notesWithBeats.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    // Check if already sorted
    bool needsSort = false;
    for (int i = 0; i < static_cast<int>(notesWithBeats.size()); ++i)
    {
        if (tree_.getChild(i) != notesWithBeats[i].second)
        {
            needsSort = true;
            break;
        }
    }

    if (needsSort)
    {
        // Reorder without creating undo entries for the sort itself
        for (int i = 0; i < static_cast<int>(notesWithBeats.size()); ++i)
        {
            tree_.moveChild(tree_.indexOf(notesWithBeats[i].second), i, nullptr);
        }
    }
}

void PatternModel::valueTreeChildAdded(juce::ValueTree & /*parent*/, juce::ValueTree & /*child*/)
{
    // Auto-sync to legacy pattern for sequencer playback
    if (autoSyncPattern_)
        saveToPattern(*autoSyncPattern_);

    if (onPatternChanged)
        onPatternChanged();
}

void PatternModel::valueTreeChildRemoved(juce::ValueTree & /*parent*/, juce::ValueTree & /*child*/,
                                          int /*index*/)
{
    // Auto-sync to legacy pattern for sequencer playback
    if (autoSyncPattern_)
        saveToPattern(*autoSyncPattern_);

    if (onPatternChanged)
        onPatternChanged();
}

void PatternModel::valueTreePropertyChanged(juce::ValueTree & /*tree*/,
                                             const juce::Identifier & /*property*/)
{
    // Auto-sync to legacy pattern for sequencer playback
    if (autoSyncPattern_)
        saveToPattern(*autoSyncPattern_);

    if (onPatternChanged)
        onPatternChanged();
}

} // namespace SurgeBox
