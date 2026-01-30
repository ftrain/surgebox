/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "PianoRollWidget.h"
#include "core/SurgeBoxEngine.h"
#include <algorithm>

namespace SurgeBox
{

// Static clipboard for copy/paste
static std::vector<std::tuple<double, double, int, int>> g_clipboard;

PianoRollWidget::PianoRollWidget()
{
    setOpaque(true);
    setWantsKeyboardFocus(true);
}

PianoRollWidget::~PianoRollWidget()
{
    if (patternModel_)
        patternModel_->onPatternChanged = nullptr;
}

void PianoRollWidget::setEngine(SurgeBoxEngine *engine)
{
    engine_ = engine;

    // Set up note playback callbacks
    if (engine_)
    {
        onNoteOn = [this](int pitch, int velocity) {
            auto *synth = engine_->getActiveSynth();
            if (synth)
                synth->playNote(0, pitch, velocity, 0, -1);
        };
        onNoteOff = [this](int pitch) {
            auto *synth = engine_->getActiveSynth();
            if (synth)
                synth->releaseNote(0, pitch, 0);
        };
    }
}

void PianoRollWidget::setPatternModel(PatternModel *model)
{
    patternModel_ = model;

    if (patternModel_)
    {
        patternModel_->onPatternChanged = [this]() {
            std::set<int> validSelection;
            for (int idx : selectedNotes_)
            {
                if (idx < patternModel_->getNumNotes())
                    validSelection.insert(idx);
            }
            selectedNotes_ = validSelection;
            repaint();
        };
    }

    selectedNotes_.clear();
    draggingNoteIndex_ = -1;
    repaint();
}

void PianoRollWidget::setStepRecordEnabled(bool enabled)
{
    stepRecordEnabled_ = enabled;
    repaint();
}

void PianoRollWidget::resetStepPosition()
{
    stepPosition_ = 0.0;
    repaint();
}

void PianoRollWidget::addNoteAtCurrentStep(int pitch, int velocity)
{
    if (!patternModel_)
        return;

    patternModel_->beginTransaction("Step Record Note");

    // Remove any overlapping notes first
    removeOverlappingNotes(pitch, stepPosition_, stepPosition_ + gridSize_);

    patternModel_->addNote(stepPosition_, gridSize_, pitch, velocity);

    stepPosition_ += stepSize_;

    double patternLength = patternModel_->lengthInBeats();
    if (stepPosition_ >= patternLength)
        stepPosition_ = 0.0;

    repaint();
}

void PianoRollWidget::selectAll()
{
    if (!patternModel_)
        return;

    selectedNotes_.clear();
    for (int i = 0; i < patternModel_->getNumNotes(); ++i)
        selectedNotes_.insert(i);
    repaint();
}

void PianoRollWidget::clearSelection()
{
    selectedNotes_.clear();
    repaint();
}

void PianoRollWidget::deleteSelected()
{
    if (!patternModel_ || selectedNotes_.empty())
        return;

    patternModel_->beginTransaction("Delete Notes");

    std::vector<int> toDelete(selectedNotes_.begin(), selectedNotes_.end());
    std::sort(toDelete.rbegin(), toDelete.rend());

    for (int idx : toDelete)
        patternModel_->removeNote(idx);

    selectedNotes_.clear();
    repaint();
}

void PianoRollWidget::removeOverlappingNotes(int pitch, double startBeat, double endBeat, int excludeIndex)
{
    if (!patternModel_)
        return;

    // Find and remove notes that overlap with the given range
    for (int i = patternModel_->getNumNotes() - 1; i >= 0; --i)
    {
        if (i == excludeIndex)
            continue;

        double noteStart, noteDuration;
        int notePitch, noteVelocity;
        patternModel_->getNoteAt(i, noteStart, noteDuration, notePitch, noteVelocity);

        if (notePitch != pitch)
            continue;

        double noteEnd = noteStart + noteDuration;

        // Check for overlap
        if (noteStart < endBeat && noteEnd > startBeat)
        {
            patternModel_->removeNote(i);
            // Adjust excludeIndex if we removed a note before it
            if (excludeIndex > i)
                excludeIndex--;
        }
    }
}

juce::Rectangle<int> PianoRollWidget::getPianoArea() const
{
    auto bounds = getLocalBounds();
    return bounds.removeFromTop(PIANO_KEY_HEIGHT);
}

juce::Rectangle<int> PianoRollWidget::getGridArea() const
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(PIANO_KEY_HEIGHT);
    return bounds;
}

int PianoRollWidget::getPitchAtX(int x) const
{
    int noteIndex = x / noteWidth_;
    return std::clamp(lowestNote_ + noteIndex, lowestNote_, highestNote_ - 1);
}

void PianoRollWidget::playNote(int pitch)
{
    if (onNoteOn)
        onNoteOn(pitch, 100);
    playingNote_ = pitch;
    repaint();
}

void PianoRollWidget::stopNote(int pitch)
{
    if (onNoteOff)
        onNoteOff(pitch);
    if (playingNote_ == pitch)
        playingNote_ = -1;
    repaint();
}

void PianoRollWidget::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds();

    g.fillAll(bgColor_);

    // Update sequencer playing notes for highlighting
    if (engine_ && engine_->isPlaying())
        sequencerPlayingNotes_ = engine_->getActivePlayingNotes();
    else
        sequencerPlayingNotes_.clear();

    auto pianoArea = bounds.removeFromTop(PIANO_KEY_HEIGHT);
    auto gridArea = bounds;

    drawPianoKeys(g, pianoArea);
    drawGrid(g, gridArea);
    drawNotes(g, gridArea);
    drawPlayhead(g, gridArea);

    if (stepRecordEnabled_)
        drawStepCursor(g, gridArea);

    if (dragMode_ == DragMode::BoxSelect)
        drawBoxSelection(g, gridArea);
}

void PianoRollWidget::resized()
{
}

void PianoRollWidget::drawPianoKeys(juce::Graphics &g, const juce::Rectangle<int> &area)
{
    int numNotes = highestNote_ - lowestNote_;

    for (int i = 0; i < numNotes; i++)
    {
        int note = lowestNote_ + i;
        int x = area.getX() + (i * noteWidth_);

        int noteInOctave = note % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                          noteInOctave == 8 || noteInOctave == 10);

        // Check if note is playing from sequencer
        bool isSequencerPlaying = std::find(sequencerPlayingNotes_.begin(),
                                            sequencerPlayingNotes_.end(),
                                            static_cast<uint8_t>(note)) != sequencerPlayingNotes_.end();

        // Highlight playing note (from mouse click or sequencer)
        if (note == playingNote_ || isSequencerPlaying)
            g.setColour(playingKeyColor_);
        else
            g.setColour(isBlackKey ? blackKeyColor_ : whiteKeyColor_);

        g.fillRect(x, area.getY(), noteWidth_ - 1, area.getHeight());

        // Draw note name for C notes
        if (noteInOctave == 0)
        {
            int octave = (note / 12) - 1;
            g.setColour(juce::Colours::black);
            g.setFont(9.0f);
            g.drawText(juce::String("C") + juce::String(octave), x, area.getY(),
                       noteWidth_, area.getHeight(), juce::Justification::centred);
        }
    }

    // Bottom border of piano
    g.setColour(barLineColor_);
    g.drawHorizontalLine(area.getBottom() - 1, static_cast<float>(area.getX()),
                        static_cast<float>(area.getRight()));
}

void PianoRollWidget::drawGrid(juce::Graphics &g, const juce::Rectangle<int> &area)
{
    if (!patternModel_)
        return;

    int numNotes = highestNote_ - lowestNote_;
    double totalBeats = patternModel_->lengthInBeats();

    // Only draw grid for actual pattern length
    int patternHeight = static_cast<int>(totalBeats * pixelsPerBeat_);

    // First pass: Draw background shading for black keys and alternating beats
    // These layer on top of each other

    // Draw alternating beat shading (horizontal bands) - only within pattern
    for (int beat = 0; beat < static_cast<int>(totalBeats); beat++)
    {
        int y1 = area.getY() + static_cast<int>(beat * pixelsPerBeat_);
        int y2 = area.getY() + static_cast<int>((beat + 1) * pixelsPerBeat_);

        // Alternate every beat
        if (beat % 2 == 1)
        {
            g.setColour(juce::Colour(0x08ffffff));  // Very subtle white overlay
            g.fillRect(area.getX(), y1, area.getWidth(), y2 - y1);
        }
    }

    // Draw black key column shading (vertical bands) - only within pattern height
    for (int i = 0; i < numNotes; i++)
    {
        int note = lowestNote_ + i;
        int noteInOctave = note % 12;
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                          noteInOctave == 8 || noteInOctave == 10);

        if (isBlackKey)
        {
            int x = area.getX() + (i * noteWidth_);
            g.setColour(juce::Colour(0x15000000));  // Subtle dark overlay
            g.fillRect(x, area.getY(), noteWidth_, patternHeight);
        }
    }

    // Second pass: Draw grid lines

    // Vertical lines (pitch columns) - only to pattern height
    for (int i = 0; i <= numNotes; i++)
    {
        int x = area.getX() + (i * noteWidth_);
        int note = lowestNote_ + i;
        int noteInOctave = note % 12;

        bool isC = (noteInOctave == 0);
        g.setColour(isC ? barLineColor_ : gridColor_);
        g.drawVerticalLine(x, static_cast<float>(area.getY()),
                          static_cast<float>(area.getY() + patternHeight));
    }

    // Horizontal lines (time/beats) - only within pattern
    for (double beat = 0; beat <= totalBeats; beat += gridSize_)
    {
        int y = area.getY() + static_cast<int>(beat * pixelsPerBeat_);

        bool isBar = (static_cast<int>(beat) % 4 == 0 && beat == static_cast<int>(beat));
        bool isBeat = (beat == static_cast<int>(beat));

        if (isBar)
        {
            // Bar lines are thickest
            g.setColour(barLineColor_);
            g.fillRect(area.getX(), y - 1, area.getWidth(), 3);
        }
        else if (isBeat)
        {
            // Beat lines are thicker than sub-beats
            g.setColour(beatLineColor_);
            g.fillRect(area.getX(), y, area.getWidth(), 2);
        }
        else
        {
            g.setColour(gridColor_);
            g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                                static_cast<float>(area.getRight()));
        }
    }

    // Draw a highlight line at the pattern boundary
    int patternEndY = area.getY() + patternHeight;
    g.setColour(noteColor_.withAlpha(0.5f));
    g.fillRect(area.getX(), patternEndY - 1, area.getWidth(), 2);
}

void PianoRollWidget::drawNotes(juce::Graphics &g, const juce::Rectangle<int> &area)
{
    if (!patternModel_)
        return;

    for (int i = 0; i < patternModel_->getNumNotes(); ++i)
    {
        auto noteRect = noteToScreen(i, area);
        if (noteRect.isEmpty() || !noteRect.intersects(area))
            continue;

        bool selected = isNoteSelected(i);
        g.setColour(selected ? noteSelectedColor_ : noteColor_);
        g.fillRoundedRectangle(noteRect.toFloat(), 3.0f);

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(noteRect.toFloat(), 3.0f, 1.0f);

        double startBeat, duration;
        int pitch, velocity;
        patternModel_->getNoteAt(i, startBeat, duration, pitch, velocity);

        float velAlpha = 1.0f - (static_cast<float>(velocity) / 127.0f) * 0.5f;
        g.setColour(juce::Colours::black.withAlpha(velAlpha * 0.3f));
        g.fillRoundedRectangle(noteRect.toFloat(), 3.0f);
    }
}

void PianoRollWidget::drawPlayhead(juce::Graphics &g, const juce::Rectangle<int> &area)
{
    if (!engine_ || !engine_->isPlaying())
        return;

    double playhead = engine_->getPlayheadBeats();
    int y = area.getY() + static_cast<int>(playhead * pixelsPerBeat_);

    if (y >= area.getY() && y <= area.getBottom())
    {
        g.setColour(playheadColor_);
        g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                            static_cast<float>(area.getRight()));

        juce::Path triangle;
        triangle.addTriangle(static_cast<float>(area.getX()), static_cast<float>(y - 5),
                            static_cast<float>(area.getX()), static_cast<float>(y + 5),
                            static_cast<float>(area.getX() + 8), static_cast<float>(y));
        g.fillPath(triangle);
    }
}

void PianoRollWidget::drawStepCursor(juce::Graphics &g, const juce::Rectangle<int> &area)
{
    int y = area.getY() + static_cast<int>(stepPosition_ * pixelsPerBeat_);

    if (y >= area.getY() && y <= area.getBottom())
    {
        g.setColour(stepCursorColor_);
        g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                            static_cast<float>(area.getRight()));

        g.fillRect(area.getRight() - 8, y - 4, 8, 8);

        g.setFont(10.0f);
        g.drawText("STEP", area.getRight() - 40, y - 15, 30, 12, juce::Justification::centred);
    }
}

void PianoRollWidget::drawBoxSelection(juce::Graphics &g, const juce::Rectangle<int> & /*area*/)
{
    auto selRect = juce::Rectangle<int>(boxSelectStart_, boxSelectEnd_);

    g.setColour(boxSelectColor_);
    g.fillRect(selRect);

    g.setColour(noteColor_);
    g.drawRect(selRect, 1);
}

std::pair<double, int> PianoRollWidget::screenToNote(juce::Point<int> pos,
                                                      const juce::Rectangle<int> &area)
{
    double beat = (pos.y - area.getY()) / pixelsPerBeat_;
    int noteIndex = (pos.x - area.getX()) / noteWidth_;
    int pitch = lowestNote_ + noteIndex;

    return {beat, std::clamp(pitch, lowestNote_, highestNote_ - 1)};
}

juce::Rectangle<int> PianoRollWidget::noteToScreen(int noteIndex,
                                                    const juce::Rectangle<int> &area)
{
    if (!patternModel_ || noteIndex < 0 || noteIndex >= patternModel_->getNumNotes())
        return {};

    double startBeat, duration;
    int pitch, velocity;
    patternModel_->getNoteAt(noteIndex, startBeat, duration, pitch, velocity);

    int noteCol = pitch - lowestNote_;
    int x = area.getX() + (noteCol * noteWidth_);
    int y = area.getY() + static_cast<int>(startBeat * pixelsPerBeat_);
    int h = static_cast<int>(duration * pixelsPerBeat_);

    return juce::Rectangle<int>(x + 1, y, noteWidth_ - 2, std::max(6, h));
}

void PianoRollWidget::selectNotesInRect(const juce::Rectangle<int> &rect,
                                         const juce::Rectangle<int> &gridArea)
{
    if (!patternModel_)
        return;

    for (int i = 0; i < patternModel_->getNumNotes(); ++i)
    {
        auto noteRect = noteToScreen(i, gridArea);
        if (rect.intersects(noteRect))
            selectedNotes_.insert(i);
    }
}

void PianoRollWidget::mouseDown(const juce::MouseEvent &e)
{
    if (!patternModel_)
        return;

    grabKeyboardFocus();

    auto pianoArea = getPianoArea();
    auto gridArea = getGridArea();

    // Check if clicking on piano keys
    if (pianoArea.contains(e.getPosition()))
    {
        int pitch = getPitchAtX(e.x);
        playNote(pitch);

        // In step record mode, also add a note
        if (stepRecordEnabled_)
        {
            addNoteAtCurrentStep(pitch, 100);
        }

        dragMode_ = DragMode::PlayingPiano;
        return;
    }

    auto [beat, pitch] = screenToNote(e.getPosition(), gridArea);
    double quantizedBeat = std::floor(beat / gridSize_) * gridSize_;

    int clickedIndex = patternModel_->findNoteContaining(quantizedBeat, pitch, 0.05);

    // Right-click to delete/erase mode
    if (e.mods.isRightButtonDown())
    {
        patternModel_->beginTransaction("Erase Notes");

        if (clickedIndex >= 0)
        {
            patternModel_->removeNote(clickedIndex);
            selectedNotes_.erase(clickedIndex);
        }

        // Enter erasing mode for drag-to-erase
        dragMode_ = DragMode::Erasing;
        lastDrawnBeat_ = quantizedBeat;
        lastDrawnPitch_ = pitch;
        repaint();
        return;
    }

    // Left-click behavior
    if (clickedIndex >= 0)
    {
        // Clicked on existing note - can move or resize
        if (!isNoteSelected(clickedIndex))
        {
            selectedNotes_.clear();
            selectedNotes_.insert(clickedIndex);
        }

        draggingNoteIndex_ = clickedIndex;

        double startBeat, duration;
        int notePitch, velocity;
        patternModel_->getNoteAt(clickedIndex, startBeat, duration, notePitch, velocity);

        dragStartBeat_ = startBeat;
        dragStartPitch_ = notePitch;
        dragStartDuration_ = duration;
        originalNoteBeat_ = startBeat;
        originalNotePitch_ = notePitch;

        // Clicking existing note allows resize by dragging up/down
        dragMode_ = DragMode::ResizeEnd;
        patternModel_->beginTransaction("Resize Note");
    }
    else
    {
        // Clicked on empty space - add note and enter drawing mode
        selectedNotes_.clear();

        patternModel_->beginTransaction("Draw Notes");

        // Remove any overlapping notes first
        removeOverlappingNotes(pitch, quantizedBeat, quantizedBeat + gridSize_);

        patternModel_->addNote(quantizedBeat, gridSize_, pitch, 100);

        int newIndex = patternModel_->findNoteAt(quantizedBeat, pitch);
        if (newIndex >= 0)
        {
            selectedNotes_.insert(newIndex);
        }

        // Enter drawing mode for drag-to-draw
        dragMode_ = DragMode::Drawing;
        lastDrawnBeat_ = quantizedBeat;
        lastDrawnPitch_ = pitch;
        draggingNoteIndex_ = -1;
    }
    repaint();
}

void PianoRollWidget::mouseDrag(const juce::MouseEvent &e)
{
    if (!patternModel_)
        return;

    // Handle piano playing
    if (dragMode_ == DragMode::PlayingPiano)
    {
        auto pianoArea = getPianoArea();
        if (pianoArea.contains(e.getPosition()))
        {
            int pitch = getPitchAtX(e.x);
            if (pitch != playingNote_)
            {
                if (playingNote_ >= 0)
                    stopNote(playingNote_);
                playNote(pitch);
            }
        }
        return;
    }

    auto gridArea = getGridArea();

    if (dragMode_ == DragMode::BoxSelect)
    {
        boxSelectEnd_ = e.getPosition();
        repaint();
        return;
    }

    auto [beat, pitch] = screenToNote(e.getPosition(), gridArea);
    double quantizedBeat = std::floor(beat / gridSize_) * gridSize_;

    // Handle drawing mode - add notes while dragging
    if (dragMode_ == DragMode::Drawing)
    {
        // Only add note if position changed
        if (quantizedBeat != lastDrawnBeat_ || pitch != lastDrawnPitch_)
        {
            // Check pattern bounds
            if (quantizedBeat >= 0 && quantizedBeat < patternModel_->lengthInBeats())
            {
                // Remove any overlapping notes first
                removeOverlappingNotes(pitch, quantizedBeat, quantizedBeat + gridSize_);

                patternModel_->addNote(quantizedBeat, gridSize_, pitch, 100);

                lastDrawnBeat_ = quantizedBeat;
                lastDrawnPitch_ = pitch;
                repaint();
            }
        }
        return;
    }

    // Handle erasing mode - delete notes while dragging
    if (dragMode_ == DragMode::Erasing)
    {
        // Only check if position changed
        if (quantizedBeat != lastDrawnBeat_ || pitch != lastDrawnPitch_)
        {
            int noteIndex = patternModel_->findNoteContaining(quantizedBeat, pitch, 0.05);
            if (noteIndex >= 0)
            {
                patternModel_->removeNote(noteIndex);
                selectedNotes_.erase(noteIndex);
                repaint();
            }

            lastDrawnBeat_ = quantizedBeat;
            lastDrawnPitch_ = pitch;
        }
        return;
    }

    if (draggingNoteIndex_ < 0 || dragMode_ == DragMode::None)
        return;

    if (dragMode_ == DragMode::Move)
    {
        double beatDelta = quantizedBeat - dragStartBeat_;
        int pitchDelta = pitch - dragStartPitch_;

        double newBeat = std::max(0.0, originalNoteBeat_ + beatDelta);
        int newPitch = std::clamp(originalNotePitch_ + pitchDelta, lowestNote_, highestNote_ - 1);

        // Only update if position actually changed
        double currentBeat, currentDuration;
        int currentPitch, currentVel;
        patternModel_->getNoteAt(draggingNoteIndex_, currentBeat, currentDuration, currentPitch, currentVel);

        if (newBeat != currentBeat || newPitch != currentPitch)
        {
            patternModel_->moveNote(draggingNoteIndex_, newBeat, newPitch);
        }
    }
    else if (dragMode_ == DragMode::ResizeEnd)
    {
        double startBeat, duration;
        int notePitch, velocity;
        patternModel_->getNoteAt(draggingNoteIndex_, startBeat, duration, notePitch, velocity);

        double newEnd = std::max(startBeat + gridSize_, quantizedBeat + gridSize_);
        double newDuration = newEnd - startBeat;

        if (newDuration != duration)
        {
            // Remove notes that would be overlapped by the resize
            removeOverlappingNotes(notePitch, startBeat, newEnd, draggingNoteIndex_);
            patternModel_->resizeNote(draggingNoteIndex_, newDuration);
        }
    }

    repaint();
}

void PianoRollWidget::mouseUp(const juce::MouseEvent &e)
{
    if (dragMode_ == DragMode::PlayingPiano)
    {
        if (playingNote_ >= 0)
            stopNote(playingNote_);
    }
    else if (dragMode_ == DragMode::BoxSelect)
    {
        auto gridArea = getGridArea();
        auto selRect = juce::Rectangle<int>(boxSelectStart_, boxSelectEnd_);

        if (!e.mods.isShiftDown())
            selectedNotes_.clear();

        selectNotesInRect(selRect, gridArea);
    }
    else if (dragMode_ == DragMode::Move && draggingNoteIndex_ >= 0)
    {
        // After moving, remove any notes that overlap with the moved note
        double startBeat, duration;
        int pitch, velocity;
        patternModel_->getNoteAt(draggingNoteIndex_, startBeat, duration, pitch, velocity);
        removeOverlappingNotes(pitch, startBeat, startBeat + duration, draggingNoteIndex_);
    }

    draggingNoteIndex_ = -1;
    dragMode_ = DragMode::None;
    repaint();
}

void PianoRollWidget::mouseWheelMove(const juce::MouseEvent &e,
                                      const juce::MouseWheelDetails &wheel)
{
    // Cmd+wheel for zooming
    if (e.mods.isCommandDown())
    {
        double zoomFactor = wheel.deltaY > 0 ? 1.15 : 0.87;
        double oldPixelsPerBeat = pixelsPerBeat_;
        pixelsPerBeat_ = std::clamp(pixelsPerBeat_ * zoomFactor, 15.0, 120.0);

        // Notify parent to resize us
        if (pixelsPerBeat_ != oldPixelsPerBeat)
        {
            if (auto *parent = getParentComponent())
                parent->resized();
        }
        return;
    }

    // Otherwise let viewport handle scrolling
    juce::Component::mouseWheelMove(e, wheel);
}

bool PianoRollWidget::keyPressed(const juce::KeyPress &key)
{
    // Cmd+A = Select All
    if (key.isKeyCode('A') && key.getModifiers().isCommandDown())
    {
        selectAll();
        return true;
    }

    // Cmd+C = Copy
    if (key.isKeyCode('C') && key.getModifiers().isCommandDown())
    {
        if (!patternModel_ || selectedNotes_.empty())
            return true;

        // Store copied notes in global clipboard
        g_clipboard.clear();

        // Find the earliest selected note to use as reference
        double minBeat = std::numeric_limits<double>::max();
        for (int idx : selectedNotes_)
        {
            double startBeat, duration;
            int pitch, velocity;
            patternModel_->getNoteAt(idx, startBeat, duration, pitch, velocity);
            minBeat = std::min(minBeat, startBeat);
        }

        // Copy notes relative to the earliest note
        for (int idx : selectedNotes_)
        {
            double startBeat, duration;
            int pitch, velocity;
            patternModel_->getNoteAt(idx, startBeat, duration, pitch, velocity);
            g_clipboard.emplace_back(startBeat - minBeat, duration, pitch, velocity);
        }

        return true;
    }

    // Cmd+X = Cut
    if (key.isKeyCode('X') && key.getModifiers().isCommandDown())
    {
        if (!patternModel_ || selectedNotes_.empty())
            return true;

        // Copy first
        g_clipboard.clear();

        double minBeat = std::numeric_limits<double>::max();
        for (int idx : selectedNotes_)
        {
            double startBeat, duration;
            int pitch, velocity;
            patternModel_->getNoteAt(idx, startBeat, duration, pitch, velocity);
            minBeat = std::min(minBeat, startBeat);
        }

        for (int idx : selectedNotes_)
        {
            double startBeat, duration;
            int pitch, velocity;
            patternModel_->getNoteAt(idx, startBeat, duration, pitch, velocity);
            g_clipboard.emplace_back(startBeat - minBeat, duration, pitch, velocity);
        }

        // Then delete
        deleteSelected();

        return true;
    }

    // Cmd+V = Paste
    if (key.isKeyCode('V') && key.getModifiers().isCommandDown())
    {
        if (!patternModel_)
            return true;

        if (g_clipboard.empty())
            return true;

        patternModel_->beginTransaction("Paste Notes");

        // Paste at step position or beginning
        double pastePosition = stepRecordEnabled_ ? stepPosition_ : 0.0;

        selectedNotes_.clear();

        for (const auto &[relBeat, duration, pitch, velocity] : g_clipboard)
        {
            double startBeat = pastePosition + relBeat;

            // Remove overlapping notes
            removeOverlappingNotes(pitch, startBeat, startBeat + duration);

            patternModel_->addNote(startBeat, duration, pitch, velocity);

            int newIndex = patternModel_->findNoteAt(startBeat, pitch);
            if (newIndex >= 0)
                selectedNotes_.insert(newIndex);
        }

        repaint();
        return true;
    }

    // Delete or Backspace
    if (key.isKeyCode(juce::KeyPress::deleteKey) ||
        key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        deleteSelected();
        return true;
    }

    // Escape
    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        clearSelection();
        return true;
    }

    return false;
}

} // namespace SurgeBox
