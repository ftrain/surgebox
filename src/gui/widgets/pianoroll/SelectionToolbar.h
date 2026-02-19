/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Floating toolbar that appears after box-selecting notes in the piano roll.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "gui/widgets/SelfDrawButton.h"
#include <functional>
#include <memory>

namespace SurgeBox
{
namespace PianoRoll
{

class SelectionToolbar : public juce::Component
{
  public:
    SelectionToolbar();
    ~SelectionToolbar() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Show at a specific position (relative to parent)
    void showAt(juce::Point<int> position);
    void dismiss();

    // Button callbacks
    std::function<void()> onLoop;
    std::function<void()> onDelete;
    std::function<void()> onInvert;
    std::function<void()> onHalve;
    std::function<void()> onDouble;
    std::function<void()> onCancel;

  private:
    static constexpr int kButtonWidth = 42;
    static constexpr int kButtonHeight = 22;
    static constexpr int kPadding = 4;
    static constexpr int kNumButtons = 6;

    std::unique_ptr<Widgets::SelfDrawButton> loopBtn_;
    std::unique_ptr<Widgets::SelfDrawButton> deleteBtn_;
    std::unique_ptr<Widgets::SelfDrawButton> invertBtn_;
    std::unique_ptr<Widgets::SelfDrawButton> halveBtn_;
    std::unique_ptr<Widgets::SelfDrawButton> doubleBtn_;
    std::unique_ptr<Widgets::SelfDrawButton> cancelBtn_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectionToolbar)
};

} // namespace PianoRoll
} // namespace SurgeBox
