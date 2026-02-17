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
#include <functional>
#include <string>
#include <vector>

class TiXmlElement;

namespace SurgeBox
{

// Targets that a CC can be mapped to
enum class MappingTarget : int
{
    None = 0,
    MasterVolume,
    Tempo,
    Voice1Volume, Voice2Volume, Voice3Volume, Voice4Volume,
    Voice1Pan, Voice2Pan, Voice3Pan, Voice4Pan,
    Voice1Mute, Voice2Mute, Voice3Mute, Voice4Mute,
    Voice1Solo, Voice2Solo, Voice3Solo, Voice4Solo,
    Voice1SendA, Voice2SendA, Voice3SendA, Voice4SendA,
    Voice1SendB, Voice2SendB, Voice3SendB, Voice4SendB,
    NumTargets
};

const char *mappingTargetToString(MappingTarget target);
MappingTarget mappingTargetFromString(const char *str);

struct CCMapping
{
    int cc{-1};              // MIDI CC number (0-127), -1 = unmapped
    int channel{-1};         // MIDI channel (0-15), -1 = any
    MappingTarget target{MappingTarget::None};
    float minValue{0.0f};    // Range minimum
    float maxValue{1.0f};    // Range maximum
};

class MidiMappingEngine
{
  public:
    static constexpr int MAX_MAPPINGS = 64;

    MidiMappingEngine();

    // Process a CC message, applying mapped values to project state.
    // Returns true if the CC was consumed by a mapping.
    bool processCC(int channel, int cc, int value, GrooveboxProject &project);

    // Mapping management
    int addMapping(const CCMapping &mapping);
    void removeMapping(int index);
    void clearAllMappings();
    int getNumMappings() const;
    const CCMapping &getMapping(int index) const;

    // MIDI Learn
    void startLearn(MappingTarget target);
    void cancelLearn();
    bool isLearning() const { return learning_; }
    MappingTarget getLearnTarget() const { return learnTarget_; }

    // Returns true if learn completed (consumed the CC)
    bool handleLearnCC(int channel, int cc);

    // Serialization
    void toXML(TiXmlElement *parent) const;
    void fromXML(TiXmlElement *element);

    // Callback when a mapping is learned or changed
    std::function<void()> onMappingsChanged;

  private:
    void applyMapping(const CCMapping &mapping, int ccValue, GrooveboxProject &project);

    std::vector<CCMapping> mappings_;
    bool learning_{false};
    MappingTarget learnTarget_{MappingTarget::None};

    static const CCMapping nullMapping_;
};

} // namespace SurgeBox
