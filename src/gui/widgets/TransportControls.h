/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SurgeBoxWidgets.h"

namespace SurgeBox
{

class SurgeBoxEngine;

class TransportControls : public juce::Component
{
  public:
    TransportControls();
    ~TransportControls() override = default;

    void setEngine(SurgeBoxEngine *engine);
    void updateDisplay();

    void resized() override;

  private:
    SurgeBoxEngine *engine_{nullptr};

    std::unique_ptr<Widgets::IconButton> playButton_;
    std::unique_ptr<Widgets::IconButton> stopButton_;
    std::unique_ptr<Widgets::NumberField> tempoField_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportControls)
};

} // namespace SurgeBox
