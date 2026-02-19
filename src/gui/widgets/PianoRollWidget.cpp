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
#include "pianoroll/GridLayer.h"
#include "pianoroll/NoteLayer.h"
#include "pianoroll/PlayheadLayer.h"
#include "pianoroll/LoopLayer.h"
#include "pianoroll/GhostNoteLayer.h"
#include "pianoroll/OverlayLayer.h"
#include <algorithm>

namespace SurgeBox
{

PianoRollWidget::PianoRollWidget()
{
    setOpaque(true);
    setWantsKeyboardFocus(true);
    rebuildVisiblePitches();

    // Create layers (order = z-order, bottom to top)
    gridLayer_ = std::make_unique<GridLayer>();
    addAndMakeVisible(*gridLayer_);

    noteLayer_ = std::make_unique<NoteLayer>();
    addAndMakeVisible(*noteLayer_);

    loopLayer_ = std::make_unique<LoopLayer>();
    addAndMakeVisible(*loopLayer_);

    ghostNoteLayer_ = std::make_unique<GhostNoteLayer>();
    addAndMakeVisible(*ghostNoteLayer_);

    overlayLayer_ = std::make_unique<OverlayLayer>();
    addAndMakeVisible(*overlayLayer_);

    playheadLayer_ = std::make_unique<PlayheadLayer>();
    addAndMakeVisible(*playheadLayer_);

    // Selection toolbar (on top of all layers)
    selectionToolbar_ = std::make_unique<PianoRoll::SelectionToolbar>();
    addChildComponent(*selectionToolbar_);

    selectionToolbar_->onLoop = [this]() { performLoop(); };
    selectionToolbar_->onDelete = [this]() {
        deleteSelected();
        hideSelectionToolbar();
    };
    selectionToolbar_->onInvert = [this]() { performInvert(); };
    selectionToolbar_->onHalve = [this]() { performHalveDuration(); };
    selectionToolbar_->onDouble = [this]() { performDoubleDuration(); };
    selectionToolbar_->onCancel = [this]() {
        clearSelection();
        hideSelectionToolbar();
    };
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
            if (patternModel_->hasLoopRegions())
                rebuildGhostNotes();
            pushRenderParams();
            noteLayer_->repaint();
            loopLayer_->repaint();
            ghostNoteLayer_->repaint();
        };
    }

    selection_.clear();
    hideSelectionToolbar();  // must run before resetting activeLoopIndex_ so deselectLoop() restores callbacks
    activeLoopIndex_ = -1;
    draggingNoteIndex_ = -1;
    rebuildGhostNotes();

    // Update all layers with new model
    gridLayer_->setPatternModel(model);
    noteLayer_->setPatternModel(model);
    loopLayer_->setPatternModel(model);
    ghostNoteLayer_->setPatternModel(model);
    overlayLayer_->setPatternModel(model);
    playheadLayer_->setPatternModel(model);

    pushRenderParams();
    gridLayer_->invalidateCache();
    noteLayer_->repaint();
    loopLayer_->repaint();
    ghostNoteLayer_->repaint();
    overlayLayer_->repaint();
}

void PianoRollWidget::rebuildVisiblePitches()
{
    if (!fixedPitches_.empty())
    {
        visiblePitches_ = fixedPitches_;
    }
    else
    {
        visiblePitches_ = MusicTheory::buildScalePitches(lowestNote_, highestNote_, scaleRoot_, scaleType_);
    }
}

void PianoRollWidget::setFixedPitches(const std::vector<int>& pitches)
{
    fixedPitches_ = pitches;
    rebuildVisiblePitches();
    pushRenderParams();
    gridLayer_->invalidateCache();
    noteLayer_->repaint();
}

void PianoRollWidget::setGridSize(double beats)
{
    gridSize_ = beats;
    pushRenderParams();
    gridLayer_->invalidateCache();
}

void PianoRollWidget::setPixelsPerBeat(double ppb)
{
    pixelsPerBeat_ = std::clamp(ppb, 15.0, 120.0);
    pushRenderParams();
    gridLayer_->invalidateCache();
    noteLayer_->repaint();
    loopLayer_->repaint();
    ghostNoteLayer_->repaint();
}

void PianoRollWidget::setNoteWidth(int w)
{
    noteWidth_ = w;
    pushRenderParams();
    gridLayer_->invalidateCache();
    noteLayer_->repaint();
}

void PianoRollWidget::setScale(int root, ScaleType type)
{
    // In drum mode, ignore scale changes — pitches are fixed to drum voices
    if (!fixedPitches_.empty())
        return;

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
    pushRenderParams();
    gridLayer_->invalidateCache();
    noteLayer_->repaint();
}

bool PianoRollWidget::isNoteInScale(int pitch) const
{
    return MusicTheory::isPitchInScale(pitch, scaleRoot_, scaleType_);
}

void PianoRollWidget::setStepRecordEnabled(bool enabled)
{
    stepRecordEnabled_ = enabled;
    if (enabled)
        overlayLayer_->setStepCursor(stepPosition_);
    else
        overlayLayer_->clearStepCursor();
}

void PianoRollWidget::resetStepPosition()
{
    stepPosition_ = 0.0;
    if (stepRecordEnabled_)
        overlayLayer_->setStepCursor(stepPosition_);
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

    noteLayer_->repaint();
    if (stepRecordEnabled_)
        overlayLayer_->setStepCursor(stepPosition_);
}

void PianoRollWidget::selectAll()
{
    selection_.selectAll(patternModel_);
    pushRenderParams();
    noteLayer_->repaint();
}

void PianoRollWidget::clearSelection()
{
    selection_.clear();
    hideSelectionToolbar();
    pushRenderParams();
    noteLayer_->repaint();
}

void PianoRollWidget::deleteSelected()
{
    selection_.deleteSelected(patternModel_);
    pushRenderParams();
    noteLayer_->repaint();
}

void PianoRollWidget::pushRenderParams()
{
    auto params = buildRenderParams();
    gridLayer_->setRenderParams(params);
    noteLayer_->setRenderParams(params);
    playheadLayer_->setRenderParams(params);
    loopLayer_->setRenderParams(params);
    ghostNoteLayer_->setRenderParams(params);
    overlayLayer_->setRenderParams(params);
}

void PianoRollWidget::updatePlayhead(double beats)
{
    pushRenderParams();
    playheadLayer_->updatePlayheadPosition(beats);
}

void PianoRollWidget::hidePlayhead()
{
    playheadLayer_->hidePlayhead();
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
    params.isChromatic = fixedPitches_.empty() && (scaleType_ == ScaleType::Chromatic);
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
}

void PianoRollWidget::stopNote(int pitch)
{
    if (onNoteOff)
        onNoteOff(pitch);
    if (playingNote_ == pitch)
        playingNote_ = -1;
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
    // Layers handle all rendering; parent just fills background behind any gaps
    g.fillAll(Theme::color(Theme::pianoRollBackground));
}

void PianoRollWidget::resized()
{
    auto bounds = getLocalBounds();

    // All layers cover the full widget area
    gridLayer_->setBounds(bounds);
    noteLayer_->setBounds(bounds);
    loopLayer_->setBounds(bounds);
    ghostNoteLayer_->setBounds(bounds);
    overlayLayer_->setBounds(bounds);
    playheadLayer_->setBounds(bounds);

    pushRenderParams();
    gridLayer_->invalidateCache();
}

void PianoRollWidget::mouseDown(const juce::MouseEvent& e)
{
    if (!patternModel_)
        return;

    // Hide toolbar if clicking outside it
    if (selectionToolbar_ && selectionToolbar_->isVisible())
    {
        auto toolbarBounds = selectionToolbar_->getBounds();
        if (!toolbarBounds.contains(e.getPosition()))
            hideSelectionToolbar();
    }

    grabKeyboardFocus();

    auto gridArea = getGridArea();

    auto [beat, pitch] = screenToNote(e.getPosition());
    double quantizedBeat = std::floor(beat / gridSize_) * gridSize_;

    // Prevent editing in the looped (ghost) region
    if (isInLoopedRegion(quantizedBeat, pitch))
        return;

    // Loop region interaction: select/resize by clicking on loop border edges
    if (!e.mods.isShiftDown() && patternModel_->hasLoopRegions())
    {
        // Check if clicking on the edge of any loop (selected or not)
        for (int i = 0; i < patternModel_->getNumLoopRegions(); ++i)
        {
            auto edge = findLoopEdge(i, e.getPosition());
            if (edge != LoopEdge::None)
            {
                if (i != activeLoopIndex_)
                    selectLoop(i, e.getPosition());
                resizingEdge_ = edge;
                dragMode_ = DragMode::LoopResize;
                dragStartBeat_ = beat;
                dragStartPitch_ = pitch;
                return;
            }
        }

        // Click outside all loops deselects
        if (activeLoopIndex_ >= 0)
        {
            int loopIdx = patternModel_->findLoopRegionAt(beat, pitch);
            if (loopIdx < 0)
                deselectLoop();
        }
    }

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
        pushRenderParams();
        noteLayer_->repaint();
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
            if (activeLoopIndex_ >= 0)
                deselectLoop();
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
    pushRenderParams();
    noteLayer_->repaint();
}

void PianoRollWidget::mouseDrag(const juce::MouseEvent& e)
{
    if (!patternModel_)
        return;

    if (dragMode_ == DragMode::PlayingPiano)
        return;

    auto gridArea = getGridArea();

    if (dragMode_ == DragMode::LoopResize)
    {
        if (activeLoopIndex_ < 0 || activeLoopIndex_ >= patternModel_->getNumLoopRegions())
            return;

        auto [beat, pitch] = screenToNote(e.getPosition());
        double quantizedBeat = std::floor(beat / gridSize_) * gridSize_;
        const auto& lr = patternModel_->getLoopRegions()[activeLoopIndex_];

        double newStart = lr.startBeat;
        double newEnd = lr.endBeat;
        int newMinPitch = lr.minPitch;
        int newMaxPitch = lr.maxPitch;

        switch (resizingEdge_)
        {
            case LoopEdge::Top:
                newStart = std::min(quantizedBeat, newEnd - gridSize_);
                break;
            case LoopEdge::Bottom:
                newEnd = std::max(quantizedBeat + gridSize_, newStart + gridSize_);
                break;
            case LoopEdge::Left:
                newMinPitch = std::min(pitch, newMaxPitch);
                break;
            case LoopEdge::Right:
                newMaxPitch = std::max(pitch, newMinPitch);
                break;
            default:
                break;
        }

        patternModel_->resizeLoopRegion(static_cast<size_t>(activeLoopIndex_),
                                         newStart, newEnd, newMinPitch, newMaxPitch);
        rebuildGhostNotes();
        loopLayer_->repaint();
        ghostNoteLayer_->repaint();
        return;
    }

    if (dragMode_ == DragMode::BoxSelect)
    {
        boxSelectEnd_ = e.getPosition();
        overlayLayer_->setBoxSelection(boxSelectStart_, boxSelectEnd_);
        return;
    }

    auto [beat, pitch] = screenToNote(e.getPosition());
    double quantizedBeat = std::floor(beat / gridSize_) * gridSize_;

    if (dragMode_ == DragMode::Drawing)
    {
        if (quantizedBeat != lastDrawnBeat_ || pitch != lastDrawnPitch_)
        {
            if (quantizedBeat >= 0 && quantizedBeat < patternModel_->lengthInBeats()
                && !isInLoopedRegion(quantizedBeat, pitch))
            {
                editor_.removeOverlappingNotes(patternModel_, pitch, quantizedBeat, quantizedBeat + gridSize_);
                editor_.addNote(patternModel_, quantizedBeat, gridSize_, pitch, 100);

                lastDrawnBeat_ = quantizedBeat;
                lastDrawnPitch_ = pitch;
                pushRenderParams();
                noteLayer_->repaint();
            }
        }
        return;
    }

    if (dragMode_ == DragMode::Erasing)
    {
        if (quantizedBeat != lastDrawnBeat_ || pitch != lastDrawnPitch_)
        {
            if (isInLoopedRegion(quantizedBeat, pitch))
            {
                lastDrawnBeat_ = quantizedBeat;
                lastDrawnPitch_ = pitch;
                return;
            }
            int noteIndex = patternModel_->findNoteContaining(quantizedBeat, pitch, 0.05);
            if (noteIndex >= 0)
            {
                patternModel_->removeNote(noteIndex);
                selection_.remove(noteIndex);
                pushRenderParams();
                noteLayer_->repaint();
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

    pushRenderParams();
    noteLayer_->repaint();
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

        overlayLayer_->clearBoxSelection();

        if (!selection_.empty())
        {
            // Store the beat + pitch range of the box selection for loop region
            auto [beatStart, pitchStart] = screenToNote(boxSelectStart_);
            auto [beatEnd, pitchEnd] = screenToNote(boxSelectEnd_);
            boxSelectBeatStart_ = std::floor(std::min(beatStart, beatEnd) / gridSize_) * gridSize_;
            boxSelectBeatEnd_ = std::ceil(std::max(beatStart, beatEnd) / gridSize_) * gridSize_;
            boxSelectMinPitch_ = std::min(pitchStart, pitchEnd);
            boxSelectMaxPitch_ = std::max(pitchStart, pitchEnd);

            showSelectionToolbar(e.getPosition());
        }
    }
    else if (dragMode_ == DragMode::LoopResize)
    {
        resizingEdge_ = LoopEdge::None;
        rebuildGhostNotes();
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
    pushRenderParams();
    noteLayer_->repaint();
}

void PianoRollWidget::mouseMove(const juce::MouseEvent& e)
{
    if (!patternModel_)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    auto gridArea = getGridArea();

    // Show resize cursor when hovering over any loop edge
    if (patternModel_->hasLoopRegions())
    {
        for (int i = 0; i < patternModel_->getNumLoopRegions(); ++i)
        {
            auto edge = findLoopEdge(i, e.getPosition());
            if (edge == LoopEdge::Top || edge == LoopEdge::Bottom)
            {
                setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
                return;
            }
            if (edge == LoopEdge::Left || edge == LoopEdge::Right)
            {
                setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
                return;
            }
        }
    }

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
            pushRenderParams();
            gridLayer_->invalidateCache();
            noteLayer_->repaint();
            loopLayer_->repaint();
            ghostNoteLayer_->repaint();

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
        pushRenderParams();
        noteLayer_->repaint();
        return true;
    }

    if (key.isKeyCode('V') && key.getModifiers().isCommandDown())
    {
        double pastePosition = stepRecordEnabled_ ? stepPosition_ : 0.0;
        selection_.paste(patternModel_, pastePosition);
        pushRenderParams();
        noteLayer_->repaint();
        return true;
    }

    if (key.isKeyCode(juce::KeyPress::deleteKey) ||
        key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        // Delete active loop if one is selected, otherwise delete selected notes
        if (activeLoopIndex_ >= 0)
        {
            deleteActiveLoop();
            return true;
        }
        deleteSelected();
        return true;
    }

    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        // Priority chain: toolbar → active loop → selection → all loops
        if (selectionToolbar_ && selectionToolbar_->isVisible())
        {
            hideSelectionToolbar();
            clearSelection();
            return true;
        }
        if (activeLoopIndex_ >= 0)
        {
            deselectLoop();
            return true;
        }
        if (!selection_.empty())
        {
            clearSelection();
            return true;
        }
        if (patternModel_ && patternModel_->hasLoopRegions())
        {
            patternModel_->clearLoopRegions();
            ghostNotes_.clear();
            ghostNoteLayer_->repaint();
            loopLayer_->repaint();
            return true;
        }
        return false;
    }

    return false;
}

// --- Selection toolbar ---

void PianoRollWidget::showSelectionToolbar(juce::Point<int> position)
{
    if (selectionToolbar_)
    {
        // Pre-compute ghost preview from the box selection beat range
        rebuildGhostNotes();
        selectionToolbar_->showAt(position);
    }
}

void PianoRollWidget::hideSelectionToolbar()
{
    // If a loop was selected, deselect it (restores toolbar callbacks)
    if (activeLoopIndex_ >= 0)
        deselectLoop();
    else if (selectionToolbar_)
        selectionToolbar_->dismiss();

    // Only clear ghosts if there's no active loop region
    if (!patternModel_ || !patternModel_->hasLoopRegions())
    {
        ghostNotes_.clear();
        ghostNoteLayer_->repaint();
    }
}

void PianoRollWidget::rebuildGhostNotes()
{
    ghostNotes_.clear();

    if (!patternModel_)
    {
        ghostNoteLayer_->setGhostNotes(&ghostNotes_);
        ghostNoteLayer_->repaint();
        return;
    }

    // If there are active loop regions, build ghosts from source notes in each region
    if (patternModel_->hasLoopRegions())
    {
        double patLen = patternModel_->lengthInBeats();

        for (const auto& lr : patternModel_->getLoopRegions())
        {
            double loopLen = lr.length();
            if (loopLen <= 0.0)
                continue;

            for (int i = 0; i < patternModel_->getNumNotes(); ++i)
            {
                double beat, dur;
                int pitch, vel;
                if (!patternModel_->getNoteAt(i, beat, dur, pitch, vel))
                    continue;

                if (beat < lr.startBeat || beat >= lr.endBeat)
                    continue;
                if (!lr.containsPitch(pitch))
                    continue;

                for (double offset = loopLen; lr.startBeat + offset < patLen; offset += loopLen)
                {
                    double ghostBeat = beat + offset;
                    if (ghostBeat >= patLen)
                        break;
                    double ghostDur = std::min(dur, patLen - ghostBeat);
                    ghostNotes_.push_back({ghostBeat, ghostDur, pitch});
                }
            }
        }

        ghostNoteLayer_->setGhostNotes(&ghostNotes_);
        ghostNoteLayer_->repaint();
        return;
    }

    // If a kernel is active, generate ghost notes for kernel-derived notes
    if (patternModel_ && patternModel_->getNumNotes() > 0)
    {
        // Read kernel from the auto-synced pattern (accessible via the engine)
        const PatternKernel* kernel = nullptr;
        if (engine_)
        {
            auto* activeModel = engine_->getActivePatternModel();
            if (activeModel == patternModel_)
            {
                int voice = engine_->getActiveVoice();
                kernel = &engine_->getProject().voices[voice].pattern.kernel;
            }
        }

        if (kernel && kernel->isActive())
        {
            double patLen = patternModel_->lengthInBeats();

            for (int i = 0; i < patternModel_->getNumNotes(); ++i)
            {
                double beat, dur;
                int pitch, vel;
                if (!patternModel_->getNoteAt(i, beat, dur, pitch, vel))
                    continue;

                // Build a temporary MIDINote for the kernel
                MIDINote srcNote(beat, dur, static_cast<uint8_t>(pitch),
                                 static_cast<uint8_t>(vel));

                // Apply kernel with iteration 0 for preview (static preview)
                for (const auto& cell : kernel->cells)
                {
                    // Skip the identity cell (pitchOffset=0, timeOffset=0) — that's the
                    // original note, already drawn by the NoteLayer
                    if (cell.pitchOffset == 0 && std::abs(cell.timeOffset) < 0.001)
                        continue;

                    int derivedPitch = pitch + cell.pitchOffset;
                    if (kernel->scaleAware && scaleType_ != ScaleType::Chromatic && cell.pitchOffset != 0)
                        derivedPitch = MusicTheory::findNearestScalePitch(derivedPitch, scaleRoot_, scaleType_);
                    derivedPitch = std::clamp(derivedPitch, 0, 127);

                    double derivedBeat = beat + cell.timeOffset;
                    if (derivedBeat < 0.0)
                        derivedBeat += patLen;
                    if (derivedBeat >= patLen)
                        derivedBeat = std::fmod(derivedBeat, patLen);

                    ghostNotes_.push_back({derivedBeat, dur, derivedPitch});
                }

                // For Invert mode, show the inverted note
                if (kernel->mode == KernelMode::Invert)
                {
                    int invertedPitch = 2 * kernel->pivotPitch - pitch;
                    invertedPitch = std::clamp(invertedPitch, 0, 127);
                    if (invertedPitch != pitch)
                        ghostNotes_.push_back({beat, dur, invertedPitch});
                }

                // For Retrograde mode, show the reversed note
                if (kernel->mode == KernelMode::Retrograde)
                {
                    double retroBeat = patLen - beat - dur;
                    if (retroBeat < 0.0) retroBeat = 0.0;
                    if (std::abs(retroBeat - beat) > 0.001)
                        ghostNotes_.push_back({retroBeat, dur, pitch});
                }
            }

            if (!ghostNotes_.empty())
            {
                ghostNoteLayer_->setGhostNotes(&ghostNotes_);
                ghostNoteLayer_->repaint();
                return;
            }
        }
    }

    // Otherwise, preview from the current box selection (before Loop is clicked)
    if (selection_.empty())
    {
        ghostNoteLayer_->setGhostNotes(&ghostNotes_);
        ghostNoteLayer_->repaint();
        return;
    }

    double loopStart = boxSelectBeatStart_;
    double loopEnd = boxSelectBeatEnd_;
    double loopLength = loopEnd - loopStart;
    if (loopLength <= 0.0)
    {
        ghostNoteLayer_->setGhostNotes(&ghostNotes_);
        ghostNoteLayer_->repaint();
        return;
    }

    double patternLength = patternModel_->lengthInBeats();

    for (int idx : selection_.getSelection())
    {
        double beat, dur;
        int pitch, vel;
        if (!patternModel_->getNoteAt(idx, beat, dur, pitch, vel))
            continue;

        for (double offset = loopLength; loopStart + offset < patternLength; offset += loopLength)
        {
            double ghostBeat = beat + offset;
            if (ghostBeat >= patternLength)
                break;
            double ghostDuration = std::min(dur, patternLength - ghostBeat);
            ghostNotes_.push_back({ghostBeat, ghostDuration, pitch});
        }
    }

    ghostNoteLayer_->setGhostNotes(&ghostNotes_);
    ghostNoteLayer_->repaint();
}

bool PianoRollWidget::isInLoopedRegion(double beat, int pitch) const
{
    if (!patternModel_ || !patternModel_->hasLoopRegions())
        return false;

    double patLen = patternModel_->lengthInBeats();

    for (const auto& lr : patternModel_->getLoopRegions())
    {
        if (!lr.containsPitch(pitch) || beat < lr.endBeat)
            continue;

        // Check if beat falls within any repetition tile of this loop
        double loopLen = lr.length();
        if (loopLen <= 0.0)
            continue;

        for (double offset = loopLen; lr.startBeat + offset < patLen; offset += loopLen)
        {
            double repStart = lr.startBeat + offset;
            double repEnd = std::min(repStart + loopLen, patLen);
            if (beat >= repStart && beat < repEnd)
                return true;
        }
    }
    return false;
}

// --- Loop interaction ---

juce::Rectangle<int> PianoRollWidget::loopRegionToScreen(int loopIndex) const
{
    if (!patternModel_ || loopIndex < 0 || loopIndex >= patternModel_->getNumLoopRegions())
        return {};

    const auto& lr = patternModel_->getLoopRegions()[loopIndex];
    auto gridArea = getGridArea();
    auto params = buildRenderParams();

    // Find column bounds from visible pitches
    int minCol = -1, maxCol = -1;
    if (params.visiblePitches)
    {
        for (int i = 0; i < static_cast<int>(params.visiblePitches->size()); ++i)
        {
            int p = (*params.visiblePitches)[i];
            if (p >= lr.minPitch && p <= lr.maxPitch)
            {
                if (minCol < 0) minCol = i;
                maxCol = i;
            }
        }
    }
    if (minCol < 0 || maxCol < 0)
        return {};

    int x = gridArea.getX() + minCol * params.noteWidth;
    int w = (maxCol - minCol + 1) * params.noteWidth;
    int y = gridArea.getY() + static_cast<int>(lr.startBeat * params.pixelsPerBeat);
    int h = static_cast<int>(lr.length() * params.pixelsPerBeat);

    return {x, y, w, h};
}

PianoRollWidget::LoopEdge PianoRollWidget::findLoopEdge(int loopIndex, juce::Point<int> pos,
                                                         int tolerance) const
{
    auto rect = loopRegionToScreen(loopIndex);
    if (rect.isEmpty())
        return LoopEdge::None;

    // Detect clicks in an inside zone along each edge
    bool inHorizRange = pos.x >= rect.getX() && pos.x <= rect.getRight();
    bool inVertRange = pos.y >= rect.getY() && pos.y <= rect.getBottom();

    bool nearTop = pos.y >= rect.getY() && pos.y <= rect.getY() + tolerance;
    bool nearBottom = pos.y >= rect.getBottom() - tolerance && pos.y <= rect.getBottom();
    bool nearLeft = pos.x >= rect.getX() && pos.x <= rect.getX() + tolerance;
    bool nearRight = pos.x >= rect.getRight() - tolerance && pos.x <= rect.getRight();

    if (nearTop && inHorizRange) return LoopEdge::Top;
    if (nearBottom && inHorizRange) return LoopEdge::Bottom;
    if (nearLeft && inVertRange) return LoopEdge::Left;
    if (nearRight && inVertRange) return LoopEdge::Right;

    return LoopEdge::None;
}

void PianoRollWidget::selectLoop(int index, juce::Point<int> position)
{
    // Clear selection and dismiss toolbar directly (don't use hideSelectionToolbar
    // which would call deselectLoop and reset activeLoopIndex_)
    selection_.clear();
    if (selectionToolbar_)
        selectionToolbar_->dismiss();
    ghostNotes_.clear();

    activeLoopIndex_ = index;
    loopLayer_->setActiveLoopIndex(index);

    // Show toolbar with Delete/Cancel for the loop
    if (selectionToolbar_)
    {
        selectionToolbar_->onLoop = nullptr;
        selectionToolbar_->onDelete = [this]() { deleteActiveLoop(); };
        selectionToolbar_->onInvert = nullptr;
        selectionToolbar_->onHalve = nullptr;
        selectionToolbar_->onDouble = nullptr;
        selectionToolbar_->onCancel = [this]() { deselectLoop(); };
        selectionToolbar_->showAt(position);
    }

    pushRenderParams();
    noteLayer_->repaint();
    loopLayer_->repaint();
    ghostNoteLayer_->repaint();
}

void PianoRollWidget::deselectLoop()
{
    activeLoopIndex_ = -1;
    resizingEdge_ = LoopEdge::None;
    loopLayer_->setActiveLoopIndex(-1);

    if (selectionToolbar_)
        selectionToolbar_->dismiss();

    // Restore normal toolbar callbacks
    if (selectionToolbar_)
    {
        selectionToolbar_->onLoop = [this]() { performLoop(); };
        selectionToolbar_->onDelete = [this]() {
            deleteSelected();
            hideSelectionToolbar();
        };
        selectionToolbar_->onInvert = [this]() { performInvert(); };
        selectionToolbar_->onHalve = [this]() { performHalveDuration(); };
        selectionToolbar_->onDouble = [this]() { performDoubleDuration(); };
        selectionToolbar_->onCancel = [this]() {
            clearSelection();
            hideSelectionToolbar();
        };
    }

    loopLayer_->repaint();
}

void PianoRollWidget::deleteActiveLoop()
{
    if (!patternModel_ || activeLoopIndex_ < 0)
        return;

    int idx = activeLoopIndex_;
    deselectLoop();
    patternModel_->removeLoopRegion(static_cast<size_t>(idx));
    rebuildGhostNotes();
    loopLayer_->repaint();
}

// --- Toolbar operations ---

void PianoRollWidget::performLoop()
{
    if (!patternModel_)
        return;

    // Use the box selection beat range as the loop region
    double loopStart = boxSelectBeatStart_;
    double loopEnd = boxSelectBeatEnd_;

    if (loopEnd - loopStart <= 0.0)
        return;

    // Set the non-destructive loop region on the pattern model
    patternModel_->addLoopRegion(loopStart, loopEnd, boxSelectMinPitch_, boxSelectMaxPitch_);

    selectionToolbar_->dismiss();
    selection_.clear();
    rebuildGhostNotes();
    pushRenderParams();
    noteLayer_->repaint();
    loopLayer_->repaint();
}

void PianoRollWidget::performInvert()
{
    if (!patternModel_ || selection_.empty())
        return;

    // Find min and max pitch
    int minPitch = 127, maxPitch = 0;
    for (int idx : selection_.getSelection())
    {
        double beat, dur;
        int pitch, vel;
        if (!patternModel_->getNoteAt(idx, beat, dur, pitch, vel))
            continue;
        minPitch = std::min(minPitch, pitch);
        maxPitch = std::max(maxPitch, pitch);
    }

    int centerPitch = minPitch + maxPitch;

    patternModel_->beginTransaction("Invert");

    for (int idx : selection_.getSelection())
    {
        double beat, dur;
        int pitch, vel;
        if (!patternModel_->getNoteAt(idx, beat, dur, pitch, vel))
            continue;

        int newPitch = centerPitch - pitch;
        patternModel_->moveNote(idx, beat, newPitch);
    }

    rebuildGhostNotes();
    pushRenderParams();
    noteLayer_->repaint();
}

void PianoRollWidget::performHalveDuration()
{
    if (!patternModel_ || selection_.empty())
        return;

    patternModel_->beginTransaction("Halve Duration");

    for (int idx : selection_.getSelection())
    {
        double beat, dur;
        int pitch, vel;
        if (!patternModel_->getNoteAt(idx, beat, dur, pitch, vel))
            continue;

        double newDuration = std::max(gridSize_, dur * 0.5);
        patternModel_->resizeNote(idx, newDuration);
    }

    rebuildGhostNotes();
    pushRenderParams();
    noteLayer_->repaint();
}

void PianoRollWidget::performDoubleDuration()
{
    if (!patternModel_ || selection_.empty())
        return;

    double patternLength = patternModel_->lengthInBeats();

    patternModel_->beginTransaction("Double Duration");

    for (int idx : selection_.getSelection())
    {
        double beat, dur;
        int pitch, vel;
        if (!patternModel_->getNoteAt(idx, beat, dur, pitch, vel))
            continue;

        double newDuration = std::min(dur * 2.0, patternLength - beat);
        patternModel_->resizeNote(idx, newDuration);
    }

    rebuildGhostNotes();
    pushRenderParams();
    noteLayer_->repaint();
}

} // namespace SurgeBox
