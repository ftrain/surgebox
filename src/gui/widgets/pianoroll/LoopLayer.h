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

class LoopLayer : public PianoRollLayer
{
  public:
    LoopLayer() = default;
    void paint(juce::Graphics& g) override;
    void setActiveLoopIndex(int index) { activeLoopIndex_ = index; repaint(); }

  private:
    int activeLoopIndex_{-1};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopLayer)
};

} // namespace SurgeBox
