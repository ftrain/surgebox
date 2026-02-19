/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Piano roll rendering utilities.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <set>

namespace SurgeBox
{

class PatternModel;

namespace PianoRoll
{

struct GhostNote
{
    double beat;
    double duration;
    int pitch;
};

struct RenderParams
{
    int noteWidth{18};
    double pixelsPerBeat{60.0};
    double gridSize{0.25};
    int lowestNote{21};
    int highestNote{108};
    const std::vector<int>* visiblePitches{nullptr};
    const std::set<int>* selectedNotes{nullptr};
    bool isChromatic{true};
    int scaleRoot{0};
};

// Draw the grid background
void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& area,
              PatternModel* model, const RenderParams& params);

// Draw notes on the piano roll
void drawNotes(juce::Graphics& g, const juce::Rectangle<int>& area,
               PatternModel* model, const RenderParams& params);

// Draw the playhead line
void drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& area,
                  double playheadBeats, double pixelsPerBeat);

// Draw the step cursor for step recording
void drawStepCursor(juce::Graphics& g, const juce::Rectangle<int>& area,
                    double stepPosition, double pixelsPerBeat);

// Draw box selection rectangle with highlighted grid cells
void drawBoxSelection(juce::Graphics& g, const juce::Point<int>& start,
                      const juce::Point<int>& end, const juce::Rectangle<int>& area,
                      const RenderParams& params);

// Draw ghost notes (translucent loop preview)
void drawGhostNotes(juce::Graphics& g, const juce::Rectangle<int>& area,
                    const std::vector<GhostNote>& ghosts, const RenderParams& params);

// Draw loop region outline (source box + repetition boxes)
void drawLoopRegion(juce::Graphics& g, const juce::Rectangle<int>& area,
                    double loopStartBeat, double loopEndBeat,
                    int minPitch, int maxPitch,
                    double patternLengthBeats, const RenderParams& params,
                    bool selected = false);

// Coordinate conversion helpers
std::pair<double, int> screenToNote(juce::Point<int> pos, const juce::Rectangle<int>& area,
                                    const RenderParams& params);

juce::Rectangle<int> noteToScreen(int noteIndex, const juce::Rectangle<int>& area,
                                  PatternModel* model, const RenderParams& params);

// Helper to convert pitch to column
int pitchToColumn(int pitch, const RenderParams& params);

// Helper to convert column to pitch
int columnToPitch(int column, const RenderParams& params);

} // namespace PianoRoll
} // namespace SurgeBox
