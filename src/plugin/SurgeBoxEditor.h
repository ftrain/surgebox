/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SurgeBoxProcessor.h"
#include "CommandBar.h"
#include "gui/widgets/PianoRollWidget.h"
#include "gui/widgets/PianoKeyboardWidget.h"
#include "gui/widgets/MasterFXEditor.h"
#include "gui/widgets/KernelEditorPanel.h"
#include "gui/SurgeBoxLookAndFeel.h"
#include "gui/Layout.h"
#include "core/VoicePreset.h"

class SurgeBoxEditor : public juce::AudioProcessorEditor,
                       public juce::Timer,
                       public juce::MidiKeyboardState::Listener,
                       public juce::ScrollBar::Listener
{
  public:
    explicit SurgeBoxEditor(SurgeBoxProcessor&);
    ~SurgeBoxEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key) override;

    // MidiKeyboardState::Listener - captures notes for step recording
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber,
                      float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber,
                       float velocity) override;

    // ScrollBar::Listener
    void scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) override;

  private:
    SurgeBoxProcessor& processor_;
    SurgeBox::SurgeBoxEngine& engine_;

    // Command bar
    std::unique_ptr<SurgeBox::CommandBar> commandBar_;
    bool stepRecordEnabled_{false};

    // Instrument editor in scrollable viewport
    std::unique_ptr<juce::Viewport> instrumentViewport_;
    std::unique_ptr<juce::Component> instrumentEditorWrapper_;
    std::unique_ptr<juce::AudioProcessorEditor> instrumentEditor_;
    int currentEditorVoice_{-1};

    // Piano roll in scrollable viewport with resizable divider
    std::unique_ptr<juce::Viewport> pianoRollViewport_;
    std::unique_ptr<SurgeBox::PianoRollWidget> pianoRoll_;
    std::unique_ptr<SurgeBox::PianoKeyboardWidget> pianoKeyboard_;
    int pianoRollHeight_{300};
    bool draggingDivider_{false};

    // Master FX editor (shown in instrument viewport when FX button is toggled)
    std::unique_ptr<SurgeBox::MasterFXEditor> masterFXEditor_;
    bool showingMasterFX_{false};
    bool showingChordTrack_{false};

    // Kernel editor (shown in instrument viewport when K button is toggled)
    std::unique_ptr<SurgeBox::KernelEditorPanel> kernelEditor_;
    bool showingKernelEditor_{false};

    void rebuildInstrumentEditor();
    void onVoiceChanged(int voice);
    void updateKeyboardListener();
    void updateInstrumentEditorScale();
    void showMasterFXEditor(bool show);
    void showChordTrackEditor(bool show);
    void showKernelEditor(bool show);

    SurgeBox::SurgeBoxLookAndFeel lookAndFeel_;

    // Mouse handling for divider
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    juce::Rectangle<int> getDividerBounds() const;
    juce::Rectangle<int> getCommandBarBounds() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SurgeBoxEditor)
};
