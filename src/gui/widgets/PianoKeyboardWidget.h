/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PianoRollWidget.h"
#include <functional>
#include <set>

namespace SurgeBox
{

/**
 * Horizontal piano keyboard that stays fixed at the top while the grid scrolls.
 * Renders note names on C keys and highlights playing notes.
 */
class PianoKeyboardWidget : public juce::Component
{
  public:
    PianoKeyboardWidget();
    ~PianoKeyboardWidget() override = default;

    void paint(juce::Graphics &g) override;

    // Sync with viewport horizontal scroll
    void setScrollOffset(int offset) { scrollOffset_ = offset; repaint(); }
    int getScrollOffset() const { return scrollOffset_; }

    // Set note width to match piano roll
    void setNoteWidth(int width) { noteWidth_ = width; repaint(); }
    int getNoteWidth() const { return noteWidth_; }

    // Scale filtering
    void setScale(int root, ScaleType type);
    void setVisiblePitches(const std::vector<int>& pitches);

    // Drum mode — wider keys with drum voice labels
    void setDrumMode(bool enabled);
    bool isDrumMode() const { return drumMode_; }

    // Note preview callbacks
    std::function<void(int pitch, int velocity)> onNoteOn;
    std::function<void(int pitch)> onNoteOff;

    // Highlight notes from sequencer playback
    void setPlayingNotes(const std::vector<uint8_t>& notes);

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

  private:
    int scrollOffset_{0};
    int noteWidth_{18};
    int lowestNote_{21};    // A0
    int highestNote_{108};  // C8
    int playingNote_{-1};

    // Scale state
    int scaleRoot_{0};
    ScaleType scaleType_{ScaleType::Chromatic};
    std::vector<int> visiblePitches_;

    // Sequencer playback notes
    std::vector<uint8_t> sequencerPlayingNotes_;

    int getPitchAtX(int x) const;
    void playNote(int pitch);
    void stopNote(int pitch);

    // Drum mode
    bool drumMode_{false};

    // Colors
    juce::Colour whiteKeyColor_{0xffe8e8e8};
    juce::Colour blackKeyColor_{0xff3a3a4e};
    juce::Colour playingKeyColor_{0xff00d4ff};
    juce::Colour barLineColor_{0xff5a5a7e};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboardWidget)
};

} // namespace SurgeBox
