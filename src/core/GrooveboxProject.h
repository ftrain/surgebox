/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#include "filesystem/import.h"

class TiXmlElement;
class TiXmlDocument;
class SurgeStorage;
class SurgeSynthesizer;

namespace SurgeBox
{

static constexpr int NUM_VOICES = 4;
static constexpr int NUM_GLOBAL_FX = 4;
static constexpr int FX_PARAMS_PER_SLOT = 12;

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

static constexpr uint32_t PROJECT_FORMAT_VERSION = 1;

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
// Pattern
// ============================================================================

struct Pattern
{
    std::vector<MIDINote> notes;
    int bars{4};
    double swing{0.0};

    double lengthInBeats() const { return bars * 4.0; }

    void addNote(double startBeat, double duration, uint8_t pitch, uint8_t velocity);
    void removeNote(size_t index);
    void removeNotesAt(double beat, uint8_t pitch, double tolerance = 0.01);
    void clear();
    void sortNotes();

    MIDINote *findNoteAt(double beat, uint8_t pitch, double tolerance = 0.01);
    std::vector<MIDINote *> getNotesInRange(double startBeat, double endBeat);
    std::vector<const MIDINote *> getNotesStartingInRange(double startBeat, double endBeat) const;

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

    float volume{1.0f};
    float pan{0.0f};
    float sendA{0.0f};
    float sendB{0.0f};
    bool mute{false};
    bool solo{false};

    VoiceState();

    void toXML(TiXmlElement *parent, int index) const;
    void fromXML(TiXmlElement *element, int index);

    void captureFromSynth(SurgeSynthesizer *synth);
    void restoreToSynth(SurgeSynthesizer *synth);
};

// ============================================================================
// Groovebox Project
// ============================================================================

class GrooveboxProject
{
  public:
    double tempo{120.0};
    int loopBars{4};
    double swing{0.0};
    float masterVolume{0.8f};

    std::array<VoiceState, NUM_VOICES> voices;
    std::array<GlobalFXSlot, NUM_GLOBAL_FX> globalFX;

    std::string projectName{"Untitled"};
    std::string author;
    std::string comment;
    std::vector<std::string> tags;

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
