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
#include "gui/widgets/PianoRollWidget.h"
#include "gui/widgets/VoiceSelector.h"
#include "gui/widgets/TransportControls.h"
#include "gui/SurgeBoxLookAndFeel.h"

class SurgeBoxEditor : public juce::AudioProcessorEditor,
                       public juce::Timer,
                       public juce::Button::Listener,
                       public juce::ComboBox::Listener,
                       public juce::Slider::Listener,
                       public juce::MidiKeyboardState::Listener
{
  public:
    explicit SurgeBoxEditor(SurgeBoxProcessor &);
    ~SurgeBoxEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;
    void timerCallback() override;
    void buttonClicked(juce::Button *button) override;
    void comboBoxChanged(juce::ComboBox *comboBox) override;
    void sliderValueChanged(juce::Slider *slider) override;
    bool keyPressed(const juce::KeyPress &key) override;

    // MidiKeyboardState::Listener - captures notes for step recording
    void handleNoteOn(juce::MidiKeyboardState *, int midiChannel, int midiNoteNumber,
                      float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState *, int midiChannel, int midiNoteNumber,
                       float velocity) override;

  private:
    SurgeBoxProcessor &processor_;
    SurgeBox::SurgeBoxEngine &engine_;

    // UI Components
    std::unique_ptr<SurgeBox::VoiceSelector> voiceSelector_;
    std::unique_ptr<SurgeBox::TransportControls> transport_;

    // Step record button
    std::unique_ptr<juce::TextButton> stepRecordButton_;
    bool stepRecordEnabled_{false};

    // Measure control buttons
    std::unique_ptr<juce::TextButton> measuresDoubleBtn_;
    std::unique_ptr<juce::TextButton> measuresHalfBtn_;
    std::unique_ptr<juce::TextButton> measuresAddBtn_;
    std::unique_ptr<juce::TextButton> measuresSubBtn_;
    std::unique_ptr<juce::Label> measuresLabel_;

    // Grid size dropdown
    std::unique_ptr<juce::ComboBox> gridSizeCombo_;
    std::unique_ptr<juce::Label> gridSizeLabel_;

    // Tempo control
    std::unique_ptr<juce::Slider> tempoSlider_;
    std::unique_ptr<juce::Label> tempoLabel_;

    // Surge editor in scrollable viewport
    std::unique_ptr<juce::Viewport> surgeViewport_;
    std::unique_ptr<juce::Component> surgeEditorWrapper_;
    std::unique_ptr<juce::AudioProcessorEditor> surgeEditor_;
    int currentSurgeVoice_{-1};

    // Piano roll in scrollable viewport with resizable divider
    std::unique_ptr<juce::Viewport> pianoRollViewport_;
    std::unique_ptr<SurgeBox::PianoRollWidget> pianoRoll_;
    int pianoRollHeight_{300};
    bool draggingDivider_{false};

    // Layout: Surge → Command bar → Piano Roll
    static constexpr int COMMAND_BAR_HEIGHT = 44;
    static constexpr int DIVIDER_HEIGHT = 6;
    static constexpr int MIN_PIANO_ROLL_HEIGHT = 150;
    static constexpr int MIN_SYNTH_HEIGHT = 200;

    void rebuildSurgeEditor();
    void onVoiceChanged(int voice);
    void updateKeyboardListener();
    void updateSurgeEditorScale();
    void updateMeasuresLabel();
    // SurgeBox look-and-feel
    SurgeBox::SurgeBoxLookAndFeel lookAndFeel_;

    // Measure operations
    void doubleMeasures();
    void halveMeasures();
    void addMeasure();
    void subtractMeasure();

    // Mouse handling for divider
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseMove(const juce::MouseEvent &e) override;
    juce::Rectangle<int> getDividerBounds() const;
    juce::Rectangle<int> getCommandBarBounds() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SurgeBoxEditor)
};
