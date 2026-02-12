/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Note selection and clipboard management.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <set>
#include <tuple>
#include <vector>

namespace SurgeBox
{

class PatternModel;

namespace PianoRoll
{

struct RenderParams;

class NoteSelection
{
  public:
    NoteSelection() = default;

    // Selection operations
    void selectAll(PatternModel* model);
    void clear();
    void add(int index) { selectedNotes_.insert(index); }
    void remove(int index) { selectedNotes_.erase(index); }
    bool contains(int index) const { return selectedNotes_.count(index) > 0; }
    bool empty() const { return selectedNotes_.empty(); }

    const std::set<int>& getSelection() const { return selectedNotes_; }

    // Validate selection after pattern changes
    void validateSelection(PatternModel* model);

    // Select notes within a screen rectangle
    void selectInRect(const juce::Rectangle<int>& screenRect,
                      const juce::Rectangle<int>& gridArea,
                      PatternModel* model, const RenderParams& params);

    // Delete selected notes
    void deleteSelected(PatternModel* model);

    // Clipboard operations
    void copy(PatternModel* model);
    void cut(PatternModel* model);
    void paste(PatternModel* model, double pastePosition);
    bool hasClipboard() const;

  private:
    std::set<int> selectedNotes_;
};

// Global clipboard (shared across instances)
std::vector<std::tuple<double, double, int, int>>& getClipboard();

} // namespace PianoRoll
} // namespace SurgeBox
