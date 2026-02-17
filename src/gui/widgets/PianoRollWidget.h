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
#include <functional>

namespace SurgeBox
{

class SurgeBoxEngine;

class PianoRollWidget : public juce::Component
{
  public:
    PianoRollWidget();
    ~PianoRollWidget() override;

    void setEngine(SurgeBoxEngine* engine);
    void setPatternModel(PatternModel* model);
    void setGridSize(double beats) { gridSize_ = beats; repaint(); }
    double getGridSize() const { return gridSize_; }
    double getPixelsPerBeat() const { return pixelsPerBeat_; }
    void setPixelsPerBeat(double ppb) { pixelsPerBeat_ = std::clamp(ppb, 15.0, 120.0); repaint(); }

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

  private:
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
        Erasing
    } dragMode_{DragMode::None};

    double lastDrawnBeat_{-1.0};
    int lastDrawnPitch_{-1};
    int playingNote_{-1};
    juce::Point<int> boxSelectStart_;
    juce::Point<int> boxSelectEnd_;

    // References
    SurgeBoxEngine* engine_{nullptr};
    PatternModel* patternModel_{nullptr};

    // Sequencer playback highlighting
    std::vector<uint8_t> sequencerPlayingNotes_;

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
