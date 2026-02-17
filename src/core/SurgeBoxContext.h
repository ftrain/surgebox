/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace SurgeBox
{

class SurgeBoxEngine;
class GrooveboxProject;
class ProjectModel;
class MidiMappingEngine;

/**
 * Lightweight context object holding references to all major subsystems.
 * UI components can take a SurgeBoxContext instead of an engine reference,
 * making dependencies explicit and enabling gradual decoupling.
 */
struct SurgeBoxContext
{
    SurgeBoxEngine &engine;
    GrooveboxProject &project;
    juce::UndoManager &undoManager;
    ProjectModel &projectModel;
    MidiMappingEngine &midiMappingEngine;
};

} // namespace SurgeBox
