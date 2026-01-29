/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "PianoRollWidget.h"
#include "core/SurgeBoxEngine.h"

namespace SurgeBox
{

PianoRollWidget::PianoRollWidget() { setOpaque(true); }

void PianoRollWidget::setEngine(SurgeBoxEngine *engine)
{
    engine_ = engine;
    if (engine_)
    {
        auto &project = engine_->getProject();
        int activeVoice = engine_->getActiveVoice();
        pattern_ = &project.voices[activeVoice].pattern;
    }
}

void PianoRollWidget::setPattern(Pattern *pattern)
{
    pattern_ = pattern;
    repaint();
}

void PianoRollWidget::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(bgColor_);

    // Piano keys on left
    auto pianoArea = bounds.removeFromLeft(PIANO_KEY_WIDTH);
    auto gridArea = bounds;

    drawPianoKeys(g, pianoArea);
    drawGrid(g, gridArea);
    drawNotes(g, gridArea);
    drawPlayhead(g, gridArea);
}

void PianoRollWidget::resized()
{
    // Calculate note height based on visible range
    int numNotes = highestNote_ - lowestNote_;
    if (numNotes > 0)
        noteHeight_ = std::max(4, getHeight() / numNotes);
}

void PianoRollWidget::drawPianoKeys(juce::Graphics &g, const juce::Rectangle<int> &area)
{
    int numNotes = highestNote_ - lowestNote_;

    for (int i = 0; i < numNotes; i++)
    {
        int note = highestNote_ - 1 - i;
        int y = area.getY() + (i * noteHeight_);

        // Determine if black or white key
        int noteInOctave = note % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                          noteInOctave == 8 || noteInOctave == 10);

        g.setColour(isBlackKey ? blackKeyColor_ : whiteKeyColor_);
        g.fillRect(area.getX(), y, area.getWidth(), noteHeight_);

        // Draw note name for C notes
        if (noteInOctave == 0)
        {
            int octave = (note / 12) - 1;
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.setFont(10.0f);
            g.drawText(juce::String("C") + juce::String(octave), area.getX() + 2, y,
                       area.getWidth() - 4, noteHeight_, juce::Justification::centredLeft);
        }

        // Border
        g.setColour(gridColor_);
        g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                            static_cast<float>(area.getRight()));
    }
}

void PianoRollWidget::drawGrid(juce::Graphics &g, const juce::Rectangle<int> &area)
{
    if (!pattern_)
        return;

    int numNotes = highestNote_ - lowestNote_;
    double totalBeats = pattern_->lengthInBeats();

    // Horizontal lines (notes)
    for (int i = 0; i <= numNotes; i++)
    {
        int y = area.getY() + (i * noteHeight_);
        int note = highestNote_ - i;
        int noteInOctave = note % 12;

        bool isC = (noteInOctave == 0);
        g.setColour(isC ? barLineColor_ : gridColor_);
        g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                            static_cast<float>(area.getRight()));
    }

    // Vertical lines (beats)
    for (double beat = 0; beat <= totalBeats; beat += 0.25)
    {
        int x = area.getX() + static_cast<int>((beat - viewStartBeat_) * pixelsPerBeat_);
        if (x < area.getX() || x > area.getRight())
            continue;

        bool isBar = (static_cast<int>(beat) % 4 == 0 && beat == static_cast<int>(beat));
        bool isBeat = (beat == static_cast<int>(beat));

        if (isBar)
            g.setColour(barLineColor_);
        else if (isBeat)
            g.setColour(beatLineColor_);
        else
            g.setColour(gridColor_);

        g.drawVerticalLine(x, static_cast<float>(area.getY()),
                          static_cast<float>(area.getBottom()));
    }
}

void PianoRollWidget::drawNotes(juce::Graphics &g, const juce::Rectangle<int> &area)
{
    if (!pattern_)
        return;

    for (const auto &note : pattern_->notes)
    {
        auto noteRect = noteToScreen(note, area);
        if (noteRect.isEmpty() || !noteRect.intersects(area))
            continue;

        // Note fill
        bool selected = (selectedNote_ && selectedNote_->pitch == note.pitch &&
                        selectedNote_->startBeat == note.startBeat);
        g.setColour(selected ? noteSelectedColor_ : noteColor_);
        g.fillRoundedRectangle(noteRect.toFloat(), 2.0f);

        // Note border
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(noteRect.toFloat(), 2.0f, 1.0f);

        // Velocity indicator (darker = lower velocity)
        float velAlpha = 1.0f - (static_cast<float>(note.velocity) / 127.0f) * 0.5f;
        g.setColour(juce::Colours::black.withAlpha(velAlpha * 0.3f));
        g.fillRoundedRectangle(noteRect.toFloat(), 2.0f);
    }
}

void PianoRollWidget::drawPlayhead(juce::Graphics &g, const juce::Rectangle<int> &area)
{
    if (!engine_ || !engine_->isPlaying())
        return;

    double playhead = engine_->getPlayheadBeats();
    int x = area.getX() + static_cast<int>((playhead - viewStartBeat_) * pixelsPerBeat_);

    if (x >= area.getX() && x <= area.getRight())
    {
        g.setColour(playheadColor_);
        g.drawVerticalLine(x, static_cast<float>(area.getY()),
                          static_cast<float>(area.getBottom()));

        // Playhead triangle at top
        juce::Path triangle;
        triangle.addTriangle(static_cast<float>(x - 5), static_cast<float>(area.getY()),
                            static_cast<float>(x + 5), static_cast<float>(area.getY()),
                            static_cast<float>(x), static_cast<float>(area.getY() + 8));
        g.fillPath(triangle);
    }
}

std::pair<double, int> PianoRollWidget::screenToNote(juce::Point<int> pos,
                                                      const juce::Rectangle<int> &area)
{
    double beat = viewStartBeat_ + (pos.x - area.getX()) / pixelsPerBeat_;
    int noteIndex = (pos.y - area.getY()) / noteHeight_;
    int pitch = highestNote_ - 1 - noteIndex;

    return {beat, std::clamp(pitch, lowestNote_, highestNote_ - 1)};
}

juce::Rectangle<int> PianoRollWidget::noteToScreen(const MIDINote &note,
                                                    const juce::Rectangle<int> &area)
{
    int x = area.getX() + static_cast<int>((note.startBeat - viewStartBeat_) * pixelsPerBeat_);
    int w = static_cast<int>(note.duration * pixelsPerBeat_);
    int noteIndex = highestNote_ - 1 - note.pitch;
    int y = area.getY() + (noteIndex * noteHeight_);

    return juce::Rectangle<int>(x, y, std::max(4, w), noteHeight_ - 1);
}

void PianoRollWidget::mouseDown(const juce::MouseEvent &e)
{
    if (!pattern_)
        return;

    auto gridArea = getLocalBounds();
    gridArea.removeFromLeft(PIANO_KEY_WIDTH);

    auto [beat, pitch] = screenToNote(e.getPosition(), gridArea);

    // Quantize to 16th notes
    double quantizedBeat = std::floor(beat * 4.0) / 4.0;

    // Check if clicking on existing note
    MIDINote *clickedNote = pattern_->findNoteAt(quantizedBeat, static_cast<uint8_t>(pitch), 0.1);

    if (e.mods.isRightButtonDown())
    {
        // Right click: delete note
        if (clickedNote)
        {
            pattern_->removeNotesAt(clickedNote->startBeat, clickedNote->pitch);
            repaint();
        }
    }
    else
    {
        if (clickedNote)
        {
            // Click on existing note: select and prepare for drag
            selectedNote_ = clickedNote;
            draggingNote_ = clickedNote;
            dragStartBeat_ = clickedNote->startBeat;
            dragStartPitch_ = clickedNote->pitch;

            // Check if near end for resize
            auto noteRect = noteToScreen(*clickedNote, gridArea);
            if (e.x > noteRect.getRight() - 8)
                dragMode_ = DragMode::ResizeEnd;
            else
                dragMode_ = DragMode::Move;
        }
        else
        {
            // Click on empty: add new note
            pattern_->addNote(quantizedBeat, 0.25, static_cast<uint8_t>(pitch), 100);
            selectedNote_ = pattern_->findNoteAt(quantizedBeat, static_cast<uint8_t>(pitch));
            draggingNote_ = selectedNote_;
            dragMode_ = DragMode::ResizeEnd; // Allow immediate resize
        }
        repaint();
    }
}

void PianoRollWidget::mouseDrag(const juce::MouseEvent &e)
{
    if (!pattern_ || !draggingNote_ || dragMode_ == DragMode::None)
        return;

    auto gridArea = getLocalBounds();
    gridArea.removeFromLeft(PIANO_KEY_WIDTH);

    auto [beat, pitch] = screenToNote(e.getPosition(), gridArea);
    double quantizedBeat = std::floor(beat * 4.0) / 4.0;

    if (dragMode_ == DragMode::Move)
    {
        double beatDelta = quantizedBeat - dragStartBeat_;
        int pitchDelta = pitch - dragStartPitch_;

        draggingNote_->startBeat = std::max(0.0, dragStartBeat_ + beatDelta);
        draggingNote_->pitch =
            static_cast<uint8_t>(std::clamp(dragStartPitch_ + pitchDelta, lowestNote_, highestNote_ - 1));
    }
    else if (dragMode_ == DragMode::ResizeEnd)
    {
        double newEnd = std::max(draggingNote_->startBeat + 0.125, quantizedBeat);
        draggingNote_->duration = newEnd - draggingNote_->startBeat;
    }

    repaint();
}

void PianoRollWidget::mouseUp(const juce::MouseEvent &)
{
    if (draggingNote_ && pattern_)
        pattern_->sortNotes();

    draggingNote_ = nullptr;
    dragMode_ = DragMode::None;
}

void PianoRollWidget::mouseWheelMove(const juce::MouseEvent &e,
                                      const juce::MouseWheelDetails &wheel)
{
    if (e.mods.isCommandDown())
    {
        // Zoom
        double zoomFactor = wheel.deltaY > 0 ? 1.1 : 0.9;
        pixelsPerBeat_ = std::clamp(pixelsPerBeat_ * zoomFactor, 10.0, 200.0);
    }
    else
    {
        // Scroll
        viewStartBeat_ -= wheel.deltaX * 2.0;
        viewStartBeat_ = std::max(0.0, viewStartBeat_);
    }
    repaint();
}

} // namespace SurgeBox
