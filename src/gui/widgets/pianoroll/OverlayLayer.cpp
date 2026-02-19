/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "OverlayLayer.h"

namespace SurgeBox
{

void OverlayLayer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (showStepCursor_)
        PianoRoll::drawStepCursor(g, bounds, stepPosition_, params_.pixelsPerBeat);

    if (showBoxSelect_)
        PianoRoll::drawBoxSelection(g, boxSelectStart_, boxSelectEnd_, bounds, params_);
}

void OverlayLayer::setBoxSelection(juce::Point<int> start, juce::Point<int> end)
{
    boxSelectStart_ = start;
    boxSelectEnd_ = end;
    showBoxSelect_ = true;
    repaint();
}

void OverlayLayer::clearBoxSelection()
{
    if (showBoxSelect_)
    {
        showBoxSelect_ = false;
        repaint();
    }
}

void OverlayLayer::setStepCursor(double position)
{
    stepPosition_ = position;
    showStepCursor_ = true;
    repaint();
}

void OverlayLayer::clearStepCursor()
{
    if (showStepCursor_)
    {
        showStepCursor_ = false;
        repaint();
    }
}

} // namespace SurgeBox
