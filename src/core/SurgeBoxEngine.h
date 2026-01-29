/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include "GrooveboxProject.h"
#include <array>
#include <memory>
#include <atomic>
#include <functional>

class SurgeSynthesizer;
class SurgeStorage;

namespace SurgeBox
{

// ============================================================================
// Sequencer Engine - Playback control
// ============================================================================

class SequencerEngine
{
  public:
    SequencerEngine();

    void setProject(GrooveboxProject *project);
    void setSynths(std::array<SurgeSynthesizer *, NUM_VOICES> synths);

    void play();
    void stop();
    void setPlaying(bool playing);
    bool isPlaying() const { return playing_.load(); }

    void setPositionBeats(double beat);
    double getPositionBeats() const { return currentBeat_.load(); }
    void rewind() { setPositionBeats(0.0); }

    double getLoopEndBeat() const;

    // Called from audio thread
    void process(int numSamples, double sampleRate);

  private:
    void triggerNotesInRange(double startBeat, double endBeat);
    void releaseNotesEndingInRange(double startBeat, double endBeat);

    GrooveboxProject *project_{nullptr};
    std::array<SurgeSynthesizer *, NUM_VOICES> synths_{};

    std::atomic<bool> playing_{false};
    std::atomic<double> currentBeat_{0.0};
    double sampleRate_{44100.0};

    struct ActiveNote
    {
        int voiceIndex;
        uint8_t pitch;
        double endBeat;
    };
    std::vector<ActiveNote> activeNotes_;
};

// ============================================================================
// SurgeBox Engine - Multi-instance manager
// ============================================================================

class SurgeBoxEngine
{
  public:
    SurgeBoxEngine();
    ~SurgeBoxEngine();

    // Initialization
    bool initialize(double sampleRate, int blockSize);
    void shutdown();

    // Audio processing
    void process(float *outputL, float *outputR, int numSamples);

    // Voice management
    int getActiveVoice() const { return activeVoice_; }
    void setActiveVoice(int voice);

    SurgeSynthesizer *getSynth(int voice);
    SurgeSynthesizer *getActiveSynth() { return getSynth(activeVoice_); }

    // Project
    GrooveboxProject &getProject() { return project_; }
    const GrooveboxProject &getProject() const { return project_; }

    // Transport
    SequencerEngine &getSequencer() { return sequencer_; }
    const SequencerEngine &getSequencer() const { return sequencer_; }

    void play() { sequencer_.play(); }
    void stop() { sequencer_.stop(); }
    bool isPlaying() const { return sequencer_.isPlaying(); }
    double getPlayheadBeats() const { return sequencer_.getPositionBeats(); }

    // Capture current state of all synths into project
    void captureAllVoices();

    // Restore project state to all synths
    void restoreAllVoices();

    // Sample rate
    double getSampleRate() const { return sampleRate_; }

    // Callbacks for UI updates
    std::function<void(int)> onVoiceChanged;
    std::function<void(double)> onPlayheadMoved;

  private:
    void mixVoices(float *outputL, float *outputR, int numSamples);

    std::array<std::unique_ptr<SurgeSynthesizer>, NUM_VOICES> synths_;
    std::shared_ptr<SurgeStorage> sharedStorage_;

    GrooveboxProject project_;
    SequencerEngine sequencer_;

    int activeVoice_{0};
    double sampleRate_{44100.0};
    int blockSize_{32};
    bool initialized_{false};

    // Mixing buffers
    alignas(16) float mixBufferL_[4096];
    alignas(16) float mixBufferR_[4096];
};

} // namespace SurgeBox
