/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Base class for piano roll rendering layers.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PianoRollRenderer.h"

namespace SurgeBox
{

class PatternModel;

class PianoRollLayer : public juce::Component
{
  public:
    PianoRollLayer()
    {
        setInterceptsMouseClicks(false, false);
        setPaintingIsUnclipped(true);
    }

    void setRenderParams(const PianoRoll::RenderParams& p) { params_ = p; }
    void setPatternModel(PatternModel* m) { patternModel_ = m; }

  protected:
    PianoRoll::RenderParams params_;
    PatternModel* patternModel_{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollLayer)
};

} // namespace SurgeBox
