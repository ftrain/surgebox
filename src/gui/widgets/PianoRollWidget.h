/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/GrooveboxProject.h"

namespace SurgeBox
{

class SurgeBoxEngine;

class PianoRollWidget : public juce::Component
{
  public:
    PianoRollWidget();
    ~PianoRollWidget() override = default;

    void setEngine(SurgeBoxEngine *engine);
    void setPattern(Pattern *pattern);

    void paint(juce::Graphics &g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;

  private:
    // Grid settings
    int lowestNote_{36};    // C2
    int highestNote_{84};   // C6
    int noteHeight_{8};
    double pixelsPerBeat_{40.0};
    double viewStartBeat_{0.0};

    // Editing state
    MIDINote *selectedNote_{nullptr};
    MIDINote *draggingNote_{nullptr};
    double dragStartBeat_{0.0};
    int dragStartPitch_{0};
    enum class DragMode
    {
        None,
        Move,
        ResizeEnd
    } dragMode_{DragMode::None};

    // References
    SurgeBoxEngine *engine_{nullptr};
    Pattern *pattern_{nullptr};

    // Drawing helpers
    void drawGrid(juce::Graphics &g, const juce::Rectangle<int> &area);
    void drawNotes(juce::Graphics &g, const juce::Rectangle<int> &area);
    void drawPlayhead(juce::Graphics &g, const juce::Rectangle<int> &area);
    void drawPianoKeys(juce::Graphics &g, const juce::Rectangle<int> &area);

    // Coordinate conversion
    std::pair<double, int> screenToNote(juce::Point<int> pos, const juce::Rectangle<int> &area);
    juce::Rectangle<int> noteToScreen(const MIDINote &note, const juce::Rectangle<int> &area);

    // Piano key area width
    static constexpr int PIANO_KEY_WIDTH = 40;

    // Colors
    juce::Colour bgColor_{0xff1a1a2e};
    juce::Colour gridColor_{0xff2a2a4e};
    juce::Colour beatLineColor_{0xff3a3a5e};
    juce::Colour barLineColor_{0xff5a5a7e};
    juce::Colour noteColor_{0xff00d4ff};
    juce::Colour noteSelectedColor_{0xffff6b6b};
    juce::Colour playheadColor_{0xffffffff};
    juce::Colour whiteKeyColor_{0xff2a2a3e};
    juce::Colour blackKeyColor_{0xff1a1a2e};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWidget)
};

} // namespace SurgeBox
