/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include "GrooveboxProject.h"
#include "PatternModel.h"
#include "MasterFXChain.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <memory>
#include <atomic>
#include <functional>

namespace SurgeBox
{

// ============================================================================
// Sequencer Engine - Playback control (instrument-agnostic, MIDI-buffer based)
// ============================================================================

class SequencerEngine
{
  public:
    SequencerEngine();

    void setProject(GrooveboxProject *project);

    void play();
    void stop();
    void setPlaying(bool playing);
    bool isPlaying() const { return playing_.load(); }

    void setPositionBeats(double beat);
    double getPositionBeats() const { return currentBeat_.load(); }
    void rewind() { setPositionBeats(0.0); }

    double getLoopEndBeat() const;

    // Get currently playing notes for a voice (for UI highlighting)
    std::vector<uint8_t> getPlayingNotes(int voiceIndex) const;

    // Called from audio thread - populates midiBuffers for each voice
    void process(int numSamples, double sampleRate,
                 std::array<juce::MidiBuffer *, NUM_VOICES> midiBuffers);

  private:
    void triggerNotesInRange(double startBeat, double endBeat, int numSamples,
                             std::array<juce::MidiBuffer *, NUM_VOICES> midiBuffers,
                             int baseSampleOffset = 0);
    void releaseNotesEndingInRange(double startBeat, double endBeat, int numSamples,
                                   std::array<juce::MidiBuffer *, NUM_VOICES> midiBuffers,
                                   int baseSampleOffset = 0);

    GrooveboxProject *project_{nullptr};

    std::atomic<bool> playing_{false};
    std::atomic<bool> pendingStop_{false};
    std::atomic<double> pendingPositionJump_{-1.0};
    std::atomic<double> currentBeat_{0.0};
    double sampleRate_{44100.0};
    double beatsPerSample_{0.0};
    double blockStartBeat_{0.0};
    int numSamplesInBlock_{0};

    struct ActiveNote
    {
        int voiceIndex;
        uint8_t pitch;
        double endBeat;
    };
    std::vector<ActiveNote> activeNotes_;
};

// ============================================================================
// Pending note event for UI -> audio thread communication
// ============================================================================

struct PendingNoteEvent
{
    int voiceIndex;
    uint8_t pitch;
    uint8_t velocity;
    bool noteOn;
};

// ============================================================================
// SurgeBox Engine - Multi-instance manager
// Works with any juce::AudioProcessor (Surge XT, Dexed, TR-808, etc.)
// ============================================================================

class SurgeBoxEngine
{
  public:
    SurgeBoxEngine();
    ~SurgeBoxEngine();

    // Initialization - processors are injected from plugin layer
    void setProcessors(std::array<juce::AudioProcessor *, NUM_VOICES> processors,
                       std::array<InstrumentType, NUM_VOICES> types);
    bool initialize(double sampleRate, int blockSize);
    void shutdown();

    // Audio processing
    void process(float *outputL, float *outputR, int numSamples);

    // Voice management
    int getActiveVoice() const { return activeVoice_; }
    void setActiveVoice(int voice);

    // Instrument type for a voice
    InstrumentType getInstrumentType(int voice) const;

    // Generic processor access (for UI editor creation)
    juce::AudioProcessor *getProcessor(int voice);
    juce::AudioProcessor *getActiveProcessor() { return getProcessor(activeVoice_); }

    // Send note from UI thread (queued for audio thread consumption)
    void sendNoteToActiveVoice(int pitch, int velocity, bool noteOn);

    // Project
    GrooveboxProject &getProject() { return project_; }
    const GrooveboxProject &getProject() const { return project_; }

    // Pattern models (with undo support)
    PatternModel *getPatternModel(int voice);
    PatternModel *getActivePatternModel() { return getPatternModel(activeVoice_); }
    juce::UndoManager &getUndoManager() { return undoManager_; }

    // Sync pattern models to/from project (for save/load)
    void syncPatternModelsFromProject();
    void syncPatternModelsToProject();

    // Transport
    SequencerEngine &getSequencer() { return sequencer_; }
    const SequencerEngine &getSequencer() const { return sequencer_; }

    void play() { sequencer_.play(); }
    void stop() { sequencer_.stop(); }
    bool isPlaying() const { return sequencer_.isPlaying(); }
    double getPlayheadBeats() const { return sequencer_.getPositionBeats(); }

    // Get playhead in voice-time (scaled by tempo multiplier) for active voice
    double getActiveVoicePlayheadBeats() const {
        double globalBeat = sequencer_.getPositionBeats();
        double multiplier = project_.voices[activeVoice_].tempoMultiplier.load();
        if (multiplier <= 0) multiplier = 1.0;
        double patternLength = project_.voices[activeVoice_].pattern.bars * 4.0;
        double voiceBeat = globalBeat * multiplier;
        return std::fmod(voiceBeat, patternLength);
    }

    // Get currently playing notes for UI highlighting
    std::vector<uint8_t> getPlayingNotes(int voice) const { return sequencer_.getPlayingNotes(voice); }
    std::vector<uint8_t> getActivePlayingNotes() const { return getPlayingNotes(activeVoice_); }

    // Capture current state of all processors into project
    void captureAllVoices();

    // Restore project state to all processors
    void restoreAllVoices();

    // Mark a voice as not ready (for thread-safe instrument swaps)
    void setVoiceReady(int voice, bool ready);

    // Master effects
    MasterFXChain &getMasterFXChain() { return masterFXChain_; }
    const MasterFXChain &getMasterFXChain() const { return masterFXChain_; }

    // Sample rate
    double getSampleRate() const { return sampleRate_; }

    // Callbacks for UI updates
    std::function<void(int)> onVoiceChanged;
    std::function<void(double)> onPlayheadMoved;

  private:
    void mixVoices(float *outputL, float *outputR, int numSamples);
    void drainPendingNotes();

    // Processors are owned by plugin, we hold pointers
    std::array<juce::AudioProcessor *, NUM_VOICES> processors_{};
    std::array<InstrumentType, NUM_VOICES> instrumentTypes_{};

    // Thread safety: audio thread checks before processing a voice
    std::array<std::atomic<bool>, NUM_VOICES> voiceReady_{};

    GrooveboxProject project_;
    SequencerEngine sequencer_;
    juce::UndoManager undoManager_;
    std::array<std::unique_ptr<PatternModel>, NUM_VOICES> patternModels_;

    int activeVoice_{0};
    double sampleRate_{44100.0};
    int blockSize_{32};
    bool initialized_{false};

    // Mixing buffers - used to call processBlock on each processor
    alignas(16) float mixBufferL_[4096];
    alignas(16) float mixBufferR_[4096];

    // Pre-allocated buffer for voice processing (avoid allocations in audio thread)
    std::unique_ptr<juce::AudioBuffer<float>> voiceBuffer_;

    // Pre-allocated MIDI buffers for each voice (avoid allocations in audio thread)
    std::array<juce::MidiBuffer, NUM_VOICES> voiceMidiBuffers_;

    // Lock-free SPSC note queue for UI -> audio thread
    static constexpr int MAX_PENDING_NOTES = 64;
    std::array<PendingNoteEvent, MAX_PENDING_NOTES> pendingNotes_;
    std::atomic<int> pendingNoteWritePos_{0};
    std::atomic<int> pendingNoteReadPos_{0};

    // Block start position for syncing Surge's internal time
    double blockStartBeat_{0.0};

    // Master effects chain (Surge-based FX applied after voice mix)
    MasterFXChain masterFXChain_;
};

} // namespace SurgeBox
