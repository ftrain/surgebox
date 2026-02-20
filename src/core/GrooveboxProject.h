/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#include "filesystem/import.h"

class TiXmlElement;
class TiXmlDocument;

namespace juce
{
class AudioProcessor;
} // namespace juce

namespace SurgeBox
{

static constexpr int NUM_VOICES = 4;
static constexpr int NUM_GLOBAL_FX = 4;
static constexpr int FX_PARAMS_PER_SLOT = 12;

// ============================================================================
// Instrument Types
// ============================================================================

enum class InstrumentType : int
{
    Unknown = -1,
    SurgeXT = 0,
    Dexed = 1,
    TR808 = 2
};

// ============================================================================
// File Format
// ============================================================================

#pragma pack(push, 1)
struct ProjectHeader
{
    char tag[4];              // "SBOX"
    uint32_t version;
    uint32_t xmlsize;
    uint32_t numVoices;
    uint32_t reserved[8];
};
#pragma pack(pop)

static constexpr uint32_t PROJECT_FORMAT_VERSION = 2;

// ============================================================================
// MIDI Note
// ============================================================================

struct MIDINote
{
    double startBeat{0.0};
    double duration{1.0};
    uint8_t pitch{60};
    uint8_t velocity{100};

    MIDINote() = default;
    MIDINote(double start, double dur, uint8_t p, uint8_t vel)
        : startBeat(start), duration(dur), pitch(p), velocity(vel)
    {
    }

    double endBeat() const { return startBeat + duration; }
    bool operator<(const MIDINote &other) const;

    void toXML(TiXmlElement *parent) const;
    static MIDINote fromXML(TiXmlElement *element);
};

// ============================================================================
// Loop Region — non-destructive tiling of a beat range
// ============================================================================

struct LoopRegion
{
    double startBeat{0.0};
    double endBeat{0.0};
    int minPitch{0};
    int maxPitch{127};
    bool active{false};

    double length() const { return endBeat - startBeat; }
    bool containsPitch(int pitch) const { return pitch >= minPitch && pitch <= maxPitch; }
};

// ============================================================================
// Pattern Kernel — matrix transformation applied per-note at playback time
// ============================================================================

// A single kernel cell: defines one derived note relative to each source note
struct KernelCell
{
    int pitchOffset{0};         // Semitones relative to source note
    double timeOffset{0.0};     // Beats relative to source note start
    float velocityScale{1.0f};  // Multiply source velocity (0.0–1.0)
    float probability{1.0f};    // Chance this cell fires (0.0–1.0)

    void toXML(TiXmlElement *parent) const;
    static KernelCell fromXML(TiXmlElement *element);
};

// How the kernel interacts with the source pattern
enum class KernelMode : int
{
    Off = 0,        // Kernel disabled, play pattern as-is
    Spawn = 1,      // Each source note spawns additional notes from kernel cells
    Transform = 2,  // Kernel cells replace the source note (first cell = the note)
    Invert = 3,     // Mirror pitches around pivotPitch
    Retrograde = 4  // Reverse the time axis of the pattern
};

struct PatternKernel
{
    KernelMode mode{KernelMode::Off};
    std::vector<KernelCell> cells;

    // Scale-awareness: snap kernel-generated pitches to this scale
    bool scaleAware{true};       // When true, pitchOffsets are in scale degrees, not semitones
    int scaleRoot{0};            // MIDI note number of scale root (0=C)
    int scaleType{0};            // Maps to ScaleType enum

    // Pivot for Invert mode
    int pivotPitch{60};          // Center pitch for melodic inversion

    // Iteration behavior (how kernel evolves across pattern loops)
    int accumulateSemitones{0};  // Add N semitones per iteration (rising/falling sequences)
    int resetAfterIterations{0}; // Reset accumulation after N iterations (0 = never)

    // Probability seed — 0 means use random seed each loop (fresh variation)
    uint32_t seed{0};

    // Chord tracking — when true, kernel pitches follow the global chord progression
    bool followChords{false};

    bool isActive() const { return mode != KernelMode::Off && !cells.empty(); }

    void toXML(TiXmlElement *parent) const;
    void fromXML(TiXmlElement *element);
};

// Factory presets for common musical kernels
namespace KernelPresets
{
    PatternKernel arpeggioUp();         // Major triad rising
    PatternKernel arpeggioDown();       // Major triad falling
    PatternKernel arpeggioUpDown();     // Up then down
    PatternKernel chord();              // Simultaneous major triad
    PatternKernel octaveDouble();       // Double an octave up
    PatternKernel echo();               // Rhythmic echo with decay
    PatternKernel strum();              // Guitar-like micro-timing
    PatternKernel probabilityThin();    // Random note dropout
    PatternKernel risingSequence();     // Transpose up 1 semitone per iteration
    PatternKernel invertMelody(int pivot = 60);  // Melodic inversion
} // namespace KernelPresets

// ============================================================================
// Chord Progression — global harmonic timeline that voices can follow
// ============================================================================

enum class ChordQuality : int
{
    Major = 0,
    Minor = 1,
    Dominant7 = 2,
    Major7 = 3,
    Minor7 = 4,
    Diminished = 5,
    Augmented = 6,
    Sus2 = 7,
    Sus4 = 8,
    Power = 9  // Root + 5th only
};

struct ChordEvent
{
    double startBeat{0.0};       // When this chord begins (in global beats)
    int root{0};                 // Root note as pitch class (0=C, 1=C#, ..., 11=B)
    ChordQuality quality{ChordQuality::Major};
    int bassNote{-1};            // Slash chord bass (-1 = use root)

    // Get the intervals (semitones from root) for this chord's quality
    static const std::vector<int> &getIntervals(ChordQuality q);

    // Check if a pitch class (0-11) belongs to this chord
    bool containsPitchClass(int pc) const;

    // Find the nearest chord tone to a given pitch
    int findNearestChordTone(int pitch) const;

    void toXML(TiXmlElement *parent) const;
    static ChordEvent fromXML(TiXmlElement *element);
};

struct ChordProgression
{
    std::vector<ChordEvent> events;  // Sorted by startBeat
    bool active{false};              // Global enable/disable

    // Get the chord active at a given beat
    const ChordEvent *chordAtBeat(double beat) const;

    // Sort events by start beat
    void sort();
    void clear();

    void toXML(TiXmlElement *parent) const;
    void fromXML(TiXmlElement *element);
};

// ============================================================================
// Pattern
// ============================================================================

struct Pattern
{
    std::vector<MIDINote> notes;
    int bars{4};
    double swing{0.0};
    std::vector<LoopRegion> loopRegions;
    PatternKernel kernel;
    bool snapToChord{false};  // When true, snap all note pitches to the active chord

    double lengthInBeats() const { return bars * 4.0; }

    void addNote(double startBeat, double duration, uint8_t pitch, uint8_t velocity);
    void removeNote(size_t index);
    void removeNotesAt(double beat, uint8_t pitch, double tolerance = 0.01);
    void clear();
    void sortNotes();

    MIDINote *findNoteAt(double beat, uint8_t pitch, double tolerance = 0.01);
    std::vector<MIDINote *> getNotesInRange(double startBeat, double endBeat);
    std::vector<const MIDINote *> getNotesStartingInRange(double startBeat, double endBeat) const;

    // Rebuild cached loop notes from source region
    void rebuildLoopNotes();

    void toXML(TiXmlElement *parent) const;
    void fromXML(TiXmlElement *element);

  private:
    // Cached looped copies (rebuilt when notes or loop region change)
    std::vector<MIDINote> loopedNotes_;
};

// ============================================================================
// Chord Track — a control track whose notes define the chord progression
// ============================================================================

namespace ChordRecognition
{
    struct ChordResult
    {
        int root{0};
        ChordQuality quality{ChordQuality::Major};
        bool recognized{false};
    };

    // Given a set of pitch classes (0-11), identify chord quality and root
    ChordResult identify(const std::vector<int> &pitchClasses);

    // Human-readable chord name: "C", "Am7", "F#dim", etc.
    std::string chordName(int root, ChordQuality quality);
    std::string chordName(const ChordEvent &ev);
} // namespace ChordRecognition

struct ChordTrack
{
    Pattern pattern;
    std::atomic<double> tempoMultiplier{1.0};
    std::atomic<double> pendingTempoMultiplier{1.0};

    // Limited pitch range for chord entry (2 octaves: C3–C5)
    static constexpr int LOWEST_NOTE = 48;
    static constexpr int HIGHEST_NOTE = 73;

    ChordTrack();
    ChordTrack(const ChordTrack &other);
    ChordTrack &operator=(const ChordTrack &other);

    // Analyze pattern notes into ChordProgression events
    void rebuildChords(ChordProgression &prog) const;

    void toXML(TiXmlElement *parent) const;
    void fromXML(TiXmlElement *element);
};

// ============================================================================
// Global FX Slot
// ============================================================================

struct GlobalFXSlot
{
    int type{0};
    std::array<float, FX_PARAMS_PER_SLOT> params{};
    bool enabled{true};

    GlobalFXSlot();

    void toXML(TiXmlElement *parent, int index) const;
    void fromXML(TiXmlElement *element);
};

// ============================================================================
// Voice State
// ============================================================================

struct VoiceState
{
    std::string name;
    std::vector<char> patchData;
    Pattern pattern;
    InstrumentType instrumentType{InstrumentType::SurgeXT};

    float volume{1.0f};
    float pan{0.0f};
    float sendA{0.0f};
    float sendB{0.0f};
    bool mute{false};
    bool solo{false};
    std::atomic<double> tempoMultiplier{1.0};        // Current tempo multiplier (0.0625 to 4.0)
    std::atomic<double> pendingTempoMultiplier{1.0}; // Pending change (applied at next bar boundary)

    VoiceState();
    VoiceState(const VoiceState &other);
    VoiceState &operator=(const VoiceState &other);

    void toXML(TiXmlElement *parent, int index) const;
    void fromXML(TiXmlElement *element, int index);

    void captureFromProcessor(juce::AudioProcessor *proc);
    void restoreToProcessor(juce::AudioProcessor *proc);
};

// ============================================================================
// Groovebox Project
// ============================================================================

class MidiMappingEngine; // forward declaration

class GrooveboxProject
{
  public:
    double tempo{120.0};
    int loopBars{4};
    double swing{0.0};
    float masterVolume{0.8f};

    std::array<VoiceState, NUM_VOICES> voices;
    std::array<GlobalFXSlot, NUM_GLOBAL_FX> globalFX;
    ChordTrack chordTrack;
    ChordProgression chordProgression;  // Derived from chordTrack.pattern

    std::string projectName{"Untitled"};
    std::string author;
    std::string comment;
    std::vector<std::string> tags;

    // Optional pointer to MIDI mapping engine for serialization
    MidiMappingEngine *midiMappingEngine{nullptr};

    GrooveboxProject();

    bool saveToFile(const fs::path &path);
    bool loadFromFile(const fs::path &path);
    void reset();

    int getMaxPatternBars() const;

  private:
    void toXML(TiXmlDocument &doc);
    void fromXML(TiXmlDocument &doc);
    std::string getCurrentTimestamp() const;

    std::string createdDate_;
    std::string modifiedDate_;
};

} // namespace SurgeBox
