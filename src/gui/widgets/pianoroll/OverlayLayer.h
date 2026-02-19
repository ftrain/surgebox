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

class OverlayLayer : public PianoRollLayer
{
  public:
    OverlayLayer() = default;
    void paint(juce::Graphics& g) override;

    void setBoxSelection(juce::Point<int> start, juce::Point<int> end);
    void clearBoxSelection();

    void setStepCursor(double position);
    void clearStepCursor();

  private:
    bool showBoxSelect_{false};
    juce::Point<int> boxSelectStart_;
    juce::Point<int> boxSelectEnd_;

    bool showStepCursor_{false};
    double stepPosition_{0.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayLayer)
};

} // namespace SurgeBox
