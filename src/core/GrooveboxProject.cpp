/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "GrooveboxProject.h"
#include "SurgeSynthesizer.h"
#include "tinyxml/tinyxml.h"
#include "sst/basic-blocks/mechanics/endian-ops.h"
#include <fmt/core.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace mech = sst::basic_blocks::mechanics;

namespace SurgeBox
{

// ============================================================================
// MIDINote
// ============================================================================

bool MIDINote::operator<(const MIDINote &other) const
{
    if (startBeat != other.startBeat)
        return startBeat < other.startBeat;
    return pitch < other.pitch;
}

void MIDINote::toXML(TiXmlElement *parent) const
{
    TiXmlElement noteEl("note");
    noteEl.SetDoubleAttribute("start", startBeat);
    noteEl.SetDoubleAttribute("duration", duration);
    noteEl.SetAttribute("pitch", pitch);
    noteEl.SetAttribute("velocity", velocity);
    parent->InsertEndChild(noteEl);
}

MIDINote MIDINote::fromXML(TiXmlElement *element)
{
    MIDINote note;
    element->QueryDoubleAttribute("start", &note.startBeat);
    element->QueryDoubleAttribute("duration", &note.duration);

    int pitchInt = 60, velInt = 100;
    element->QueryIntAttribute("pitch", &pitchInt);
    element->QueryIntAttribute("velocity", &velInt);
    note.pitch = static_cast<uint8_t>(std::clamp(pitchInt, 0, 127));
    note.velocity = static_cast<uint8_t>(std::clamp(velInt, 1, 127));

    return note;
}

// ============================================================================
// Pattern
// ============================================================================

void Pattern::addNote(double start, double dur, uint8_t pitch, uint8_t velocity)
{
    notes.emplace_back(start, dur, pitch, velocity);
    sortNotes();
}

void Pattern::removeNote(size_t index)
{
    if (index < notes.size())
        notes.erase(notes.begin() + static_cast<ptrdiff_t>(index));
}

void Pattern::removeNotesAt(double beat, uint8_t pitch, double tolerance)
{
    notes.erase(std::remove_if(notes.begin(), notes.end(),
                               [beat, pitch, tolerance](const MIDINote &n) {
                                   return n.pitch == pitch &&
                                          std::abs(n.startBeat - beat) < tolerance;
                               }),
                notes.end());
}

void Pattern::clear() { notes.clear(); }

void Pattern::sortNotes() { std::sort(notes.begin(), notes.end()); }

MIDINote *Pattern::findNoteAt(double beat, uint8_t pitch, double tolerance)
{
    for (auto &note : notes)
    {
        if (note.pitch == pitch && beat >= note.startBeat - tolerance &&
            beat < note.endBeat() + tolerance)
        {
            return &note;
        }
    }
    return nullptr;
}

std::vector<MIDINote *> Pattern::getNotesInRange(double startBeat, double endBeat)
{
    std::vector<MIDINote *> result;
    for (auto &note : notes)
    {
        if (note.startBeat < endBeat && note.endBeat() > startBeat)
            result.push_back(&note);
    }
    return result;
}

std::vector<const MIDINote *> Pattern::getNotesStartingInRange(double startBeat,
                                                                double endBeat) const
{
    std::vector<const MIDINote *> result;
    for (const auto &note : notes)
    {
        if (note.startBeat >= startBeat && note.startBeat < endBeat)
            result.push_back(&note);
    }
    return result;
}

void Pattern::toXML(TiXmlElement *parent) const
{
    TiXmlElement patternEl("pattern");
    patternEl.SetAttribute("bars", bars);
    patternEl.SetDoubleAttribute("swing", swing);

    for (const auto &note : notes)
        note.toXML(&patternEl);

    parent->InsertEndChild(patternEl);
}

void Pattern::fromXML(TiXmlElement *element)
{
    notes.clear();
    element->QueryIntAttribute("bars", &bars);
    element->QueryDoubleAttribute("swing", &swing);

    for (TiXmlElement *noteEl = element->FirstChildElement("note"); noteEl;
         noteEl = noteEl->NextSiblingElement("note"))
    {
        notes.push_back(MIDINote::fromXML(noteEl));
    }
    sortNotes();
}

// ============================================================================
// GlobalFXSlot
// ============================================================================

GlobalFXSlot::GlobalFXSlot() { params.fill(0.0f); }

void GlobalFXSlot::toXML(TiXmlElement *parent, int index) const
{
    TiXmlElement slotEl("slot");
    slotEl.SetAttribute("index", index);
    slotEl.SetAttribute("type", type);
    slotEl.SetAttribute("enabled", enabled ? 1 : 0);

    for (int i = 0; i < FX_PARAMS_PER_SLOT; i++)
    {
        TiXmlElement paramEl("param");
        paramEl.SetAttribute("index", i);
        paramEl.SetDoubleAttribute("value", params[i]);
        slotEl.InsertEndChild(paramEl);
    }

    parent->InsertEndChild(slotEl);
}

void GlobalFXSlot::fromXML(TiXmlElement *element)
{
    element->QueryIntAttribute("type", &type);

    int enabledInt = 1;
    element->QueryIntAttribute("enabled", &enabledInt);
    enabled = (enabledInt != 0);

    for (TiXmlElement *paramEl = element->FirstChildElement("param"); paramEl;
         paramEl = paramEl->NextSiblingElement("param"))
    {
        int index = 0;
        paramEl->QueryIntAttribute("index", &index);
        if (index >= 0 && index < FX_PARAMS_PER_SLOT)
        {
            double val = 0.0;
            paramEl->QueryDoubleAttribute("value", &val);
            params[index] = static_cast<float>(val);
        }
    }
}

// ============================================================================
// VoiceState
// ============================================================================

VoiceState::VoiceState() : name("Voice") {}

void VoiceState::toXML(TiXmlElement *parent, int index) const
{
    TiXmlElement voiceEl("voice");
    voiceEl.SetAttribute("index", index);
    voiceEl.SetAttribute("name", name.c_str());

    // Mixer
    TiXmlElement mixerEl("mixer");
    mixerEl.SetDoubleAttribute("volume", volume);
    mixerEl.SetDoubleAttribute("pan", pan);
    mixerEl.SetDoubleAttribute("sendA", sendA);
    mixerEl.SetDoubleAttribute("sendB", sendB);
    mixerEl.SetAttribute("mute", mute ? 1 : 0);
    mixerEl.SetAttribute("solo", solo ? 1 : 0);
    voiceEl.InsertEndChild(mixerEl);

    // Patch data as hex
    if (!patchData.empty())
    {
        TiXmlElement patchEl("patch");
        patchEl.SetAttribute("size", static_cast<int>(patchData.size()));

        std::string hexStr;
        hexStr.reserve(patchData.size() * 2);
        for (char c : patchData)
        {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02x", static_cast<unsigned char>(c));
            hexStr += buf;
        }

        TiXmlText textNode(hexStr.c_str());
        patchEl.InsertEndChild(textNode);
        voiceEl.InsertEndChild(patchEl);
    }

    pattern.toXML(&voiceEl);
    parent->InsertEndChild(voiceEl);
}

void VoiceState::fromXML(TiXmlElement *element, int /*index*/)
{
    if (const char *nameAttr = element->Attribute("name"))
        name = nameAttr;

    if (TiXmlElement *mixerEl = element->FirstChildElement("mixer"))
    {
        double val;
        if (mixerEl->QueryDoubleAttribute("volume", &val) == TIXML_SUCCESS)
            volume = static_cast<float>(val);
        if (mixerEl->QueryDoubleAttribute("pan", &val) == TIXML_SUCCESS)
            pan = static_cast<float>(val);
        if (mixerEl->QueryDoubleAttribute("sendA", &val) == TIXML_SUCCESS)
            sendA = static_cast<float>(val);
        if (mixerEl->QueryDoubleAttribute("sendB", &val) == TIXML_SUCCESS)
            sendB = static_cast<float>(val);

        int boolVal;
        if (mixerEl->QueryIntAttribute("mute", &boolVal) == TIXML_SUCCESS)
            mute = (boolVal != 0);
        if (mixerEl->QueryIntAttribute("solo", &boolVal) == TIXML_SUCCESS)
            solo = (boolVal != 0);
    }

    if (TiXmlElement *patchEl = element->FirstChildElement("patch"))
    {
        int size = 0;
        patchEl->QueryIntAttribute("size", &size);

        if (const char *hexData = patchEl->GetText())
        {
            patchData.clear();
            patchData.reserve(size);

            for (int i = 0; i < size && hexData[i * 2] && hexData[i * 2 + 1]; i++)
            {
                char byteStr[3] = {hexData[i * 2], hexData[i * 2 + 1], 0};
                unsigned int byteVal;
                if (sscanf(byteStr, "%02x", &byteVal) == 1)
                    patchData.push_back(static_cast<char>(byteVal));
            }
        }
    }

    if (TiXmlElement *patternEl = element->FirstChildElement("pattern"))
        pattern.fromXML(patternEl);
}

void VoiceState::captureFromSynth(SurgeSynthesizer *synth)
{
    if (!synth)
        return;

    void *data = nullptr;
    size_t size = synth->saveRaw(&data);

    if (data && size > 0)
    {
        patchData.resize(size);
        memcpy(patchData.data(), data, size);
        free(data);
    }

    name = synth->storage.getPatch().name;
    if (name.empty())
        name = "Voice";
}

void VoiceState::restoreToSynth(SurgeSynthesizer *synth)
{
    if (!synth || patchData.empty())
        return;
    synth->loadRaw(patchData.data(), patchData.size(), true);
}

// ============================================================================
// GrooveboxProject
// ============================================================================

GrooveboxProject::GrooveboxProject() { reset(); }

void GrooveboxProject::reset()
{
    tempo = 120.0;
    loopBars = 4;
    swing = 0.0;
    masterVolume = 0.8f;

    for (int i = 0; i < NUM_VOICES; i++)
    {
        voices[i] = VoiceState();
        voices[i].name = fmt::format("Voice {}", i + 1);
        voices[i].pattern.bars = loopBars;
    }

    for (int i = 0; i < NUM_GLOBAL_FX; i++)
        globalFX[i] = GlobalFXSlot();

    projectName = "Untitled";
    author.clear();
    comment.clear();
    tags.clear();
    createdDate_ = getCurrentTimestamp();
    modifiedDate_ = createdDate_;
}

int GrooveboxProject::getMaxPatternBars() const
{
    int maxBars = 1;
    for (const auto &voice : voices)
        maxBars = std::max(maxBars, voice.pattern.bars);
    return maxBars;
}

std::string GrooveboxProject::getCurrentTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

void GrooveboxProject::toXML(TiXmlDocument &doc)
{
    auto *decl = new TiXmlDeclaration("1.0", "UTF-8", "");
    doc.LinkEndChild(decl);

    TiXmlElement root("surgebox-project");
    root.SetAttribute("version", PROJECT_FORMAT_VERSION);

    // Global
    TiXmlElement globalEl("global");
    globalEl.SetDoubleAttribute("tempo", tempo);
    globalEl.SetAttribute("loop_bars", loopBars);
    globalEl.SetDoubleAttribute("swing", swing);
    globalEl.SetDoubleAttribute("master_volume", masterVolume);

    TiXmlElement globalFXEl("global_fx");
    for (int i = 0; i < NUM_GLOBAL_FX; i++)
        globalFX[i].toXML(&globalFXEl, i);
    globalEl.InsertEndChild(globalFXEl);
    root.InsertEndChild(globalEl);

    // Voices
    for (int i = 0; i < NUM_VOICES; i++)
        voices[i].toXML(&root, i);

    // Metadata
    TiXmlElement metaEl("meta");
    metaEl.SetAttribute("name", projectName.c_str());
    metaEl.SetAttribute("author", author.c_str());
    metaEl.SetAttribute("created", createdDate_.c_str());
    metaEl.SetAttribute("modified", modifiedDate_.c_str());

    if (!comment.empty())
    {
        TiXmlElement commentEl("comment");
        commentEl.InsertEndChild(TiXmlText(comment.c_str()));
        metaEl.InsertEndChild(commentEl);
    }

    if (!tags.empty())
    {
        TiXmlElement tagsEl("tags");
        for (const auto &tag : tags)
        {
            TiXmlElement tagEl("tag");
            tagEl.InsertEndChild(TiXmlText(tag.c_str()));
            tagsEl.InsertEndChild(tagEl);
        }
        metaEl.InsertEndChild(tagsEl);
    }

    root.InsertEndChild(metaEl);
    doc.InsertEndChild(root);
}

void GrooveboxProject::fromXML(TiXmlDocument &doc)
{
    TiXmlElement *root = doc.FirstChildElement("surgebox-project");
    if (!root)
        return;

    if (TiXmlElement *globalEl = root->FirstChildElement("global"))
    {
        globalEl->QueryDoubleAttribute("tempo", &tempo);
        globalEl->QueryIntAttribute("loop_bars", &loopBars);
        globalEl->QueryDoubleAttribute("swing", &swing);

        double mv;
        if (globalEl->QueryDoubleAttribute("master_volume", &mv) == TIXML_SUCCESS)
            masterVolume = static_cast<float>(mv);

        if (TiXmlElement *globalFXEl = globalEl->FirstChildElement("global_fx"))
        {
            for (TiXmlElement *slotEl = globalFXEl->FirstChildElement("slot"); slotEl;
                 slotEl = slotEl->NextSiblingElement("slot"))
            {
                int index = 0;
                slotEl->QueryIntAttribute("index", &index);
                if (index >= 0 && index < NUM_GLOBAL_FX)
                    globalFX[index].fromXML(slotEl);
            }
        }
    }

    for (TiXmlElement *voiceEl = root->FirstChildElement("voice"); voiceEl;
         voiceEl = voiceEl->NextSiblingElement("voice"))
    {
        int index = 0;
        voiceEl->QueryIntAttribute("index", &index);
        if (index >= 0 && index < NUM_VOICES)
            voices[index].fromXML(voiceEl, index);
    }

    if (TiXmlElement *metaEl = root->FirstChildElement("meta"))
    {
        if (const char *attr = metaEl->Attribute("name"))
            projectName = attr;
        if (const char *attr = metaEl->Attribute("author"))
            author = attr;
        if (const char *attr = metaEl->Attribute("created"))
            createdDate_ = attr;
        if (const char *attr = metaEl->Attribute("modified"))
            modifiedDate_ = attr;

        if (TiXmlElement *commentEl = metaEl->FirstChildElement("comment"))
        {
            if (commentEl->GetText())
                comment = commentEl->GetText();
        }

        tags.clear();
        if (TiXmlElement *tagsEl = metaEl->FirstChildElement("tags"))
        {
            for (TiXmlElement *tagEl = tagsEl->FirstChildElement("tag"); tagEl;
                 tagEl = tagEl->NextSiblingElement("tag"))
            {
                if (tagEl->GetText())
                    tags.push_back(tagEl->GetText());
            }
        }
    }
}

bool GrooveboxProject::saveToFile(const fs::path &path)
{
    modifiedDate_ = getCurrentTimestamp();

    TiXmlDocument doc;
    toXML(doc);

    TiXmlPrinter printer;
    printer.SetIndent("  ");
    doc.Accept(&printer);
    std::string xmlStr = printer.Str();

    ProjectHeader header{};
    memcpy(header.tag, "SBOX", 4);
    header.version = mech::endian_write_int32LE(PROJECT_FORMAT_VERSION);
    header.xmlsize = mech::endian_write_int32LE(static_cast<uint32_t>(xmlStr.size()));
    header.numVoices = mech::endian_write_int32LE(NUM_VOICES);

    std::ofstream file(path, std::ios::binary);
    if (!file)
        return false;

    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(xmlStr.data(), xmlStr.size());

    return file.good();
}

bool GrooveboxProject::loadFromFile(const fs::path &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    ProjectHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    if (memcmp(header.tag, "SBOX", 4) != 0)
        return false;

    uint32_t version = mech::endian_read_int32LE(header.version);
    if (version > PROJECT_FORMAT_VERSION)
        return false;

    uint32_t xmlsize = mech::endian_read_int32LE(header.xmlsize);

    std::string xmlStr(xmlsize, '\0');
    file.read(xmlStr.data(), xmlsize);

    if (!file)
        return false;

    TiXmlDocument doc;
    doc.Parse(xmlStr.c_str());

    if (doc.Error())
        return false;

    reset();
    fromXML(doc);

    return true;
}

} // namespace SurgeBox
