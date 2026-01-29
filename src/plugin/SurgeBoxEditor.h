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

class SurgeGUIEditor;

class SurgeBoxEditor : public juce::AudioProcessorEditor, public juce::Timer
{
  public:
    explicit SurgeBoxEditor(SurgeBoxProcessor &);
    ~SurgeBoxEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;
    void timerCallback() override;

  private:
    SurgeBoxProcessor &processor_;
    SurgeBox::SurgeBoxEngine &engine_;

    // UI Components
    std::unique_ptr<SurgeBox::VoiceSelector> voiceSelector_;
    std::unique_ptr<SurgeBox::TransportControls> transport_;
    std::unique_ptr<SurgeBox::PianoRollWidget> pianoRoll_;

    // Surge's main editor for the active voice
    std::unique_ptr<juce::Component> surgeEditor_;

    // Layout
    static constexpr int HEADER_HEIGHT = 40;
    static constexpr int PIANO_ROLL_HEIGHT = 150;

    void rebuildSurgeEditor();
    void onVoiceChanged(int voice);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SurgeBoxEditor)
};
