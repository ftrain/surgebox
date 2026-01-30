/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SurgeBoxEngine.h"
#include "SurgeSynthProcessor.h"
#include "globals.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <algorithm>
#include <cstring>

namespace SurgeBox
{

// ============================================================================
// SequencerEngine
// ============================================================================

SequencerEngine::SequencerEngine() = default;

void SequencerEngine::setProject(GrooveboxProject *project) { project_ = project; }

void SequencerEngine::setSynths(std::array<SurgeSynthesizer *, NUM_VOICES> synths)
{
    synths_ = synths;
}

void SequencerEngine::play()
{
    // If already playing, don't reset position
    if (playing_.load())
        return;

    // Start from current position (which should be 0 after stop/rewind)
    playing_.store(true);
}

void SequencerEngine::stop()
{
    playing_.store(false);

    // Release all active notes
    for (const auto &active : activeNotes_)
    {
        if (synths_[active.voiceIndex])
            synths_[active.voiceIndex]->releaseNote(0, active.pitch, 0);
    }
    activeNotes_.clear();

    // Reset to beginning when stopped
    currentBeat_.store(0.0);
}

void SequencerEngine::setPlaying(bool playing)
{
    if (playing)
        play();
    else
        stop();
}

void SequencerEngine::setPositionBeats(double beat)
{
    // Release all active notes when jumping
    for (const auto &active : activeNotes_)
    {
        if (synths_[active.voiceIndex])
            synths_[active.voiceIndex]->releaseNote(0, active.pitch, 0);
    }
    activeNotes_.clear();
    currentBeat_.store(beat);
}

double SequencerEngine::getLoopEndBeat() const
{
    if (!project_)
        return 4.0;

    // Loop length is the maximum of all pattern lengths
    double maxLength = 4.0;
    for (const auto &voice : project_->voices)
    {
        double patternLength = voice.pattern.bars * 4.0;
        if (patternLength > maxLength)
            maxLength = patternLength;
    }
    return maxLength;
}

std::vector<uint8_t> SequencerEngine::getPlayingNotes(int voiceIndex) const
{
    std::vector<uint8_t> notes;
    for (const auto &active : activeNotes_)
    {
        if (active.voiceIndex == voiceIndex)
            notes.push_back(active.pitch);
    }
    return notes;
}

void SequencerEngine::process(int numSamples, double sampleRate,
                              std::array<juce::MidiBuffer *, NUM_VOICES> midiBuffers)
{
    if (!playing_.load() || !project_)
        return;

    sampleRate_ = sampleRate;
    numSamplesInBlock_ = numSamples;

    double beatsPerSecond = project_->tempo / 60.0;
    beatsPerSample_ = beatsPerSecond / sampleRate_;
    double beatsThisBlock = beatsPerSample_ * numSamples;

    double startBeat = currentBeat_.load();
    blockStartBeat_ = startBeat;
    double endBeat = startBeat + beatsThisBlock;
    double loopEnd = getLoopEndBeat();

    if (endBeat >= loopEnd)
    {
        // Before loop point - starts at sample 0
        triggerNotesInRange(startBeat, loopEnd, numSamples, midiBuffers, 0);
        releaseNotesEndingInRange(startBeat, loopEnd, numSamples, midiBuffers, 0);

        // Calculate sample offset at loop point
        double beatsBeforeLoop = loopEnd - startBeat;
        int loopSampleOffset = static_cast<int>(beatsBeforeLoop / beatsPerSample_);

        // Wrap - notes after loop start at loopSampleOffset
        double remainder = endBeat - loopEnd;
        for (auto &active : activeNotes_)
            active.endBeat -= loopEnd;

        triggerNotesInRange(0.0, remainder, numSamples, midiBuffers, loopSampleOffset);
        releaseNotesEndingInRange(0.0, remainder, numSamples, midiBuffers, loopSampleOffset);
        currentBeat_.store(remainder);
    }
    else
    {
        triggerNotesInRange(startBeat, endBeat, numSamples, midiBuffers, 0);
        releaseNotesEndingInRange(startBeat, endBeat, numSamples, midiBuffers, 0);
        currentBeat_.store(endBeat);
    }
}

void SequencerEngine::triggerNotesInRange(double startBeat, double endBeat, int numSamples,
                                          std::array<juce::MidiBuffer *, NUM_VOICES> midiBuffers,
                                          int baseSampleOffset)
{
    if (!project_)
        return;

    // Check for solo
    bool anySolo = false;
    for (const auto &v : project_->voices)
    {
        if (v.solo)
        {
            anySolo = true;
            break;
        }
    }

    double blockLength = endBeat - startBeat;

    for (int v = 0; v < NUM_VOICES; v++)
    {
        if (!midiBuffers[v])
            continue;

        const auto &voice = project_->voices[v];

        if (anySolo && !voice.solo)
            continue;
        if (!anySolo && voice.mute)
            continue;

        double patternLength = voice.pattern.bars * 4.0;
        if (patternLength <= 0)
            continue;

        // Wrap the global position to pattern-local position
        double wrappedStart = std::fmod(startBeat, patternLength);
        double wrappedEnd = wrappedStart + blockLength;

        // Helper to add note-on with sample-accurate timing
        auto addNoteOn = [&](const MIDINote *note, double noteOffsetBeats) {
            int samplePos = baseSampleOffset + static_cast<int>(noteOffsetBeats / beatsPerSample_);
            samplePos = std::clamp(samplePos, 0, numSamples - 1);
            midiBuffers[v]->addEvent(
                juce::MidiMessage::noteOn(1, note->pitch, (juce::uint8)note->velocity),
                samplePos);
            double noteEndGlobal = startBeat + noteOffsetBeats + note->duration;
            activeNotes_.push_back({v, note->pitch, noteEndGlobal});
        };

        // If this block would cross the pattern boundary, handle in two parts
        if (wrappedEnd > patternLength)
        {
            // Part 1: from wrappedStart to patternLength
            auto notes1 = voice.pattern.getNotesStartingInRange(wrappedStart, patternLength);
            for (const auto *note : notes1)
            {
                double noteOffsetBeats = note->startBeat - wrappedStart;
                addNoteOn(note, noteOffsetBeats);
            }

            // Part 2: from 0 to remainder
            double remainder = wrappedEnd - patternLength;
            auto notes2 = voice.pattern.getNotesStartingInRange(0.0, remainder);
            for (const auto *note : notes2)
            {
                // Note starts at (patternLength - wrappedStart) beats into this block
                double noteOffsetBeats = (patternLength - wrappedStart) + note->startBeat;
                addNoteOn(note, noteOffsetBeats);
            }
        }
        else
        {
            // Simple case: block doesn't cross pattern boundary
            auto notesToTrigger = voice.pattern.getNotesStartingInRange(wrappedStart, wrappedEnd);
            for (const auto *note : notesToTrigger)
            {
                double noteOffsetBeats = note->startBeat - wrappedStart;
                addNoteOn(note, noteOffsetBeats);
            }
        }
    }
}

void SequencerEngine::releaseNotesEndingInRange(double startBeat, double endBeat, int numSamples,
                                                std::array<juce::MidiBuffer *, NUM_VOICES> midiBuffers,
                                                int baseSampleOffset)
{
    auto it = activeNotes_.begin();
    while (it != activeNotes_.end())
    {
        if (it->endBeat >= startBeat && it->endBeat < endBeat)
        {
            if (midiBuffers[it->voiceIndex])
            {
                // Calculate sample position for note-off
                double noteOffsetBeats = it->endBeat - startBeat;
                int samplePos = baseSampleOffset + static_cast<int>(noteOffsetBeats / beatsPerSample_);
                samplePos = std::clamp(samplePos, 0, numSamples - 1);
                midiBuffers[it->voiceIndex]->addEvent(
                    juce::MidiMessage::noteOff(1, it->pitch),
                    samplePos);
            }
            it = activeNotes_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// ============================================================================
// SurgeBoxEngine
// ============================================================================

SurgeBoxEngine::SurgeBoxEngine()
{
    // Create pattern models for each voice with auto-sync to project patterns
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        patternModels_[i] = std::make_unique<PatternModel>(&undoManager_);
        patternModels_[i]->setAutoSyncPattern(&project_.voices[i].pattern);
    }
}

SurgeBoxEngine::~SurgeBoxEngine() { shutdown(); }

void SurgeBoxEngine::setProcessors(std::array<SurgeSynthProcessor *, NUM_VOICES> processors)
{
    processors_ = processors;

    // Also set up synth pointers for the sequencer
    std::array<SurgeSynthesizer *, NUM_VOICES> synthPtrs{};
    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (processors_[i] && processors_[i]->surge)
            synthPtrs[i] = processors_[i]->surge.get();
    }
    sequencer_.setSynths(synthPtrs);
}

bool SurgeBoxEngine::initialize(double sampleRate, int blockSize)
{
    if (initialized_)
        return true;

    sampleRate_ = sampleRate;
    blockSize_ = blockSize;

    // Pre-allocate voice processing buffer (avoid allocations in audio thread)
    voiceBuffer_ = std::make_unique<juce::AudioBuffer<float>>(2, BLOCK_SIZE);

    // Set up sequencer with project
    sequencer_.setProject(&project_);

    // Initialize project with voice names
    project_.reset();
    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (processors_[i] && processors_[i]->surge)
        {
            project_.voices[i].name = processors_[i]->surge->storage.getPatch().name;
            if (project_.voices[i].name.empty())
                project_.voices[i].name = "Init";
        }
    }

    // Sync pattern models from project
    syncPatternModelsFromProject();

    initialized_ = true;
    return true;
}

void SurgeBoxEngine::shutdown()
{
    if (!initialized_)
        return;

    // Clear callbacks first to prevent any access during shutdown
    onVoiceChanged = nullptr;
    onPlayheadMoved = nullptr;

    // Clear pattern model callbacks
    for (auto &model : patternModels_)
    {
        if (model)
            model->onPatternChanged = nullptr;
    }

    sequencer_.stop();

    // Clear sequencer's synth pointers before we lose access to processors
    sequencer_.setSynths({});

    // Note: We don't own the processors, plugin layer does
    for (auto &proc : processors_)
    {
        if (proc && proc->surge)
            proc->surge->allNotesOff();
        proc = nullptr;
    }

    initialized_ = false;
}

void SurgeBoxEngine::process(float *outputL, float *outputR, int numSamples)
{
    if (!initialized_)
    {
        memset(outputL, 0, numSamples * sizeof(float));
        memset(outputR, 0, numSamples * sizeof(float));
        return;
    }

    // Get position BEFORE advancing (this is where audio for this block starts)
    double blockStartBeat = sequencer_.getPositionBeats();

    // Store block start for mixVoices to sync Surge
    blockStartBeat_ = blockStartBeat;

    // Clear MIDI buffers for this block
    for (auto &buf : voiceMidiBuffers_)
        buf.clear();

    // Build array of pointers for sequencer
    std::array<juce::MidiBuffer *, NUM_VOICES> midiBufferPtrs;
    for (int i = 0; i < NUM_VOICES; ++i)
        midiBufferPtrs[i] = &voiceMidiBuffers_[i];

    // Advance sequencer - populates MIDI buffers with sample-accurate events
    sequencer_.process(numSamples, sampleRate_, midiBufferPtrs);

    // Clear output
    memset(outputL, 0, numSamples * sizeof(float));
    memset(outputR, 0, numSamples * sizeof(float));

    // Process each voice and mix
    mixVoices(outputL, outputR, numSamples);

    // Apply master volume
    float mv = project_.masterVolume;
    for (int i = 0; i < numSamples; i++)
    {
        outputL[i] *= mv;
        outputR[i] *= mv;
    }

    // Notify playhead position (for UI)
    if (onPlayheadMoved && sequencer_.isPlaying())
        onPlayheadMoved(sequencer_.getPositionBeats());
}

void SurgeBoxEngine::mixVoices(float *outputL, float *outputR, int numSamples)
{
    // Check for solo
    bool anySolo = false;
    for (const auto &v : project_.voices)
    {
        if (v.solo)
        {
            anySolo = true;
            break;
        }
    }

    // Resize buffer to match incoming size
    voiceBuffer_->setSize(2, numSamples, false, false, true);

    for (int v = 0; v < NUM_VOICES; v++)
    {
        if (!processors_[v] || !processors_[v]->surge)
            continue;

        const auto &voice = project_.voices[v];

        // Skip muted voices
        if (anySolo && !voice.solo)
            continue;
        if (!anySolo && voice.mute)
            continue;

        // Sync Surge's internal time with our sequencer ONCE at block start
        auto *synth = processors_[v]->surge.get();
        synth->time_data.tempo = project_.tempo;
        synth->time_data.ppqPos = blockStartBeat_;
        synth->time_data.timeSigNumerator = 4;
        synth->time_data.timeSigDenominator = 4;
        synth->resetStateFromTimeData();

        // Clear and process with MIDI events from sequencer
        voiceBuffer_->clear();
        processors_[v]->processBlock(*voiceBuffer_, voiceMidiBuffers_[v]);

        // Get output and mix with volume/pan
        float vol = voice.volume;
        float panL = std::min(1.0f, 1.0f - voice.pan);
        float panR = std::min(1.0f, 1.0f + voice.pan);

        const float *procL = voiceBuffer_->getReadPointer(0);
        const float *procR = voiceBuffer_->getReadPointer(1);

        for (int i = 0; i < numSamples; i++)
        {
            outputL[i] += procL[i] * vol * panL;
            outputR[i] += procR[i] * vol * panR;
        }
    }
}

void SurgeBoxEngine::setActiveVoice(int voice)
{
    if (voice < 0 || voice >= NUM_VOICES)
        return;

    activeVoice_ = voice;

    if (onVoiceChanged)
        onVoiceChanged(voice);
}

SurgeSynthesizer *SurgeBoxEngine::getSynth(int voice)
{
    if (voice < 0 || voice >= NUM_VOICES)
        return nullptr;
    if (!processors_[voice])
        return nullptr;
    return processors_[voice]->surge.get();
}

void SurgeBoxEngine::captureAllVoices()
{
    for (int i = 0; i < NUM_VOICES; i++)
    {
        auto *synth = getSynth(i);
        if (synth)
            project_.voices[i].captureFromSynth(synth);
    }
}

void SurgeBoxEngine::restoreAllVoices()
{
    for (int i = 0; i < NUM_VOICES; i++)
    {
        auto *synth = getSynth(i);
        if (synth)
            project_.voices[i].restoreToSynth(synth);
    }
}

PatternModel *SurgeBoxEngine::getPatternModel(int voice)
{
    if (voice < 0 || voice >= NUM_VOICES)
        return nullptr;
    return patternModels_[voice].get();
}

void SurgeBoxEngine::syncPatternModelsFromProject()
{
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        if (patternModels_[i])
            patternModels_[i]->loadFromPattern(project_.voices[i].pattern);
    }
}

void SurgeBoxEngine::syncPatternModelsToProject()
{
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        if (patternModels_[i])
            patternModels_[i]->saveToPattern(project_.voices[i].pattern);
    }
}

} // namespace SurgeBox
