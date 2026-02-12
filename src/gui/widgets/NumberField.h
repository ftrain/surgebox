/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Draggable/scrollable number display widget.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>

namespace SurgeBox
{
namespace Widgets
{

class NumberField : public juce::Component
{
  public:
    NumberField();

    void setRange(int minVal, int maxVal);
    int getValue() const { return value_; }
    void setValue(int v);
    void setSuffix(const std::string& s) { suffix_ = s; }

    std::function<void(int)> onValueChanged = [](int) {};

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

  private:
    int value_{0};
    int minValue_{0};
    int maxValue_{100};
    std::string suffix_;
    bool isHovered_{false};
    int dragStartY_{0};
    int dragStartValue_{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NumberField)
};

} // namespace Widgets
} // namespace SurgeBox
