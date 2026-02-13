/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "core/SurgeBoxEngine.h"
#include "core/GrooveboxProject.h"
#include <array>
#include <memory>

class SurgeSynthProcessor;

class SurgeBoxProcessor : public juce::AudioProcessor
{
  public:
    SurgeBoxProcessor();
    ~SurgeBoxProcessor() override;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String &) override {}

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    // SurgeBox access
    SurgeBox::SurgeBoxEngine &getEngine() { return engine_; }
    const SurgeBox::SurgeBoxEngine &getEngine() const { return engine_; }

    // Access to individual processors (for GUI editor creation)
    juce::AudioProcessor *getProcessor(int voice);

    // Get a Surge processor specifically (for Surge-specific UI features)
    SurgeSynthProcessor *getSurgeProcessor(int voice);

    // Instrument factory
    static std::unique_ptr<juce::AudioProcessor> createInstrument(SurgeBox::InstrumentType type);

    // Switch instrument type for a voice (thread-safe)
    void switchInstrument(int voice, SurgeBox::InstrumentType newType);

  private:
    // We own the processor instances
    std::array<std::unique_ptr<juce::AudioProcessor>, SurgeBox::NUM_VOICES> processors_;
    std::array<SurgeBox::InstrumentType, SurgeBox::NUM_VOICES> instrumentTypes_{};

    // Engine orchestrates the voices
    SurgeBox::SurgeBoxEngine engine_;

    // Current sample rate / block size for re-preparing new instruments
    double currentSampleRate_{44100.0};
    int currentBlockSize_{512};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SurgeBoxProcessor)
};
