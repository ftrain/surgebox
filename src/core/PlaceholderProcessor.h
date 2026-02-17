/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GrooveboxProject.h"

namespace SurgeBox
{

/**
 * PlaceholderProcessor is a silent AudioProcessor used when an instrument
 * cannot be created (e.g., unknown type in a saved project, or creation failure).
 * It stores the original instrument type and name for potential reconnection.
 */
class PlaceholderProcessor : public juce::AudioProcessor
{
  public:
    PlaceholderProcessor(InstrumentType originalType = InstrumentType::Unknown,
                         const juce::String &originalName = "Unknown")
        : AudioProcessor(BusesProperties()
                             .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                             .withInput("Input", juce::AudioChannelSet::stereo(), true)),
          originalType_(originalType),
          originalName_(originalName)
    {
    }

    ~PlaceholderProcessor() override = default;

    InstrumentType getOriginalType() const { return originalType_; }
    const juce::String &getOriginalName() const { return originalName_; }

    // AudioProcessor interface
    const juce::String getName() const override
    {
        return "Placeholder (" + originalName_ + ")";
    }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) override
    {
        buffer.clear();
    }

    // Returns nullptr — editor checks for this to show placeholder message
    juce::AudioProcessorEditor *createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String &) override {}

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    void getStateInformation(juce::MemoryBlock &) override {}
    void setStateInformation(const void *, int) override {}

  private:
    InstrumentType originalType_;
    juce::String originalName_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaceholderProcessor)
};

} // namespace SurgeBox
