/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Multi-selection switch widget.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>
#include <string>

namespace SurgeBox
{
namespace Widgets
{

class MultiSwitch : public juce::Component
{
  public:
    MultiSwitch();

    void setLabels(const std::vector<std::string>& labels);
    void setRows(int r) { rows_ = r; }
    void setColumns(int c) { columns_ = c; }
    int getRows() const { return rows_; }
    int getColumns() const { return columns_; }

    int getValue() const { return value_; }
    void setValue(int v);

    std::function<void(int)> onValueChanged = [](int) {};

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

  private:
    int coordinateToIndex(int x, int y) const;

    std::vector<std::string> labels_;
    int rows_{0}, columns_{0};
    int value_{0};
    bool isHovered_{false};
    int hoverIndex_{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiSwitch)
};

} // namespace Widgets
} // namespace SurgeBox
