/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SurgeBoxProcessor.h"
#include "SurgeBoxEditor.h"
#include "SurgeSynthProcessor.h"
#include "core/TR808Processor.h"
#include "DexedFactory.h" // createDexedProcessor()

SurgeBoxProcessor::SurgeBoxProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Input", juce::AudioChannelSet::stereo(), true))
{
    // Create initial Surge XT processor instances for all voices
    for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
    {
        instrumentTypes_[i] = SurgeBox::InstrumentType::SurgeXT;
        processors_[i] = createInstrument(SurgeBox::InstrumentType::SurgeXT);
    }
}

SurgeBoxProcessor::~SurgeBoxProcessor()
{
    // Shutdown engine first - this clears all callbacks and processor pointers
    engine_.shutdown();

    // Release processors in REVERSE order to avoid issues with shared
    // global state. Surge uses shared lookandfeels and other resources that
    // may have dependencies on the first-created instance.
    for (int i = SurgeBox::NUM_VOICES - 1; i >= 0; i--)
    {
        if (processors_[i])
        {
            // Call releaseResources to ensure proper cleanup of audio resources
            processors_[i]->releaseResources();
            processors_[i].reset();
        }
    }
}

std::unique_ptr<juce::AudioProcessor> SurgeBoxProcessor::createInstrument(SurgeBox::InstrumentType type)
{
    switch (type)
    {
        case SurgeBox::InstrumentType::SurgeXT:
            return std::make_unique<SurgeSynthProcessor>();

        case SurgeBox::InstrumentType::TR808:
            return std::make_unique<SurgeBox::TR808Processor>();

        case SurgeBox::InstrumentType::Dexed:
            return createDexedProcessor();
    }
    return std::make_unique<SurgeSynthProcessor>();
}

void SurgeBoxProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate_ = sampleRate;
    currentBlockSize_ = samplesPerBlock;

    // Prepare all processors and pass them to engine
    std::array<juce::AudioProcessor *, SurgeBox::NUM_VOICES> procPtrs{};

    for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
    {
        if (processors_[i])
        {
            processors_[i]->prepareToPlay(sampleRate, samplesPerBlock);
            procPtrs[i] = processors_[i].get();
        }
    }

    // Pass processor pointers to engine
    engine_.setProcessors(procPtrs, instrumentTypes_);
    engine_.initialize(sampleRate, samplesPerBlock);
}

void SurgeBoxProcessor::releaseResources()
{
    // Shutdown engine (clears callbacks and processor pointers)
    engine_.shutdown();

    // Release resources on each processor
    for (auto &proc : processors_)
    {
        if (proc)
            proc->releaseResources();
    }
}

void SurgeBoxProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    int numSamples = buffer.getNumSamples();

    // Handle incoming MIDI - route to active voice via lock-free queue
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            engine_.sendNoteToActiveVoice(msg.getNoteNumber(), msg.getVelocity(), true);
        }
        else if (msg.isNoteOff())
        {
            engine_.sendNoteToActiveVoice(msg.getNoteNumber(), 0, false);
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            // Send note-off for all 128 notes is overkill;
            // for now just silence the active voice
            for (int n = 0; n < 128; n++)
                engine_.sendNoteToActiveVoice(n, 0, false);
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

juce::AudioProcessor *SurgeBoxProcessor::getProcessor(int voice)
{
    if (voice < 0 || voice >= SurgeBox::NUM_VOICES)
        return nullptr;
    return processors_[voice].get();
}

SurgeSynthProcessor *SurgeBoxProcessor::getSurgeProcessor(int voice)
{
    if (voice < 0 || voice >= SurgeBox::NUM_VOICES)
        return nullptr;
    if (instrumentTypes_[voice] != SurgeBox::InstrumentType::SurgeXT)
        return nullptr;
    return dynamic_cast<SurgeSynthProcessor *>(processors_[voice].get());
}

void SurgeBoxProcessor::switchInstrument(int voice, SurgeBox::InstrumentType newType)
{
    if (voice < 0 || voice >= SurgeBox::NUM_VOICES)
        return;
    if (instrumentTypes_[voice] == newType)
        return;

    // Mark voice as not ready (audio thread will skip it)
    engine_.setVoiceReady(voice, false);

    // Create new processor on message thread
    auto newProc = createInstrument(newType);
    if (newProc)
        newProc->prepareToPlay(currentSampleRate_, currentBlockSize_);

    // Swap processor
    auto oldProc = std::move(processors_[voice]);
    processors_[voice] = std::move(newProc);
    instrumentTypes_[voice] = newType;

    // Update engine's pointer
    std::array<juce::AudioProcessor *, SurgeBox::NUM_VOICES> procPtrs{};
    for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
        procPtrs[i] = processors_[i].get();
    engine_.setProcessors(procPtrs, instrumentTypes_);

    // Mark voice as ready again
    engine_.setVoiceReady(voice, true);

    // Cleanup old processor (safe now since audio thread won't use it)
    if (oldProc)
        oldProc->releaseResources();
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
            // Restore instrument types and recreate processors if needed
            for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
            {
                auto loadedType = engine_.getProject().voices[i].instrumentType;
                if (loadedType != instrumentTypes_[i])
                    switchInstrument(i, loadedType);
            }

            engine_.restoreAllVoices();
        }
        fs::remove(tempPath);
    }
}

// Plugin instantiation
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SurgeBoxProcessor(); }
