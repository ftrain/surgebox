/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "VoicePreset.h"
#include "tinyxml/tinyxml.h"
#include "sst/basic-blocks/mechanics/endian-ops.h"

#include <cstring>
#include <fstream>

namespace mech = sst::basic_blocks::mechanics;

namespace SurgeBox
{

bool VoicePreset::saveToFile(const VoiceState &voice, int voiceIndex, const fs::path &path)
{
    TiXmlDocument doc;
    auto *decl = new TiXmlDeclaration("1.0", "UTF-8", "");
    doc.LinkEndChild(decl);

    TiXmlElement root("surgebox-voice-preset");
    root.SetAttribute("version", PRESET_FORMAT_VERSION);
    voice.toXML(&root, voiceIndex);
    doc.InsertEndChild(root);

    TiXmlPrinter printer;
    printer.SetIndent("  ");
    doc.Accept(&printer);
    std::string xmlStr = printer.Str();

    PresetHeader header{};
    memcpy(header.tag, "SBVP", 4);
    header.version = mech::endian_write_int32LE(PRESET_FORMAT_VERSION);
    header.xmlsize = mech::endian_write_int32LE(static_cast<uint32_t>(xmlStr.size()));

    std::ofstream file(path, std::ios::binary);
    if (!file)
        return false;

    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(xmlStr.data(), xmlStr.size());

    return file.good();
}

bool VoicePreset::loadFromFile(VoiceState &voice, const fs::path &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    PresetHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    if (memcmp(header.tag, "SBVP", 4) != 0)
        return false;

    uint32_t version = mech::endian_read_int32LE(header.version);
    if (version > PRESET_FORMAT_VERSION)
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

    TiXmlElement *root = doc.FirstChildElement("surgebox-voice-preset");
    if (!root)
        return false;

    TiXmlElement *voiceEl = root->FirstChildElement("voice");
    if (!voiceEl)
        return false;

    voice.fromXML(voiceEl, 0);
    return true;
}

} // namespace SurgeBox
