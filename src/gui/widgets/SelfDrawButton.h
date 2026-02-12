/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Self-drawing clickable button widget.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <string>

namespace SurgeBox
{
namespace Widgets
{

class SelfDrawButton : public juce::Component
{
  public:
    explicit SelfDrawButton(const std::string& label);

    std::function<void()> onClick = []() {};

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

  private:
    std::string label_;
    bool isHovered_{false};
    bool isDown_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelfDrawButton)
};

} // namespace Widgets
} // namespace SurgeBox
