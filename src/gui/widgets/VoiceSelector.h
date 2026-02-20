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

class VoiceSelector : public juce::Component
{
  public:
    VoiceSelector();
    ~VoiceSelector() override = default;

    void setEngine(SurgeBoxEngine *engine);

    void resized() override;

    void selectVoice(int voice);
    void selectFX();
    void selectChordTrack();

    std::function<void(bool)> onFXSelected;
    std::function<void(bool)> onChordTrackSelected;

  private:
    SurgeBoxEngine *engine_{nullptr};
    std::unique_ptr<Widgets::MultiSwitch> multiSwitch_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceSelector)
};

} // namespace SurgeBox
