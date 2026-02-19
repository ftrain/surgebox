/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "PianoKeyboardWidget.h"
#include "core/TR808Processor.h"
#include <algorithm>

namespace SurgeBox
{

PianoKeyboardWidget::PianoKeyboardWidget()
{
    setOpaque(true);
    // Initialize with all pitches visible
    for (int pitch = lowestNote_; pitch < highestNote_; ++pitch)
        visiblePitches_.push_back(pitch);
}

void PianoKeyboardWidget::setScale(int root, ScaleType type)
{
    scaleRoot_ = root % 12;
    scaleType_ = type;
    repaint();
}

void PianoKeyboardWidget::setVisiblePitches(const std::vector<int>& pitches)
{
    visiblePitches_ = pitches;
    repaint();
}

void PianoKeyboardWidget::setDrumMode(bool enabled)
{
    drumMode_ = enabled;
    repaint();
}

void PianoKeyboardWidget::setPlayingNotes(const std::vector<uint8_t>& notes)
{
    sequencerPlayingNotes_ = notes;
    repaint();
}

int PianoKeyboardWidget::getPitchAtX(int x) const
{
    int adjustedX = x + scrollOffset_;
    int columnIndex = adjustedX / noteWidth_;

    if (columnIndex >= 0 && columnIndex < static_cast<int>(visiblePitches_.size()))
        return visiblePitches_[columnIndex];
    return -1;
}

void PianoKeyboardWidget::playNote(int pitch)
{
    if (onNoteOn && pitch >= 0)
        onNoteOn(pitch, 100);
    playingNote_ = pitch;
    repaint();
}

void PianoKeyboardWidget::stopNote(int pitch)
{
    if (onNoteOff && pitch >= 0)
        onNoteOff(pitch);
    if (playingNote_ == pitch)
        playingNote_ = -1;
    repaint();
}

void PianoKeyboardWidget::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds();
    g.fillAll(juce::Colour(0xff1a1a2e));

    int numNotes = static_cast<int>(visiblePitches_.size());

    for (int i = 0; i < numNotes; i++)
    {
        int pitch = visiblePitches_[i];
        int x = (i * noteWidth_) - scrollOffset_;

        // Skip if off-screen
        if (x + noteWidth_ < 0 || x > bounds.getWidth())
            continue;

        int noteInOctave = pitch % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                          noteInOctave == 8 || noteInOctave == 10);

        // Check if note is playing from sequencer or mouse
        bool isSequencerPlaying = std::find(sequencerPlayingNotes_.begin(),
                                            sequencerPlayingNotes_.end(),
                                            static_cast<uint8_t>(pitch)) != sequencerPlayingNotes_.end();

        if (pitch == playingNote_ || isSequencerPlaying)
            g.setColour(playingKeyColor_);
        else
            g.setColour(isBlackKey ? blackKeyColor_ : whiteKeyColor_);

        g.fillRect(x, bounds.getY(), noteWidth_ - 1, bounds.getHeight());

        if (drumMode_)
        {
            const char* name = TR808Processor::nameForMidiNote(pitch);
            if (name[0] != '\0')
            {
                g.setColour(juce::Colours::black);
                g.setFont(10.0f);
                g.drawText(juce::String(name), x + 1, bounds.getY(),
                           noteWidth_ - 2, bounds.getHeight(), juce::Justification::centred);
            }
        }
        else if (noteInOctave == 0)
        {
            int octave = (pitch / 12) - 1;
            g.setColour(juce::Colours::black);
            g.setFont(9.0f);
            g.drawText(juce::String("C") + juce::String(octave), x, bounds.getY(),
                       noteWidth_, bounds.getHeight(), juce::Justification::centred);
        }
    }

    // Bottom border
    g.setColour(barLineColor_);
    g.drawHorizontalLine(bounds.getBottom() - 1, static_cast<float>(bounds.getX()),
                        static_cast<float>(bounds.getRight()));
}

void PianoKeyboardWidget::mouseDown(const juce::MouseEvent &e)
{
    int pitch = getPitchAtX(e.x);
    if (pitch >= 0)
        playNote(pitch);
}

void PianoKeyboardWidget::mouseDrag(const juce::MouseEvent &e)
{
    int pitch = getPitchAtX(e.x);
    if (pitch != playingNote_)
    {
        if (playingNote_ >= 0)
            stopNote(playingNote_);
        if (pitch >= 0)
            playNote(pitch);
    }
}

void PianoKeyboardWidget::mouseUp(const juce::MouseEvent &)
{
    if (playingNote_ >= 0)
        stopNote(playingNote_);
}

} // namespace SurgeBox
