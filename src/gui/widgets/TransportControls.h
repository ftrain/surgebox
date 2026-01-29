/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

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

    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &e) override;

  private:
    SurgeBoxEngine *engine_{nullptr};

    juce::Rectangle<int> playBounds_;
    juce::Rectangle<int> stopBounds_;
    juce::Rectangle<int> tempoBounds_;

    juce::Colour bgColor_{0xff16213e};
    juce::Colour buttonColor_{0xff1a1a2e};
    juce::Colour playColor_{0xff4caf50};
    juce::Colour stopColor_{0xfff44336};
    juce::Colour textColor_{0xffffffff};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportControls)
};

} // namespace SurgeBox
