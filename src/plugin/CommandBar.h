/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Menu bar with dropdown menus, voice selector, instrument picker, and transport.
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

class CommandBar : public juce::Component
{
  public:
    explicit CommandBar(SurgeBoxEngine& engine);
    ~CommandBar() override = default;

    void resized() override;

    void updateMeasuresLabel();
    void updateTempoMultiplier(double multiplier);
    void setTempo(double bpm);

    // Access components for external listeners
    VoiceSelector& getVoiceSelector() { return *voiceSelector_; }
    TransportControls& getTransportControls() { return *transport_; }

    // Callbacks
    std::function<void(bool)> onStepRecordChanged;
    std::function<void(double)> onGridSizeChanged;
    std::function<void(int, ScaleType)> onScaleChanged;
    std::function<void()> onClearPattern;
    std::function<void()> onMeasuresChanged;
    std::function<void(SurgeBox::InstrumentType)> onInstrumentChanged;
    std::function<void()> onSavePreset;
    std::function<void()> onLoadPreset;

    void updateInstrumentSelector();

  private:
    SurgeBoxEngine& engine_;

    // Menu buttons
    juce::TextButton patternMenuBtn_{"Pattern"};
    juce::TextButton editMenuBtn_{"Edit"};
    juce::TextButton scaleMenuBtn_{"Scale"};
    juce::TextButton viewMenuBtn_{"View"};

    // Direct widgets (right side)
    std::unique_ptr<VoiceSelector> voiceSelector_;
    std::unique_ptr<TransportControls> transport_;
    std::unique_ptr<juce::ComboBox> instrumentCombo_;
    std::unique_ptr<juce::Label> instrumentLabel_;

    // Internal state for menu ticks
    int gridSizeId_{3};         // 1=1/4, 2=1/8, 3=1/16, 4=1/32
    int scaleRootId_{1};        // 1=C, 2=C#, ...
    int scaleTypeId_{1};        // 1=Chromatic, 2=Major, ...
    int tempoMultiplierId_{3};  // 1=4x, 2=2x, 3=1x, ...
    bool stepRecordEnabled_{false};
    bool midiLearnActive_{false};
    int currentBars_{1};
    double currentTempo_{120.0};

    void showPatternMenu();
    void showEditMenu();
    void showScaleMenu();
    void showViewMenu();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandBar)
};

} // namespace SurgeBox
