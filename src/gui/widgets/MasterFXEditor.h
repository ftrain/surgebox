/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>

namespace SurgeBox
{

class MasterFXChain;

class MasterFXEditor : public juce::Component,
                       public juce::ComboBox::Listener,
                       public juce::Slider::Listener,
                       public juce::Button::Listener
{
  public:
    explicit MasterFXEditor(MasterFXChain &fxChain);
    ~MasterFXEditor() override = default;

    void resized() override;
    void paint(juce::Graphics &g) override;

    void comboBoxChanged(juce::ComboBox *comboBox) override;
    void sliderValueChanged(juce::Slider *slider) override;
    void buttonClicked(juce::Button *button) override;

    // Refresh UI from current FX chain state
    void refreshFromChain();

  private:
    static constexpr int NUM_SLOTS = 4;
    static constexpr int NUM_PARAMS = 12;

    MasterFXChain &fxChain_;

    struct SlotUI
    {
        std::unique_ptr<juce::ToggleButton> enableButton;
        std::unique_ptr<juce::ComboBox> typeCombo;
        std::unique_ptr<juce::Label> slotLabel;
        std::array<std::unique_ptr<juce::Slider>, 12> paramSliders;
        std::array<std::unique_ptr<juce::Label>, 12> paramLabels;
    };

    std::array<SlotUI, NUM_SLOTS> slots_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterFXEditor)
};

} // namespace SurgeBox
