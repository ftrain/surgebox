/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "GridLayer.h"
#include "core/PatternModel.h"
#include "gui/Theme.h"

namespace SurgeBox
{

bool GridLayer::isCacheValid() const
{
    if (!cacheValid_ || cachedImage_.isNull())
        return false;

    int pitchCount = params_.visiblePitches ? static_cast<int>(params_.visiblePitches->size()) : 0;

    return cachedWidth_ == getWidth() &&
           cachedHeight_ == getHeight() &&
           cachedPixelsPerBeat_ == params_.pixelsPerBeat &&
           cachedGridSize_ == params_.gridSize &&
           cachedNoteWidth_ == params_.noteWidth &&
           cachedVisiblePitchCount_ == pitchCount;
}

void GridLayer::invalidateCache()
{
    cacheValid_ = false;
    repaint();
}

void GridLayer::paint(juce::Graphics& g)
{
    if (isCacheValid())
    {
        g.drawImageAt(cachedImage_, 0, 0);
        return;
    }

    // Render to cached image
    auto bounds = getLocalBounds();
    if (bounds.isEmpty())
        return;

    cachedImage_ = juce::Image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
    juce::Graphics ig(cachedImage_);

    ig.fillAll(Theme::color(Theme::pianoRollBackground));
    PianoRoll::drawGrid(ig, bounds, patternModel_, params_);

    // Store cache keys
    cachedWidth_ = getWidth();
    cachedHeight_ = getHeight();
    cachedPixelsPerBeat_ = params_.pixelsPerBeat;
    cachedGridSize_ = params_.gridSize;
    cachedNoteWidth_ = params_.noteWidth;
    cachedVisiblePitchCount_ = params_.visiblePitches ? static_cast<int>(params_.visiblePitches->size()) : 0;
    cacheValid_ = true;

    g.drawImageAt(cachedImage_, 0, 0);
}

} // namespace SurgeBox
