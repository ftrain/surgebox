/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "PianoRollWidget.h"
#include "core/SurgeBoxEngine.h"
#include "gui/Theme.h"
#include <algorithm>

namespace SurgeBox
{

PianoRollWidget::PianoRollWidget()
{
    setOpaque(true);
    setWantsKeyboardFocus(true);
    rebuildVisiblePitches();
}

PianoRollWidget::~PianoRollWidget()
{
    if (patternModel_)
        patternModel_->onPatternChanged = nullptr;
}

void PianoRollWidget::setEngine(SurgeBoxEngine* engine)
{
    engine_ = engine;

    if (engine_)
    {
        onNoteOn = [this](int pitch, int velocity) {
            engine_->sendNoteToActiveVoice(pitch, velocity, true);
        };
        onNoteOff = [this](int pitch) {
            engine_->sendNoteToActiveVoice(pitch, 0, false);
        };
    }
}

void PianoRollWidget::setPatternModel(PatternModel* model)
{
    patternModel_ = model;

    if (patternModel_)
    {
        patternModel_->onPatternChanged = [this]() {
            selection_.validateSelection(patternModel_);
            repaint();
        };
    }

    selection_.clear();
    draggingNoteIndex_ = -1;
    repaint();
}

void PianoRollWidget::rebuildVisiblePitches()
{
    visiblePitches_ = MusicTheory::buildScalePitches(lowestNote_, highestNote_, scaleRoot_, scaleType_);
}

void PianoRollWidget::setScale(int root, ScaleType type)
{
    int newRoot = root % 12;

    if (patternModel_)
    {
        bool anyChanges = false;

        for (int i = 0; i < patternModel_->getNumNotes(); ++i)
        {
            double beat, dur;
            int currentPitch, vel;
            patternModel_->getNoteAt(i, beat, dur, currentPitch, vel);

            int originalPitch = patternModel_->getOriginalPitch(i);

            int targetPitch;
            if (type == ScaleType::Chromatic)
            {
                targetPitch = originalPitch;
            }
            else
            {
                targetPitch = MusicTheory::findNearestScalePitch(originalPitch, newRoot, type);
            }

            targetPitch = std::clamp(targetPitch, lowestNote_, highestNote_ - 1);

            if (targetPitch != currentPitch)
            {
                if (!anyChanges)
                {
                    patternModel_->beginTransaction("Quantize to Scale");
                    anyChanges = true;
                }
                patternModel_->setNotePitchForScale(i, targetPitch);
            }
        }
    }

    scaleRoot_ = newRoot;
    scaleType_ = type;
    rebuildVisiblePitches();
    selection_.clear();
    repaint();
}

bool PianoRollWidget::isNoteInScale(int pitch) const
{
    return MusicTheory::isPitchInScale(pitch, scaleRoot_, scaleType_);
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

    editor_.addNoteAtStep(patternModel_, stepPosition_, gridSize_, pitch, velocity);

    stepPosition_ += stepSize_;

    double patternLength = patternModel_->lengthInBeats();
    if (stepPosition_ >= patternLength)
        stepPosition_ = 0.0;

    repaint();
}

void PianoRollWidget::selectAll()
{
    selection_.selectAll(patternModel_);
    repaint();
}

void PianoRollWidget::clearSelection()
{
    selection_.clear();
    repaint();
}

void PianoRollWidget::deleteSelected()
{
    selection_.deleteSelected(patternModel_);
    repaint();
}

PianoRoll::RenderParams PianoRollWidget::buildRenderParams() const
{
    PianoRoll::RenderParams params;
    params.noteWidth = noteWidth_;
    params.pixelsPerBeat = pixelsPerBeat_;
    params.gridSize = gridSize_;
    params.lowestNote = lowestNote_;
    params.highestNote = highestNote_;
    params.visiblePitches = &visiblePitches_;
    params.selectedNotes = &selection_.getSelection();
    params.isChromatic = (scaleType_ == ScaleType::Chromatic);
    params.scaleRoot = scaleRoot_;
    return params;
}

int PianoRollWidget::pitchToColumn(int pitch) const
{
    return PianoRoll::pitchToColumn(pitch, buildRenderParams());
}

int PianoRollWidget::columnToPitch(int column) const
{
    return PianoRoll::columnToPitch(column, buildRenderParams());
}

int PianoRollWidget::getPitchAtX(int x) const
{
    int column = x / noteWidth_;
    int pitch = columnToPitch(column);
    if (pitch < 0)
        return std::clamp(lowestNote_ + column, lowestNote_, highestNote_ - 1);
    return pitch;
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

std::pair<double, int> PianoRollWidget::screenToNote(juce::Point<int> pos)
{
    return PianoRoll::screenToNote(pos, getGridArea(), buildRenderParams());
}

juce::Rectangle<int> PianoRollWidget::noteToScreen(int noteIndex)
{
    return PianoRoll::noteToScreen(noteIndex, getGridArea(), patternModel_, buildRenderParams());
}

void PianoRollWidget::paint(juce::Graphics& g)
{
    g.fillAll(Theme::color(Theme::pianoRollBackground));

    if (engine_ && engine_->isPlaying())
        sequencerPlayingNotes_ = engine_->getActivePlayingNotes();
    else
        sequencerPlayingNotes_.clear();

    auto gridArea = getGridArea();
    auto params = buildRenderParams();

    PianoRoll::drawGrid(g, gridArea, patternModel_, params);
    PianoRoll::drawNotes(g, gridArea, patternModel_, params);

    if (engine_ && engine_->isPlaying())
    {
        double playhead = engine_->getActiveVoicePlayheadBeats();
        PianoRoll::drawPlayhead(g, gridArea, playhead, pixelsPerBeat_);
    }

    if (stepRecordEnabled_)
        PianoRoll::drawStepCursor(g, gridArea, stepPosition_, pixelsPerBeat_);

    if (dragMode_ == DragMode::BoxSelect)
        PianoRoll::drawBoxSelection(g, boxSelectStart_, boxSelectEnd_);
}

void PianoRollWidget::resized()
{
}

void PianoRollWidget::mouseDown(const juce::MouseEvent& e)
{
    if (!patternModel_)
        return;

    grabKeyboardFocus();

    auto gridArea = getGridArea();

    auto [beat, pitch] = screenToNote(e.getPosition());
    double quantizedBeat = std::floor(beat / gridSize_) * gridSize_;

    int clickedIndex = patternModel_->findNoteContaining(quantizedBeat, pitch, 0.05);

    if (e.mods.isRightButtonDown() || e.mods.isCommandDown())
    {
        patternModel_->beginTransaction("Erase Notes");

        if (clickedIndex >= 0)
        {
            patternModel_->removeNote(clickedIndex);
            selection_.remove(clickedIndex);
        }

        dragMode_ = DragMode::Erasing;
        lastDrawnBeat_ = quantizedBeat;
        lastDrawnPitch_ = pitch;
        repaint();
        return;
    }

    if (clickedIndex >= 0)
    {
        if (!selection_.contains(clickedIndex))
        {
            selection_.clear();
            selection_.add(clickedIndex);
        }

        draggingNoteIndex_ = clickedIndex;

        double startBeat, duration;
        int notePitch, velocity;
        patternModel_->getNoteAt(clickedIndex, startBeat, duration, notePitch, velocity);

        dragStartBeat_ = quantizedBeat;
        dragStartPitch_ = pitch;
        dragStartDuration_ = duration;
        originalNoteBeat_ = startBeat;
        originalNotePitch_ = notePitch;

        auto noteRect = noteToScreen(clickedIndex);
        int clickYRelative = e.y - noteRect.getY();
        int resizeZone = std::max(6, noteRect.getHeight() / 5);

        if (clickYRelative >= noteRect.getHeight() - resizeZone)
        {
            dragMode_ = DragMode::ResizeEnd;
            patternModel_->beginTransaction("Resize Note");
        }
        else
        {
            dragMode_ = DragMode::Move;
            patternModel_->beginTransaction("Move Notes");
            editor_.beginMove(patternModel_, selection_.getSelection());
        }
    }
    else
    {
        if (e.mods.isShiftDown())
        {
            dragMode_ = DragMode::BoxSelect;
            boxSelectStart_ = e.getPosition();
            boxSelectEnd_ = e.getPosition();
        }
        else
        {
            selection_.clear();

            patternModel_->beginTransaction("Draw Notes");

            editor_.removeOverlappingNotes(patternModel_, pitch, quantizedBeat, quantizedBeat + gridSize_);

            int newIndex = editor_.addNote(patternModel_, quantizedBeat, gridSize_, pitch, 100);
            if (newIndex >= 0)
            {
                selection_.add(newIndex);
            }

            dragMode_ = DragMode::Drawing;
            lastDrawnBeat_ = quantizedBeat;
            lastDrawnPitch_ = pitch;
            draggingNoteIndex_ = -1;
        }
    }
    repaint();
}

void PianoRollWidget::mouseDrag(const juce::MouseEvent& e)
{
    if (!patternModel_)
        return;

    if (dragMode_ == DragMode::PlayingPiano)
        return;

    auto gridArea = getGridArea();

    if (dragMode_ == DragMode::BoxSelect)
    {
        boxSelectEnd_ = e.getPosition();
        repaint();
        return;
    }

    auto [beat, pitch] = screenToNote(e.getPosition());
    double quantizedBeat = std::floor(beat / gridSize_) * gridSize_;

    if (dragMode_ == DragMode::Drawing)
    {
        if (quantizedBeat != lastDrawnBeat_ || pitch != lastDrawnPitch_)
        {
            if (quantizedBeat >= 0 && quantizedBeat < patternModel_->lengthInBeats())
            {
                editor_.removeOverlappingNotes(patternModel_, pitch, quantizedBeat, quantizedBeat + gridSize_);
                editor_.addNote(patternModel_, quantizedBeat, gridSize_, pitch, 100);

                lastDrawnBeat_ = quantizedBeat;
                lastDrawnPitch_ = pitch;
                repaint();
            }
        }
        return;
    }

    if (dragMode_ == DragMode::Erasing)
    {
        if (quantizedBeat != lastDrawnBeat_ || pitch != lastDrawnPitch_)
        {
            int noteIndex = patternModel_->findNoteContaining(quantizedBeat, pitch, 0.05);
            if (noteIndex >= 0)
            {
                patternModel_->removeNote(noteIndex);
                selection_.remove(noteIndex);
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

        std::set<int> selectionCopy = selection_.getSelection();
        editor_.moveNotes(patternModel_, selectionCopy, beatDelta, pitchDelta,
                          lowestNote_, highestNote_);

        // Update selection from mutable copy
        selection_.clear();
        for (int idx : selectionCopy)
            selection_.add(idx);

        dragStartBeat_ = quantizedBeat;
        dragStartPitch_ = pitch;
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
            editor_.resizeNote(patternModel_, draggingNoteIndex_, newDuration);
        }
    }

    repaint();
}

void PianoRollWidget::mouseUp(const juce::MouseEvent& e)
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
            selection_.clear();

        selection_.selectInRect(selRect, gridArea, patternModel_, buildRenderParams());
    }
    else if (dragMode_ == DragMode::Move)
    {
        std::set<int> selectionCopy = selection_.getSelection();
        editor_.endMove(patternModel_, selectionCopy);

        selection_.clear();
        for (int idx : selectionCopy)
            selection_.add(idx);
    }

    draggingNoteIndex_ = -1;
    dragMode_ = DragMode::None;
    repaint();
}

void PianoRollWidget::mouseMove(const juce::MouseEvent& e)
{
    if (!patternModel_)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    auto gridArea = getGridArea();

    auto [beat, pitch] = screenToNote(e.getPosition());
    double quantizedBeat = std::floor(beat / gridSize_) * gridSize_;
    int hoveredIndex = patternModel_->findNoteContaining(quantizedBeat, pitch, 0.05);

    if (hoveredIndex >= 0)
    {
        auto noteRect = noteToScreen(hoveredIndex);
        int mouseYRelative = e.y - noteRect.getY();
        int resizeZone = std::max(6, noteRect.getHeight() / 5);

        if (mouseYRelative >= noteRect.getHeight() - resizeZone)
        {
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        }
        else
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void PianoRollWidget::mouseWheelMove(const juce::MouseEvent& e,
                                      const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
    {
        double zoomFactor = wheel.deltaY > 0 ? 1.15 : 0.87;
        double oldPixelsPerBeat = pixelsPerBeat_;
        pixelsPerBeat_ = std::clamp(pixelsPerBeat_ * zoomFactor, 15.0, 120.0);

        if (pixelsPerBeat_ != oldPixelsPerBeat)
        {
            if (auto* parent = getParentComponent())
                parent->resized();
        }
        return;
    }

    juce::Component::mouseWheelMove(e, wheel);
}

bool PianoRollWidget::keyPressed(const juce::KeyPress& key)
{
    if (key.isKeyCode('A') && key.getModifiers().isCommandDown())
    {
        selectAll();
        return true;
    }

    if (key.isKeyCode('C') && key.getModifiers().isCommandDown())
    {
        selection_.copy(patternModel_);
        return true;
    }

    if (key.isKeyCode('X') && key.getModifiers().isCommandDown())
    {
        selection_.cut(patternModel_);
        repaint();
        return true;
    }

    if (key.isKeyCode('V') && key.getModifiers().isCommandDown())
    {
        double pastePosition = stepRecordEnabled_ ? stepPosition_ : 0.0;
        selection_.paste(patternModel_, pastePosition);
        repaint();
        return true;
    }

    if (key.isKeyCode(juce::KeyPress::deleteKey) ||
        key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        deleteSelected();
        return true;
    }

    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        clearSelection();
        return true;
    }

    return false;
}

} // namespace SurgeBox
