/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include "PianoRollLayer.h"

namespace SurgeBox
{

class GridLayer : public PianoRollLayer
{
  public:
    GridLayer() { setOpaque(true); }

    void paint(juce::Graphics& g) override;
    void invalidateCache();

  private:
    juce::Image cachedImage_;
    bool cacheValid_{false};
    int cachedWidth_{0};
    int cachedHeight_{0};
    double cachedPixelsPerBeat_{0};
    double cachedGridSize_{0};
    int cachedNoteWidth_{0};
    int cachedVisiblePitchCount_{0};

    bool isCacheValid() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GridLayer)
};

} // namespace SurgeBox
