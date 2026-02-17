/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "MidiMappingEngine.h"
#include "tinyxml/tinyxml.h"
#include <algorithm>
#include <cstring>

namespace SurgeBox
{

const CCMapping MidiMappingEngine::nullMapping_{};

// --- Target string conversion ---

struct TargetStringPair
{
    MappingTarget target;
    const char *str;
};

static const TargetStringPair targetStrings[] = {
    {MappingTarget::None, "none"},
    {MappingTarget::MasterVolume, "master_volume"},
    {MappingTarget::Tempo, "tempo"},
    {MappingTarget::Voice1Volume, "voice1_volume"},
    {MappingTarget::Voice2Volume, "voice2_volume"},
    {MappingTarget::Voice3Volume, "voice3_volume"},
    {MappingTarget::Voice4Volume, "voice4_volume"},
    {MappingTarget::Voice1Pan, "voice1_pan"},
    {MappingTarget::Voice2Pan, "voice2_pan"},
    {MappingTarget::Voice3Pan, "voice3_pan"},
    {MappingTarget::Voice4Pan, "voice4_pan"},
    {MappingTarget::Voice1Mute, "voice1_mute"},
    {MappingTarget::Voice2Mute, "voice2_mute"},
    {MappingTarget::Voice3Mute, "voice3_mute"},
    {MappingTarget::Voice4Mute, "voice4_mute"},
    {MappingTarget::Voice1Solo, "voice1_solo"},
    {MappingTarget::Voice2Solo, "voice2_solo"},
    {MappingTarget::Voice3Solo, "voice3_solo"},
    {MappingTarget::Voice4Solo, "voice4_solo"},
    {MappingTarget::Voice1SendA, "voice1_sendA"},
    {MappingTarget::Voice2SendA, "voice2_sendA"},
    {MappingTarget::Voice3SendA, "voice3_sendA"},
    {MappingTarget::Voice4SendA, "voice4_sendA"},
    {MappingTarget::Voice1SendB, "voice1_sendB"},
    {MappingTarget::Voice2SendB, "voice2_sendB"},
    {MappingTarget::Voice3SendB, "voice3_sendB"},
    {MappingTarget::Voice4SendB, "voice4_sendB"},
};

const char *mappingTargetToString(MappingTarget target)
{
    for (const auto &pair : targetStrings)
    {
        if (pair.target == target)
            return pair.str;
    }
    return "none";
}

MappingTarget mappingTargetFromString(const char *str)
{
    if (!str)
        return MappingTarget::None;
    for (const auto &pair : targetStrings)
    {
        if (std::strcmp(pair.str, str) == 0)
            return pair.target;
    }
    return MappingTarget::None;
}

// --- MidiMappingEngine ---

MidiMappingEngine::MidiMappingEngine() = default;

bool MidiMappingEngine::processCC(int channel, int cc, int value, GrooveboxProject &project)
{
    // Check learn mode first
    if (learning_ && handleLearnCC(channel, cc))
        return true;

    bool consumed = false;
    for (const auto &mapping : mappings_)
    {
        if (mapping.cc == cc && (mapping.channel == -1 || mapping.channel == channel))
        {
            applyMapping(mapping, value, project);
            consumed = true;
        }
    }
    return consumed;
}

void MidiMappingEngine::applyMapping(const CCMapping &mapping, int ccValue,
                                      GrooveboxProject &project)
{
    // Normalize CC value (0-127) to 0.0-1.0
    float normalized = static_cast<float>(ccValue) / 127.0f;

    // Scale to mapping range
    float scaled = mapping.minValue + normalized * (mapping.maxValue - mapping.minValue);

    // Helper to get voice index from target enum offset
    auto voiceIndex = [](MappingTarget target, MappingTarget base) -> int {
        return static_cast<int>(target) - static_cast<int>(base);
    };

    switch (mapping.target)
    {
        case MappingTarget::MasterVolume:
            project.masterVolume = scaled;
            break;

        case MappingTarget::Tempo:
            // Map 0-1 to 20-300 BPM
            project.tempo = 20.0 + static_cast<double>(scaled) * 280.0;
            break;

        case MappingTarget::Voice1Volume:
        case MappingTarget::Voice2Volume:
        case MappingTarget::Voice3Volume:
        case MappingTarget::Voice4Volume:
        {
            int v = voiceIndex(mapping.target, MappingTarget::Voice1Volume);
            if (v >= 0 && v < NUM_VOICES)
                project.voices[v].volume = scaled;
            break;
        }

        case MappingTarget::Voice1Pan:
        case MappingTarget::Voice2Pan:
        case MappingTarget::Voice3Pan:
        case MappingTarget::Voice4Pan:
        {
            int v = voiceIndex(mapping.target, MappingTarget::Voice1Pan);
            if (v >= 0 && v < NUM_VOICES)
                project.voices[v].pan = scaled * 2.0f - 1.0f; // Map 0-1 to -1..+1
            break;
        }

        case MappingTarget::Voice1Mute:
        case MappingTarget::Voice2Mute:
        case MappingTarget::Voice3Mute:
        case MappingTarget::Voice4Mute:
        {
            int v = voiceIndex(mapping.target, MappingTarget::Voice1Mute);
            if (v >= 0 && v < NUM_VOICES)
                project.voices[v].mute = (ccValue >= 64);
            break;
        }

        case MappingTarget::Voice1Solo:
        case MappingTarget::Voice2Solo:
        case MappingTarget::Voice3Solo:
        case MappingTarget::Voice4Solo:
        {
            int v = voiceIndex(mapping.target, MappingTarget::Voice1Solo);
            if (v >= 0 && v < NUM_VOICES)
                project.voices[v].solo = (ccValue >= 64);
            break;
        }

        case MappingTarget::Voice1SendA:
        case MappingTarget::Voice2SendA:
        case MappingTarget::Voice3SendA:
        case MappingTarget::Voice4SendA:
        {
            int v = voiceIndex(mapping.target, MappingTarget::Voice1SendA);
            if (v >= 0 && v < NUM_VOICES)
                project.voices[v].sendA = scaled;
            break;
        }

        case MappingTarget::Voice1SendB:
        case MappingTarget::Voice2SendB:
        case MappingTarget::Voice3SendB:
        case MappingTarget::Voice4SendB:
        {
            int v = voiceIndex(mapping.target, MappingTarget::Voice1SendB);
            if (v >= 0 && v < NUM_VOICES)
                project.voices[v].sendB = scaled;
            break;
        }

        default:
            break;
    }
}

int MidiMappingEngine::addMapping(const CCMapping &mapping)
{
    if (static_cast<int>(mappings_.size()) >= MAX_MAPPINGS)
        return -1;
    mappings_.push_back(mapping);
    return static_cast<int>(mappings_.size()) - 1;
}

void MidiMappingEngine::removeMapping(int index)
{
    if (index >= 0 && index < static_cast<int>(mappings_.size()))
        mappings_.erase(mappings_.begin() + index);
}

void MidiMappingEngine::clearAllMappings()
{
    mappings_.clear();
}

int MidiMappingEngine::getNumMappings() const
{
    return static_cast<int>(mappings_.size());
}

const CCMapping &MidiMappingEngine::getMapping(int index) const
{
    if (index >= 0 && index < static_cast<int>(mappings_.size()))
        return mappings_[index];
    return nullMapping_;
}

void MidiMappingEngine::startLearn(MappingTarget target)
{
    learning_ = true;
    learnTarget_ = target;
}

void MidiMappingEngine::cancelLearn()
{
    learning_ = false;
    learnTarget_ = MappingTarget::None;
}

bool MidiMappingEngine::handleLearnCC(int channel, int cc)
{
    if (!learning_ || learnTarget_ == MappingTarget::None)
        return false;

    // Remove any existing mapping for this CC + channel
    mappings_.erase(
        std::remove_if(mappings_.begin(), mappings_.end(),
                        [cc, channel](const CCMapping &m) {
                            return m.cc == cc && (m.channel == channel || m.channel == -1);
                        }),
        mappings_.end());

    // Create new mapping
    CCMapping newMapping;
    newMapping.cc = cc;
    newMapping.channel = channel;
    newMapping.target = learnTarget_;
    addMapping(newMapping);

    learning_ = false;
    learnTarget_ = MappingTarget::None;

    if (onMappingsChanged)
        onMappingsChanged();

    return true;
}

// --- Serialization ---

void MidiMappingEngine::toXML(TiXmlElement *parent) const
{
    TiXmlElement mappingsEl("midi_mappings");

    for (const auto &m : mappings_)
    {
        if (m.target == MappingTarget::None)
            continue;

        TiXmlElement mapEl("mapping");
        mapEl.SetAttribute("cc", m.cc);
        mapEl.SetAttribute("channel", m.channel);
        mapEl.SetAttribute("target", mappingTargetToString(m.target));
        mapEl.SetDoubleAttribute("min", m.minValue);
        mapEl.SetDoubleAttribute("max", m.maxValue);
        mappingsEl.InsertEndChild(mapEl);
    }

    parent->InsertEndChild(mappingsEl);
}

void MidiMappingEngine::fromXML(TiXmlElement *element)
{
    mappings_.clear();

    TiXmlElement *mappingsEl = element->FirstChildElement("midi_mappings");
    if (!mappingsEl)
        return;

    for (TiXmlElement *mapEl = mappingsEl->FirstChildElement("mapping"); mapEl;
         mapEl = mapEl->NextSiblingElement("mapping"))
    {
        CCMapping m;
        mapEl->QueryIntAttribute("cc", &m.cc);
        mapEl->QueryIntAttribute("channel", &m.channel);
        m.target = mappingTargetFromString(mapEl->Attribute("target"));

        double minVal = 0.0, maxVal = 1.0;
        mapEl->QueryDoubleAttribute("min", &minVal);
        mapEl->QueryDoubleAttribute("max", &maxVal);
        m.minValue = static_cast<float>(minVal);
        m.maxValue = static_cast<float>(maxVal);

        if (m.target != MappingTarget::None)
            mappings_.push_back(m);
    }
}

} // namespace SurgeBox
