/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "NoteLayer.h"
#include "core/PatternModel.h"
#include "core/MusicTheory.h"
#include "gui/Theme.h"

namespace SurgeBox
{

void NoteLayer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    auto clipBounds = g.getClipBounds();

    if (!patternModel_ || bounds.isEmpty())
        return;

    // Only draw notes whose screen rects intersect the clip bounds
    for (int i = 0; i < patternModel_->getNumNotes(); ++i)
    {
        auto noteRect = PianoRoll::noteToScreen(i, bounds, patternModel_, params_);
        if (noteRect.isEmpty() || !noteRect.intersects(clipBounds))
            continue;

        double startBeat, duration;
        int pitch, velocity;
        patternModel_->getNoteAt(i, startBeat, duration, pitch, velocity);

        bool selected = params_.selectedNotes && params_.selectedNotes->count(i) > 0;
        bool inScale = params_.isChromatic ||
                       MusicTheory::isPitchInScale(pitch, params_.scaleRoot,
                                                   params_.isChromatic ? ScaleType::Chromatic : ScaleType::Major);

        if (!inScale && !params_.isChromatic)
        {
            g.setColour((selected ? Theme::color(Theme::noteSelected) : Theme::color(Theme::noteColor))
                            .withAlpha(0.4f));
        }
        else
        {
            g.setColour(selected ? Theme::color(Theme::noteSelected) : Theme::color(Theme::noteColor));
        }
        g.fillRoundedRectangle(noteRect.toFloat(), 3.0f);

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(noteRect.toFloat(), 3.0f, 1.0f);

        float velAlpha = 1.0f - (static_cast<float>(velocity) / 127.0f) * 0.5f;
        g.setColour(juce::Colours::black.withAlpha(velAlpha * 0.3f));
        g.fillRoundedRectangle(noteRect.toFloat(), 3.0f);
    }
}

void NoteLayer::repaintNote(int noteIndex)
{
    if (!patternModel_ || noteIndex < 0 || noteIndex >= patternModel_->getNumNotes())
    {
        repaint();
        return;
    }

    auto bounds = getLocalBounds();
    auto noteRect = PianoRoll::noteToScreen(noteIndex, bounds, patternModel_, params_);
    if (!noteRect.isEmpty())
        repaint(noteRect.expanded(2));
    else
        repaint();
}

} // namespace SurgeBox
