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
#include "SurgeSynthesizer.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <memory>
#include <atomic>
#include <functional>

// Forward declarations
class SurgeSynthProcessor;

namespace juce
{
template <typename T>
class AudioBuffer;
} // namespace juce

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
    std::array<SurgeSynthesizer *, NUM_VOICES> synths_{};

    std::atomic<bool> playing_{false};
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
// SurgeBox Engine - Multi-instance manager
// Uses SurgeSynthProcessor to properly handle GUI keyboard input
// ============================================================================

class SurgeBoxEngine
{
  public:
    SurgeBoxEngine();
    ~SurgeBoxEngine();

    // Initialization - processors are injected from plugin layer
    void setProcessors(std::array<SurgeSynthProcessor *, NUM_VOICES> processors);
    bool initialize(double sampleRate, int blockSize);
    void shutdown();

    // Audio processing
    void process(float *outputL, float *outputR, int numSamples);

    // Voice management
    int getActiveVoice() const { return activeVoice_; }
    void setActiveVoice(int voice);

    // Access to synths (via processors)
    SurgeSynthesizer *getSynth(int voice);
    SurgeSynthesizer *getActiveSynth() { return getSynth(activeVoice_); }

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

    // Get currently playing notes for UI highlighting
    std::vector<uint8_t> getPlayingNotes(int voice) const { return sequencer_.getPlayingNotes(voice); }
    std::vector<uint8_t> getActivePlayingNotes() const { return getPlayingNotes(activeVoice_); }

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

    // Processors are owned by plugin, we hold pointers
    std::array<SurgeSynthProcessor *, NUM_VOICES> processors_{};

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

    // Block start position for syncing Surge's internal time
    double blockStartBeat_{0.0};
};

} // namespace SurgeBox
