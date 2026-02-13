/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Command bar containing voice selector, transport, scale picker, and measure buttons.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "gui/widgets/VoiceSelector.h"
#include "gui/widgets/TransportControls.h"
#include "core/MusicTheory.h"
#include "core/GrooveboxProject.h"

namespace SurgeBox
{

class SurgeBoxEngine;
class PatternModel;

class CommandBar : public juce::Component,
                   public juce::Button::Listener,
                   public juce::ComboBox::Listener,
                   public juce::Slider::Listener
{
  public:
    explicit CommandBar(SurgeBoxEngine& engine);
    ~CommandBar() override = default;

    void resized() override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void sliderValueChanged(juce::Slider* slider) override;

    void updateMeasuresLabel();
    void updateTempoMultiplier(double multiplier);

    // Access components for external listeners
    VoiceSelector& getVoiceSelector() { return *voiceSelector_; }
    TransportControls& getTransportControls() { return *transport_; }
    juce::TextButton& getStepRecordButton() { return *stepRecordButton_; }

    // Callbacks
    std::function<void(bool)> onStepRecordChanged;
    std::function<void(double)> onGridSizeChanged;
    std::function<void(int, ScaleType)> onScaleChanged;
    std::function<void()> onClearPattern;
    std::function<void()> onMeasuresChanged;
    std::function<void(bool)> onMasterFXToggled;
    std::function<void(SurgeBox::InstrumentType)> onInstrumentChanged;

    void updateInstrumentSelector();

  private:
    SurgeBoxEngine& engine_;

    std::unique_ptr<VoiceSelector> voiceSelector_;
    std::unique_ptr<TransportControls> transport_;

    std::unique_ptr<juce::TextButton> stepRecordButton_;

    // Measure controls
    std::unique_ptr<juce::TextButton> measuresDoubleBtn_;
    std::unique_ptr<juce::TextButton> measuresHalfBtn_;
    std::unique_ptr<juce::TextButton> measuresAddBtn_;
    std::unique_ptr<juce::TextButton> measuresSubBtn_;
    std::unique_ptr<juce::Label> measuresLabel_;

    // Grid size
    std::unique_ptr<juce::ComboBox> gridSizeCombo_;
    std::unique_ptr<juce::Label> gridSizeLabel_;

    // Scale picker
    std::unique_ptr<juce::ComboBox> scaleRootCombo_;
    std::unique_ptr<juce::ComboBox> scaleTypeCombo_;
    std::unique_ptr<juce::Label> scaleLabel_;

    // Clear pattern button
    std::unique_ptr<juce::TextButton> clearPatternBtn_;

    // Tempo control
    std::unique_ptr<juce::Slider> tempoSlider_;
    std::unique_ptr<juce::Label> tempoLabel_;
    std::unique_ptr<juce::ComboBox> tempoMultiplierCombo_;

    // Master FX toggle
    std::unique_ptr<juce::TextButton> masterFXButton_;

    // Instrument selector
    std::unique_ptr<juce::ComboBox> instrumentCombo_;
    std::unique_ptr<juce::Label> instrumentLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandBar)
};

} // namespace SurgeBox
