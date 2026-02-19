/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "LoopLayer.h"
#include "core/PatternModel.h"

namespace SurgeBox
{

void LoopLayer::paint(juce::Graphics& g)
{
    if (!patternModel_ || !patternModel_->hasLoopRegions())
        return;

    auto bounds = getLocalBounds();
    const auto& regions = patternModel_->getLoopRegions();

    for (int i = 0; i < static_cast<int>(regions.size()); ++i)
    {
        const auto& lr = regions[i];
        PianoRoll::drawLoopRegion(g, bounds, lr.startBeat, lr.endBeat,
                                  lr.minPitch, lr.maxPitch,
                                  patternModel_->lengthInBeats(), params_,
                                  i == activeLoopIndex_);
    }
}

} // namespace SurgeBox
