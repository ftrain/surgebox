/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "core/GrooveboxProject.h"
#include <memory>
#include <functional>

namespace SurgeBox
{

class SurgeBoxEngine;
class PatternModel;

/**
 * KernelEditorPanel - UI for editing the pattern matrix kernel.
 *
 * Shows in the instrument viewport area. Provides:
 * - Mode selector (Off, Spawn, Transform, Invert, Retrograde)
 * - Preset picker (10 factory presets)
 * - Kernel cell grid (pitch offset, time offset, velocity, probability per cell)
 * - Parameters (scale-aware, pivot, accumulate, follow chords, snap to chord)
 */
class KernelEditorPanel : public juce::Component,
                          public juce::ComboBox::Listener,
                          public juce::Slider::Listener,
                          public juce::Button::Listener
{
  public:
    KernelEditorPanel();
    ~KernelEditorPanel() override = default;

    void setEngine(SurgeBoxEngine *engine);
    void refreshFromPattern();

    void paint(juce::Graphics &g) override;
    void resized() override;

    // Listeners
    void comboBoxChanged(juce::ComboBox *comboBox) override;
    void sliderValueChanged(juce::Slider *slider) override;
    void buttonClicked(juce::Button *button) override;

    // Called when kernel data changes (so piano roll can update ghost notes)
    std::function<void()> onKernelChanged;

  private:
    static constexpr int MAX_VISIBLE_CELLS = 8;

    void applyKernelToPattern();
    void loadKernelFromPattern();
    void addCell();
    void removeCell(int index);

    SurgeBoxEngine *engine_{nullptr};

    // Mode selector
    std::unique_ptr<juce::Label> modeLabel_;
    std::unique_ptr<juce::ComboBox> modeCombo_;

    // Preset selector
    std::unique_ptr<juce::Label> presetLabel_;
    std::unique_ptr<juce::ComboBox> presetCombo_;

    // Parameter controls
    std::unique_ptr<juce::ToggleButton> scaleAwareToggle_;
    std::unique_ptr<juce::Label> scaleRootLabel_;
    std::unique_ptr<juce::ComboBox> scaleRootCombo_;
    std::unique_ptr<juce::Label> scaleTypeLabel_;
    std::unique_ptr<juce::ComboBox> scaleTypeCombo_;
    std::unique_ptr<juce::Label> pivotLabel_;
    std::unique_ptr<juce::Slider> pivotSlider_;
    std::unique_ptr<juce::Label> accumLabel_;
    std::unique_ptr<juce::Slider> accumSlider_;
    std::unique_ptr<juce::Label> resetLabel_;
    std::unique_ptr<juce::Slider> resetSlider_;
    std::unique_ptr<juce::Label> seedLabel_;
    std::unique_ptr<juce::Slider> seedSlider_;
    std::unique_ptr<juce::ToggleButton> followChordsToggle_;
    std::unique_ptr<juce::ToggleButton> snapToChordToggle_;

    // Cell editors
    struct CellUI
    {
        std::unique_ptr<juce::Label> indexLabel;
        std::unique_ptr<juce::Slider> pitchSlider;
        std::unique_ptr<juce::Slider> timeSlider;
        std::unique_ptr<juce::Slider> velocitySlider;
        std::unique_ptr<juce::Slider> probabilitySlider;
        std::unique_ptr<juce::TextButton> removeButton;
    };
    std::vector<CellUI> cellUIs_;

    std::unique_ptr<juce::TextButton> addCellButton_;

    // Local kernel state for editing
    PatternKernel editKernel_;
    bool snapToChord_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KernelEditorPanel)
};

} // namespace SurgeBox
