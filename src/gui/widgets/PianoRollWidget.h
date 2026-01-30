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
#include <functional>
#include <set>

namespace SurgeBox
{

class SurgeBoxEngine;

/**
 * Vertical piano roll with time flowing top-to-bottom.
 * Piano keys are horizontal at the TOP to align with Surge's keyboard.
 */
class PianoRollWidget : public juce::Component
{
  public:
    PianoRollWidget();
    ~PianoRollWidget() override;

    void setEngine(SurgeBoxEngine *engine);
    void setPatternModel(PatternModel *model);
    void setGridSize(double beats) { gridSize_ = beats; repaint(); }
    double getGridSize() const { return gridSize_; }
    double getPixelsPerBeat() const { return pixelsPerBeat_; }
    void setPixelsPerBeat(double ppb) { pixelsPerBeat_ = std::clamp(ppb, 15.0, 120.0); repaint(); }

    void paint(juce::Graphics &g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;
    bool keyPressed(const juce::KeyPress &key) override;

    // Selection
    void selectAll();
    void clearSelection();
    void deleteSelected();
    bool hasSelection() const { return !selectedNotes_.empty(); }

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

  private:
    // Grid settings - vertical orientation
    int lowestNote_{21};    // A0 (lowest piano key)
    int highestNote_{108};  // C8 (highest piano key) - 88 keys like a piano
    int noteWidth_{18};     // Width per pitch column (slightly narrower for more notes)
    double pixelsPerBeat_{60.0};  // Default to zoomed in view
    double gridSize_{0.25};

    // Step recording
    bool stepRecordEnabled_{false};
    double stepPosition_{0.0};
    double stepSize_{0.25};
    StepRecordCallback stepRecordCallback_;

    // Multi-selection
    std::set<int> selectedNotes_;

    // Editing state
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

    // Track last drawn/erased position to avoid duplicates
    double lastDrawnBeat_{-1.0};
    int lastDrawnPitch_{-1};

    int playingNote_{-1};

    // Box selection
    juce::Point<int> boxSelectStart_;
    juce::Point<int> boxSelectEnd_;

    // References
    SurgeBoxEngine *engine_{nullptr};
    PatternModel *patternModel_{nullptr};

    // Drawing helpers
    void drawGrid(juce::Graphics &g, const juce::Rectangle<int> &area);
    void drawNotes(juce::Graphics &g, const juce::Rectangle<int> &area);
    void drawPlayhead(juce::Graphics &g, const juce::Rectangle<int> &area);
    void drawPianoKeys(juce::Graphics &g, const juce::Rectangle<int> &area);
    void drawStepCursor(juce::Graphics &g, const juce::Rectangle<int> &area);
    void drawBoxSelection(juce::Graphics &g, const juce::Rectangle<int> &area);

    // Coordinate conversion
    std::pair<double, int> screenToNote(juce::Point<int> pos, const juce::Rectangle<int> &area);
    juce::Rectangle<int> noteToScreen(int noteIndex, const juce::Rectangle<int> &area);
    juce::Rectangle<int> getGridArea() const;
    juce::Rectangle<int> getPianoArea() const;

    // Selection helpers
    bool isNoteSelected(int index) const { return selectedNotes_.count(index) > 0; }
    void selectNotesInRect(const juce::Rectangle<int> &rect, const juce::Rectangle<int> &gridArea);

    // Note overlap handling
    void removeOverlappingNotes(int pitch, double startBeat, double endBeat, int excludeIndex = -1);

    // Piano interaction
    int getPitchAtX(int x) const;
    void playNote(int pitch);
    void stopNote(int pitch);

    // Sequencer playback highlighting
    std::vector<uint8_t> sequencerPlayingNotes_;

    // Piano key area height (at top)
    static constexpr int PIANO_KEY_HEIGHT = 30;

    // Colors
    juce::Colour bgColor_{0xff1a1a2e};
    juce::Colour gridColor_{0xff2a2a4e};
    juce::Colour beatLineColor_{0xff3a3a5e};
    juce::Colour barLineColor_{0xff5a5a7e};
    juce::Colour noteColor_{0xff00d4ff};
    juce::Colour noteSelectedColor_{0xffff6b6b};
    juce::Colour playheadColor_{0xffffffff};
    juce::Colour whiteKeyColor_{0xffe8e8e8};
    juce::Colour blackKeyColor_{0xff3a3a4e};
    juce::Colour playingKeyColor_{0xff00d4ff};
    juce::Colour stepCursorColor_{0xffff4444};
    juce::Colour boxSelectColor_{0x4400d4ff};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWidget)
};

} // namespace SurgeBox
