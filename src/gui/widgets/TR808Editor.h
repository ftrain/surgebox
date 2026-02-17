/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * TR-808 Editor - Grid of drum pads with per-voice parameter knobs.
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace SurgeBox
{

class TR808Processor;

class TR808Editor : public juce::AudioProcessorEditor
{
  public:
    explicit TR808Editor(TR808Processor &processor);
    ~TR808Editor() override = default;

    void paint(juce::Graphics &g) override;
    void resized() override;

  private:
    TR808Processor &tr808_;

    struct DrumPad
    {
        std::unique_ptr<juce::TextButton> button;
        std::unique_ptr<juce::Slider> pitchKnob;
        std::unique_ptr<juce::Slider> decayKnob;
        std::unique_ptr<juce::Slider> toneKnob;
        std::unique_ptr<juce::Slider> driveKnob;
        std::unique_ptr<juce::Label> nameLabel;
        std::unique_ptr<juce::Label> pitchLabel;
        std::unique_ptr<juce::Label> decayLabel;
        std::unique_ptr<juce::Label> toneLabel;
        std::unique_ptr<juce::Label> driveLabel;
    };

    std::array<DrumPad, 13> pads_;

    void setupPad(int index, const juce::String &name);
    void triggerDrum(int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TR808Editor)
};

} // namespace SurgeBox
