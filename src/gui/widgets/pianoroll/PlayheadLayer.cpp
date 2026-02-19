/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "PlayheadLayer.h"

namespace SurgeBox
{

void PlayheadLayer::paint(juce::Graphics& g)
{
    if (!visible_ || playheadBeats_ < 0.0)
        return;

    auto bounds = getLocalBounds();
    PianoRoll::drawPlayhead(g, bounds, playheadBeats_, params_.pixelsPerBeat);
}

void PlayheadLayer::updatePlayheadPosition(double beats)
{
    if (!visible_ || beats != playheadBeats_)
    {
        auto bounds = getLocalBounds();

        // Invalidate old position
        if (visible_ && playheadBeats_ >= 0.0)
        {
            int oldY = static_cast<int>(playheadBeats_ * params_.pixelsPerBeat);
            repaint(0, oldY - kStripHeight, getWidth(), kStripHeight * 2);
        }

        playheadBeats_ = beats;
        visible_ = true;

        // Invalidate new position
        int newY = static_cast<int>(beats * params_.pixelsPerBeat);
        repaint(0, newY - kStripHeight, getWidth(), kStripHeight * 2);
    }
}

void PlayheadLayer::hidePlayhead()
{
    if (visible_)
    {
        visible_ = false;
        repaint();
    }
}

} // namespace SurgeBox
