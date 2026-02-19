/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "GhostNoteLayer.h"

namespace SurgeBox
{

void GhostNoteLayer::paint(juce::Graphics& g)
{
    if (!ghostNotes_ || ghostNotes_->empty())
        return;

    auto bounds = getLocalBounds();
    PianoRoll::drawGhostNotes(g, bounds, *ghostNotes_, params_);
}

} // namespace SurgeBox
