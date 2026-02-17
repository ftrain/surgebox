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
    // Defer note-off to audio thread via pendingStop flag
    pendingStop_.store(true);
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
    // Defer position jump to audio thread (note-offs handled there)
    pendingPositionJump_.store(beat);
}

double SequencerEngine::getLoopEndBeat() const
{
    if (!project_)
        return 4.0;

    // Loop length must account for tempo multipliers
    // Each voice needs (patternBeats / tempoMultiplier) global beats to complete
    double maxLength = 4.0;
    for (const auto &voice : project_->voices)
    {
        double patternLength = voice.pattern.bars * 4.0;
        double multiplier = voice.tempoMultiplier.load();
        if (multiplier <= 0) multiplier = 1.0;
        double globalBeatsNeeded = patternLength / multiplier;
        if (globalBeatsNeeded > maxLength)
            maxLength = globalBeatsNeeded;
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
    // Handle pending stop (inject note-offs for all active notes)
    if (pendingStop_.load())
    {
        for (const auto &active : activeNotes_)
        {
            if (midiBuffers[active.voiceIndex])
                midiBuffers[active.voiceIndex]->addEvent(
                    juce::MidiMessage::noteOff(1, active.pitch), 0);
        }
        activeNotes_.clear();
        playing_.store(false);
        pendingStop_.store(false);
        currentBeat_.store(0.0);
        return;
    }

    // Handle pending position jump (inject note-offs, then jump)
    double jumpTo = pendingPositionJump_.exchange(-1.0);
    if (jumpTo >= 0.0)
    {
        for (const auto &active : activeNotes_)
        {
            if (midiBuffers[active.voiceIndex])
                midiBuffers[active.voiceIndex]->addEvent(
                    juce::MidiMessage::noteOff(1, active.pitch), 0);
        }
        activeNotes_.clear();
        currentBeat_.store(jumpTo);
    }

    if (!playing_.load() || !project_)
        return;

    sampleRate_ = sampleRate;
    numSamplesInBlock_ = numSamples;

    // Master clock runs at base tempo
    double beatsPerSecond = project_->tempo / 60.0;
    beatsPerSample_ = beatsPerSecond / sampleRate_;
    double beatsThisBlock = beatsPerSample_ * numSamples;

    double startBeat = currentBeat_.load();
    blockStartBeat_ = startBeat;
    double endBeat = startBeat + beatsThisBlock;
    double loopEnd = getLoopEndBeat();

    // Check for bar boundaries and apply pending tempo changes
    for (auto &voice : project_->voices)
    {
        double pending = voice.pendingTempoMultiplier.load();
        double current = voice.tempoMultiplier.load();
        if (pending != current)
        {
            // Check if we crossed a bar boundary (every 4 beats in voice-time)
            double voiceStartBeat = startBeat * current;
            double voiceEndBeat = endBeat * current;
            int startBar = static_cast<int>(voiceStartBeat / 4.0);
            int endBar = static_cast<int>(voiceEndBeat / 4.0);

            if (endBar > startBar || voiceStartBeat == 0.0)
            {
                // Crossed a bar boundary - apply pending change
                voice.tempoMultiplier.store(pending);
            }
        }
    }

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

        // Scale global positions by voice's tempo multiplier
        double multiplier = voice.tempoMultiplier.load();
        if (multiplier <= 0) multiplier = 1.0;
        double voiceStartBeat = startBeat * multiplier;
        double voiceBlockLength = blockLength * multiplier;

        // Wrap the voice-scaled position to pattern-local position
        double wrappedStart = std::fmod(voiceStartBeat, patternLength);
        double wrappedEnd = wrappedStart + voiceBlockLength;

        // Helper to add note-on with sample-accurate timing
        // noteOffsetBeats is in voice-time, need to convert to global-time for sample position
        auto addNoteOn = [&](const MIDINote *note, double noteOffsetBeatsVoiceTime) {
            // Convert voice-time offset to global-time
            double noteOffsetGlobal = multiplier > 0
                ? noteOffsetBeatsVoiceTime / multiplier
                : noteOffsetBeatsVoiceTime;
            int samplePos = baseSampleOffset + static_cast<int>(noteOffsetGlobal / beatsPerSample_);
            samplePos = std::clamp(samplePos, 0, numSamples - 1);
            midiBuffers[v]->addEvent(
                juce::MidiMessage::noteOn(1, note->pitch, (juce::uint8)note->velocity),
                samplePos);
            // Note duration is also in voice-time, convert to global-time for end tracking
            double durationGlobal = multiplier > 0
                ? note->duration / multiplier
                : note->duration;
            double noteEndGlobal = startBeat + noteOffsetGlobal + durationGlobal;
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
    : projectModel_(&undoManager_)
{
    // Initialize voiceReady flags
    for (auto &ready : voiceReady_)
        ready.store(false);

    // Create pattern models for each voice with auto-sync to project patterns
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        patternModels_[i] = std::make_unique<PatternModel>(&undoManager_);
        patternModels_[i]->setAutoSyncPattern(&project_.voices[i].pattern);
    }

    // Connect project model to project for auto-sync
    projectModel_.setProject(&project_);

    // Connect MIDI mapping engine to project for serialization
    project_.midiMappingEngine = &midiMappingEngine_;
}

SurgeBoxEngine::~SurgeBoxEngine() { shutdown(); }

void SurgeBoxEngine::setProcessors(std::array<juce::AudioProcessor *, NUM_VOICES> processors,
                                    std::array<InstrumentType, NUM_VOICES> types)
{
    processors_ = processors;
    instrumentTypes_ = types;

    for (int i = 0; i < NUM_VOICES; i++)
    {
        voiceReady_[i].store(processors_[i] != nullptr);
        project_.voices[i].instrumentType = types[i];
    }
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
        project_.voices[i].instrumentType = instrumentTypes_[i];

        // For Surge XT, try to get the patch name
        if (instrumentTypes_[i] == InstrumentType::SurgeXT)
        {
            auto *surgeProc = dynamic_cast<SurgeSynthProcessor *>(processors_[i]);
            if (surgeProc && surgeProc->surge)
            {
                project_.voices[i].name = surgeProc->surge->storage.getPatch().name;
                if (project_.voices[i].name.empty())
                    project_.voices[i].name = "Init";
            }
        }
        else if (instrumentTypes_[i] == InstrumentType::TR808)
        {
            project_.voices[i].name = "TR-808";
        }
        else if (instrumentTypes_[i] == InstrumentType::Dexed)
        {
            project_.voices[i].name = "Dexed";
        }
    }

    // Sync pattern models from project
    syncPatternModelsFromProject();

    // Initialize master FX chain using SurgeStorage from first Surge XT voice
    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (instrumentTypes_[i] == InstrumentType::SurgeXT)
        {
            auto *surgeProc = dynamic_cast<SurgeSynthProcessor *>(processors_[i]);
            if (surgeProc && surgeProc->surge)
            {
                masterFXChain_.initialize(&surgeProc->surge->storage, sampleRate);
                masterFXChain_.loadFromProject(project_);
                break;
            }
        }
    }

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
    masterFXChain_.shutdown();

    // Note: We don't own the processors, plugin layer does
    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (processors_[i])
        {
            // For Surge XT, send all-notes-off
            if (instrumentTypes_[i] == InstrumentType::SurgeXT)
            {
                auto *surgeProc = dynamic_cast<SurgeSynthProcessor *>(processors_[i]);
                if (surgeProc && surgeProc->surge)
                    surgeProc->surge->allNotesOff();
            }
        }
        processors_[i] = nullptr;
        voiceReady_[i].store(false);
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

    // Drain any pending note events from UI thread into MIDI buffers
    drainPendingNotes();

    // Build array of pointers for sequencer
    std::array<juce::MidiBuffer *, NUM_VOICES> midiBufferPtrs;
    for (int i = 0; i < NUM_VOICES; ++i)
        midiBufferPtrs[i] = &voiceMidiBuffers_[i];

    // Advance sequencer - populates MIDI buffers with sample-accurate events
    sequencer_.process(numSamples, sampleRate_, midiBufferPtrs);

    // Clear output and aux buffers
    memset(outputL, 0, numSamples * sizeof(float));
    memset(outputR, 0, numSamples * sizeof(float));
    memset(auxSendAL_, 0, numSamples * sizeof(float));
    memset(auxSendAR_, 0, numSamples * sizeof(float));
    memset(auxSendBL_, 0, numSamples * sizeof(float));
    memset(auxSendBR_, 0, numSamples * sizeof(float));

    // Process each voice and mix (also populates aux send buffers)
    mixVoices(outputL, outputR, numSamples);

    // Process send FX: slot 0 = Send A, slot 1 = Send B
    masterFXChain_.processSlot(0, auxSendAL_, auxSendAR_, numSamples);
    masterFXChain_.processSlot(1, auxSendBL_, auxSendBR_, numSamples);

    // Sum aux returns into main bus
    for (int i = 0; i < numSamples; i++)
    {
        outputL[i] += auxSendAL_[i] + auxSendBL_[i];
        outputR[i] += auxSendAR_[i] + auxSendBR_[i];
    }

    // Process insert FX: slots 2 and 3 on main bus
    masterFXChain_.processSlot(2, outputL, outputR, numSamples);
    masterFXChain_.processSlot(3, outputL, outputR, numSamples);

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

void SurgeBoxEngine::drainPendingNotes()
{
    int readPos = pendingNoteReadPos_.load(std::memory_order_acquire);
    int writePos = pendingNoteWritePos_.load(std::memory_order_acquire);

    while (readPos != writePos)
    {
        const auto &evt = pendingNotes_[readPos % MAX_PENDING_NOTES];
        int voiceIdx = evt.voiceIndex;

        if (voiceIdx >= 0 && voiceIdx < NUM_VOICES)
        {
            if (evt.noteOn)
            {
                voiceMidiBuffers_[voiceIdx].addEvent(
                    juce::MidiMessage::noteOn(1, evt.pitch, evt.velocity), 0);
            }
            else
            {
                voiceMidiBuffers_[voiceIdx].addEvent(
                    juce::MidiMessage::noteOff(1, evt.pitch), 0);
            }
        }

        readPos++;
        pendingNoteReadPos_.store(readPos, std::memory_order_release);
    }
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
        if (!processors_[v] || !voiceReady_[v].load(std::memory_order_acquire))
            continue;

        const auto &voice = project_.voices[v];

        // Skip muted voices
        if (anySolo && !voice.solo)
            continue;
        if (!anySolo && voice.mute)
            continue;

        // Sync Surge XT's internal time with our sequencer ONCE at block start
        if (instrumentTypes_[v] == InstrumentType::SurgeXT)
        {
            auto *surgeProc = dynamic_cast<SurgeSynthProcessor *>(processors_[v]);
            if (surgeProc && surgeProc->surge)
            {
                auto *synth = surgeProc->surge.get();
                double multiplier = voice.tempoMultiplier.load();
                if (multiplier <= 0) multiplier = 1.0;
                synth->time_data.tempo = project_.tempo * multiplier;
                synth->time_data.ppqPos = blockStartBeat_;
                synth->time_data.timeSigNumerator = 4;
                synth->time_data.timeSigDenominator = 4;
                synth->resetStateFromTimeData();
            }
        }

        // Clear and process with MIDI events from sequencer
        voiceBuffer_->clear();
        processors_[v]->processBlock(*voiceBuffer_, voiceMidiBuffers_[v]);

        // Get output and mix with volume/pan
        float vol = voice.volume;
        float panL = std::min(1.0f, 1.0f - voice.pan);
        float panR = std::min(1.0f, 1.0f + voice.pan);
        float sendA = voice.sendA;
        float sendB = voice.sendB;

        const float *procL = voiceBuffer_->getReadPointer(0);
        const float *procR = voiceBuffer_->getReadPointer(1);

        for (int i = 0; i < numSamples; i++)
        {
            float sampleL = procL[i] * vol * panL;
            float sampleR = procR[i] * vol * panR;

            outputL[i] += sampleL;
            outputR[i] += sampleR;

            // Route to aux send buffers (pre-fader sends using dry voice signal)
            if (sendA > 0.0f)
            {
                auxSendAL_[i] += procL[i] * sendA;
                auxSendAR_[i] += procR[i] * sendA;
            }
            if (sendB > 0.0f)
            {
                auxSendBL_[i] += procL[i] * sendB;
                auxSendBR_[i] += procR[i] * sendB;
            }
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

InstrumentType SurgeBoxEngine::getInstrumentType(int voice) const
{
    if (voice < 0 || voice >= NUM_VOICES)
        return InstrumentType::SurgeXT;
    return instrumentTypes_[voice];
}

juce::AudioProcessor *SurgeBoxEngine::getProcessor(int voice)
{
    if (voice < 0 || voice >= NUM_VOICES)
        return nullptr;
    return processors_[voice];
}

void SurgeBoxEngine::sendNoteToActiveVoice(int pitch, int velocity, bool noteOn)
{
    int writePos = pendingNoteWritePos_.load(std::memory_order_acquire);
    int readPos = pendingNoteReadPos_.load(std::memory_order_acquire);

    // Check if queue is full
    if (writePos - readPos >= MAX_PENDING_NOTES)
        return;

    auto &evt = pendingNotes_[writePos % MAX_PENDING_NOTES];
    evt.voiceIndex = activeVoice_;
    evt.pitch = static_cast<uint8_t>(pitch);
    evt.velocity = static_cast<uint8_t>(velocity);
    evt.noteOn = noteOn;

    pendingNoteWritePos_.store(writePos + 1, std::memory_order_release);
}

void SurgeBoxEngine::captureAllVoices()
{
    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (processors_[i])
            project_.voices[i].captureFromProcessor(processors_[i]);
    }
    masterFXChain_.saveToProject(project_);
}

void SurgeBoxEngine::restoreAllVoices()
{
    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (processors_[i])
            project_.voices[i].restoreToProcessor(processors_[i]);
    }
    masterFXChain_.loadFromProject(project_);
    projectModel_.syncFromProject();
}

void SurgeBoxEngine::setVoiceReady(int voice, bool ready)
{
    if (voice >= 0 && voice < NUM_VOICES)
        voiceReady_[voice].store(ready, std::memory_order_release);
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
    projectModel_.syncFromProject();
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
