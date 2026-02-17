/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include "GrooveboxProject.h"
#include "filesystem/import.h"

namespace SurgeBox
{

/**
 * VoicePreset handles saving/loading individual voice patches + patterns.
 * File format (.sbvp): binary header "SBVP" + XML containing a single <voice> element.
 * Reuses VoiceState::toXML/fromXML for the voice data.
 */
class VoicePreset
{
  public:
    static constexpr const char *FILE_EXTENSION = ".sbvp";

    // Save a voice state to a .sbvp file
    static bool saveToFile(const VoiceState &voice, int voiceIndex, const fs::path &path);

    // Load a voice state from a .sbvp file
    static bool loadFromFile(VoiceState &voice, const fs::path &path);

  private:
#pragma pack(push, 1)
    struct PresetHeader
    {
        char tag[4];          // "SBVP"
        uint32_t version;
        uint32_t xmlsize;
        uint32_t reserved[4];
    };
#pragma pack(pop)

    static constexpr uint32_t PRESET_FORMAT_VERSION = 1;
};

} // namespace SurgeBox
