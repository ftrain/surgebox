/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SurgeBoxEngine.h"
#include "SurgeSynthesizer.h"
#include "SurgeStorage.h"
#include "globals.h"

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

void SequencerEngine::play() { playing_.store(true); }

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
        return 16.0;
    return project_->loopBars * 4.0;
}

void SequencerEngine::process(int numSamples, double sampleRate)
{
    if (!playing_.load() || !project_)
        return;

    sampleRate_ = sampleRate;

    double beatsPerSecond = project_->tempo / 60.0;
    double beatsPerSample = beatsPerSecond / sampleRate_;
    double beatsThisBlock = beatsPerSample * numSamples;

    double startBeat = currentBeat_.load();
    double endBeat = startBeat + beatsThisBlock;
    double loopEnd = getLoopEndBeat();

    if (endBeat >= loopEnd)
    {
        // Before loop point
        triggerNotesInRange(startBeat, loopEnd);
        releaseNotesEndingInRange(startBeat, loopEnd);

        // Wrap
        double remainder = endBeat - loopEnd;
        for (auto &active : activeNotes_)
            active.endBeat -= loopEnd;

        triggerNotesInRange(0.0, remainder);
        releaseNotesEndingInRange(0.0, remainder);
        currentBeat_.store(remainder);
    }
    else
    {
        triggerNotesInRange(startBeat, endBeat);
        releaseNotesEndingInRange(startBeat, endBeat);
        currentBeat_.store(endBeat);
    }
}

void SequencerEngine::triggerNotesInRange(double startBeat, double endBeat)
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

    for (int v = 0; v < NUM_VOICES; v++)
    {
        if (!synths_[v])
            continue;

        const auto &voice = project_->voices[v];

        if (anySolo && !voice.solo)
            continue;
        if (!anySolo && voice.mute)
            continue;

        auto notesToTrigger = voice.pattern.getNotesStartingInRange(startBeat, endBeat);

        for (const auto *note : notesToTrigger)
        {
            synths_[v]->playNote(0, note->pitch, note->velocity, 0, -1);
            activeNotes_.push_back({v, note->pitch, note->endBeat()});
        }
    }
}

void SequencerEngine::releaseNotesEndingInRange(double startBeat, double endBeat)
{
    auto it = activeNotes_.begin();
    while (it != activeNotes_.end())
    {
        if (it->endBeat >= startBeat && it->endBeat < endBeat)
        {
            if (synths_[it->voiceIndex])
                synths_[it->voiceIndex]->releaseNote(0, it->pitch, 0);
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

SurgeBoxEngine::SurgeBoxEngine() = default;

SurgeBoxEngine::~SurgeBoxEngine() { shutdown(); }

bool SurgeBoxEngine::initialize(double sampleRate, int blockSize)
{
    if (initialized_)
        return true;

    sampleRate_ = sampleRate;
    blockSize_ = blockSize;

    // Create shared storage for wavetables, samples, etc.
    // Each synth will get its own SurgeStorage but we could optimize later

    // Create 4 Surge instances
    std::array<SurgeSynthesizer *, NUM_VOICES> synthPtrs{};

    for (int i = 0; i < NUM_VOICES; i++)
    {
        synths_[i] = std::make_unique<SurgeSynthesizer>(nullptr);

        // Initialize with sample rate
        synths_[i]->setSamplerate(static_cast<float>(sampleRate_));
        synths_[i]->audio_processing_active = true;

        // Synth is already initialized with default patch from constructor

        synthPtrs[i] = synths_[i].get();
    }

    // Set up sequencer
    sequencer_.setProject(&project_);
    sequencer_.setSynths(synthPtrs);

    // Initialize project with voice names
    project_.reset();
    for (int i = 0; i < NUM_VOICES; i++)
    {
        project_.voices[i].name = synths_[i]->storage.getPatch().name;
        if (project_.voices[i].name.empty())
            project_.voices[i].name = "Init";
    }

    initialized_ = true;
    return true;
}

void SurgeBoxEngine::shutdown()
{
    if (!initialized_)
        return;

    sequencer_.stop();

    for (auto &synth : synths_)
    {
        if (synth)
        {
            synth->allNotesOff();
            synth.reset();
        }
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

    // Advance sequencer (triggers notes)
    sequencer_.process(numSamples, sampleRate_);

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

    // Process in blocks matching Surge's block size
    int processed = 0;
    while (processed < numSamples)
    {
        int blockSamples = std::min(BLOCK_SIZE, numSamples - processed);

        for (int v = 0; v < NUM_VOICES; v++)
        {
            if (!synths_[v])
                continue;

            const auto &voice = project_.voices[v];

            // Skip muted voices
            if (anySolo && !voice.solo)
                continue;
            if (!anySolo && voice.mute)
                continue;

            // Process this synth
            synths_[v]->process();

            // Get output and mix with volume/pan
            float vol = voice.volume;
            float panL = std::min(1.0f, 1.0f - voice.pan);
            float panR = std::min(1.0f, 1.0f + voice.pan);

            for (int i = 0; i < blockSamples; i++)
            {
                float sampleL = synths_[v]->output[0][i];
                float sampleR = synths_[v]->output[1][i];

                outputL[processed + i] += sampleL * vol * panL;
                outputR[processed + i] += sampleR * vol * panR;
            }
        }

        processed += blockSamples;
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
    return synths_[voice].get();
}

void SurgeBoxEngine::captureAllVoices()
{
    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (synths_[i])
            project_.voices[i].captureFromSynth(synths_[i].get());
    }
}

void SurgeBoxEngine::restoreAllVoices()
{
    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (synths_[i])
            project_.voices[i].restoreToSynth(synths_[i].get());
    }
}

} // namespace SurgeBox
