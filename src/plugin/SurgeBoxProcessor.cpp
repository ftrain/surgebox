/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SurgeBoxProcessor.h"
#include "SurgeBoxEditor.h"
#include "SurgeSynthesizer.h"

SurgeBoxProcessor::SurgeBoxProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Input", juce::AudioChannelSet::stereo(), true))
{
    // Create the Surge processor instances
    for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
    {
        surgeProcessors_[i] = std::make_unique<SurgeSynthProcessor>();
    }
}

SurgeBoxProcessor::~SurgeBoxProcessor()
{
    // Shutdown engine first - this clears all callbacks and synth pointers
    engine_.shutdown();

    // Release Surge processors - do this explicitly before member destruction
    // to ensure proper ordering
    for (auto &proc : surgeProcessors_)
    {
        if (proc)
        {
            // Don't call releaseResources here - it may have already been called
            // and could cause issues. Just reset the unique_ptr.
            proc.reset();
        }
    }
}

void SurgeBoxProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare all Surge processors and pass them to engine
    std::array<SurgeSynthProcessor *, SurgeBox::NUM_VOICES> procPtrs{};

    for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
    {
        if (surgeProcessors_[i])
        {
            surgeProcessors_[i]->prepareToPlay(sampleRate, samplesPerBlock);
            procPtrs[i] = surgeProcessors_[i].get();
        }
    }

    // Pass processor pointers to engine (it will use processBlock to handle GUI keyboard)
    engine_.setProcessors(procPtrs);
    engine_.initialize(sampleRate, samplesPerBlock);
}

void SurgeBoxProcessor::releaseResources()
{
    // Shutdown engine (clears callbacks and synth pointers)
    engine_.shutdown();

    // Release resources on each processor
    for (auto &proc : surgeProcessors_)
    {
        if (proc)
            proc->releaseResources();
    }
}

void SurgeBoxProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();

    // Handle incoming MIDI
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        // Route MIDI to active voice
        auto *synth = engine_.getActiveSynth();
        if (!synth)
            continue;

        if (msg.isNoteOn())
        {
            synth->playNote(msg.getChannel() - 1, msg.getNoteNumber(), msg.getVelocity(), 0, -1);
        }
        else if (msg.isNoteOff())
        {
            synth->releaseNote(msg.getChannel() - 1, msg.getNoteNumber(), msg.getVelocity());
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            synth->allNotesOff();
        }
        else if (msg.isPitchWheel())
        {
            synth->pitchBend(msg.getChannel() - 1, msg.getPitchWheelValue() - 8192);
        }
        else if (msg.isController())
        {
            synth->channelController(msg.getChannel() - 1, msg.getControllerNumber(),
                                     msg.getControllerValue());
        }
    }

    // Clear input (we're a synth)
    buffer.clear();

    // Process engine
    float *outputL = buffer.getWritePointer(0);
    float *outputR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outputL;

    engine_.process(outputL, outputR, numSamples);

    // Copy to mono if needed
    if (buffer.getNumChannels() == 1)
    {
        for (int i = 0; i < numSamples; i++)
            outputL[i] = (outputL[i] + outputR[i]) * 0.5f;
    }
}

juce::AudioProcessorEditor *SurgeBoxProcessor::createEditor()
{
    return new SurgeBoxEditor(*this);
}

SurgeSynthProcessor *SurgeBoxProcessor::getProcessor(int voice)
{
    if (voice < 0 || voice >= SurgeBox::NUM_VOICES)
        return nullptr;
    return surgeProcessors_[voice].get();
}

void SurgeBoxProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // Capture current state
    engine_.captureAllVoices();

    // Save project to temp file and read it
    auto tempPath = fs::temp_directory_path() / "surgebox_temp.sbox";
    if (engine_.getProject().saveToFile(tempPath))
    {
        std::ifstream file(tempPath, std::ios::binary | std::ios::ate);
        if (file)
        {
            auto size = file.tellg();
            file.seekg(0);

            destData.setSize(static_cast<size_t>(size));
            file.read(static_cast<char *>(destData.getData()), size);
        }
        fs::remove(tempPath);
    }
}

void SurgeBoxProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // Write to temp file and load
    auto tempPath = fs::temp_directory_path() / "surgebox_temp.sbox";

    std::ofstream file(tempPath, std::ios::binary);
    if (file)
    {
        file.write(static_cast<const char *>(data), sizeInBytes);
        file.close();

        if (engine_.getProject().loadFromFile(tempPath))
        {
            engine_.restoreAllVoices();
        }
        fs::remove(tempPath);
    }
}

// Plugin instantiation
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SurgeBoxProcessor(); }
