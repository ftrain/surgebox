/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/PatternModel.h"
#include "core/MusicTheory.h"
#include "pianoroll/NoteSelection.h"
#include "pianoroll/NoteEditor.h"
#include "pianoroll/PianoRollRenderer.h"
#include "pianoroll/SelectionToolbar.h"
#include <functional>
#include <memory>

namespace SurgeBox
{

class SurgeBoxEngine;
class GridLayer;
class NoteLayer;
class PlayheadLayer;
class LoopLayer;
class GhostNoteLayer;
class OverlayLayer;

class PianoRollWidget : public juce::Component
{
  public:
    PianoRollWidget();
    ~PianoRollWidget() override;

    void setEngine(SurgeBoxEngine* engine);
    void setPatternModel(PatternModel* model);
    void setGridSize(double beats);
    double getGridSize() const { return gridSize_; }
    double getPixelsPerBeat() const { return pixelsPerBeat_; }
    void setPixelsPerBeat(double ppb);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // Selection
    void selectAll();
    void clearSelection();
    void deleteSelected();
    bool hasSelection() const { return !selection_.empty(); }

    // Step recording
    using StepRecordCallback = std::function<void(int pitch, int velocity)>;
    void setStepRecordCallback(StepRecordCallback callback) { stepRecordCallback_ = callback; }
    void setStepRecordEnabled(bool enabled);
    void resetStepPosition();
    void addNoteAtCurrentStep(int pitch, int velocity);
    double getStepPosition() const { return stepPosition_; }

    // Callback for playing notes (piano key clicks)
    std::function<void(int pitch, int velocity)> onNoteOn;
    std::function<void(int pitch)> onNoteOff;

    // Scale filtering
    void setScale(int root, ScaleType type);
    int getScaleRoot() const { return scaleRoot_; }
    ScaleType getScaleType() const { return scaleType_; }
    bool isNoteInScale(int pitch) const;
    const std::vector<int>& getVisiblePitches() const { return visiblePitches_; }
    int getVisibleNoteCount() const { return static_cast<int>(visiblePitches_.size()); }

    // Restrict pitches to a fixed set (for drum machines). Pass empty to clear.
    void setFixedPitches(const std::vector<int>& pitches);
    bool isDrumMode() const { return !fixedPitches_.empty(); }

    int getNoteWidth() const { return noteWidth_; }
    void setNoteWidth(int w);

    using GhostNote = PianoRoll::GhostNote;
    const std::vector<GhostNote>& getGhostNotes() const { return ghostNotes_; }

    // Playhead control (called from timer instead of full repaint)
    void updatePlayhead(double beats);
    void hidePlayhead();

    // Rebuild chord shading from engine's chord progression
    void rebuildChordShadings();

  private:
    // Layers
    std::unique_ptr<GridLayer> gridLayer_;
    std::unique_ptr<NoteLayer> noteLayer_;
    std::unique_ptr<PlayheadLayer> playheadLayer_;
    std::unique_ptr<LoopLayer> loopLayer_;
    std::unique_ptr<GhostNoteLayer> ghostNoteLayer_;
    std::unique_ptr<OverlayLayer> overlayLayer_;

    // Push current render params to all layers
    void pushRenderParams();

    // Selection toolbar
    std::unique_ptr<PianoRoll::SelectionToolbar> selectionToolbar_;
    void showSelectionToolbar(juce::Point<int> position);
    void hideSelectionToolbar();
    void rebuildGhostNotes();

    // Toolbar operations
    void performLoop();
    void performInvert();
    void performHalveDuration();
    void performDoubleDuration();

    // Ghost notes for loop preview
    std::vector<GhostNote> ghostNotes_;

    // Chord shading data (rebuilt when chord track changes)
    std::vector<PianoRoll::ChordShading> chordShadings_;

    // Grid settings
    int lowestNote_{21};
    int highestNote_{108};
    int noteWidth_{18};
    double pixelsPerBeat_{60.0};
    double gridSize_{0.25};

    // Step recording
    bool stepRecordEnabled_{false};
    double stepPosition_{0.0};
    double stepSize_{0.25};
    StepRecordCallback stepRecordCallback_;

    // Scale filtering
    int scaleRoot_{0};
    ScaleType scaleType_{ScaleType::Chromatic};
    std::vector<int> visiblePitches_;
    std::vector<int> fixedPitches_;
    void rebuildVisiblePitches();

    // Selection and editing
    PianoRoll::NoteSelection selection_;
    PianoRoll::NoteEditor editor_;

    // Dragging state
    int draggingNoteIndex_{-1};
    double dragStartBeat_{0.0};
    int dragStartPitch_{0};
    double dragStartDuration_{0.0};
    double originalNoteBeat_{0.0};
    int originalNotePitch_{0};

    enum class DragMode
    {
        None,
        Move,
        ResizeEnd,
        BoxSelect,
        PlayingPiano,
        Drawing,
        Erasing,
        LoopResize
    } dragMode_{DragMode::None};

    double lastDrawnBeat_{-1.0};
    int lastDrawnPitch_{-1};
    int playingNote_{-1};
    juce::Point<int> boxSelectStart_;
    juce::Point<int> boxSelectEnd_;
    double boxSelectBeatStart_{0.0};
    double boxSelectBeatEnd_{0.0};
    int boxSelectMinPitch_{0};
    int boxSelectMaxPitch_{127};

    // Check if a beat/pitch falls in the looped (ghost) region
    bool isInLoopedRegion(double beat, int pitch) const;

    // Loop interaction
    int activeLoopIndex_{-1};
    enum class LoopEdge { None, Top, Bottom, Left, Right } resizingEdge_{LoopEdge::None};
    void selectLoop(int index, juce::Point<int> position);
    void deselectLoop();
    void deleteActiveLoop();
    LoopEdge findLoopEdge(int loopIndex, juce::Point<int> pos, int tolerance = 6) const;
    juce::Rectangle<int> loopRegionToScreen(int loopIndex) const;

    // References
    SurgeBoxEngine* engine_{nullptr};
    PatternModel* patternModel_{nullptr};

    // Helper to build render params
    PianoRoll::RenderParams buildRenderParams() const;

    // Grid area (full widget now that piano keys are separate)
    juce::Rectangle<int> getGridArea() const { return getLocalBounds(); }

    // Coordinate helpers (delegating to renderer)
    std::pair<double, int> screenToNote(juce::Point<int> pos);
    juce::Rectangle<int> noteToScreen(int noteIndex);
    int pitchToColumn(int pitch) const;
    int columnToPitch(int column) const;

    // Piano interaction
    int getPitchAtX(int x) const;
    void playNote(int pitch);
    void stopNote(int pitch);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWidget)
};

} // namespace SurgeBox
