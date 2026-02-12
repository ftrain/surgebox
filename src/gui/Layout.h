/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Layout constants for the SurgeBox UI.
 */

#pragma once

namespace SurgeBox
{
namespace Layout
{

// Command bar
constexpr int COMMAND_BAR_HEIGHT = 44;

// Piano keyboard
constexpr int PIANO_KEYBOARD_HEIGHT = 30;

// Divider between surge editor and piano roll
constexpr int DIVIDER_HEIGHT = 6;

// Minimum heights
constexpr int MIN_PIANO_ROLL_HEIGHT = 150;
constexpr int MIN_SYNTH_HEIGHT = 200;

// Note dimensions
constexpr int NOTE_WIDTH = 18;

// Default zoom
constexpr double DEFAULT_PIXELS_PER_BEAT = 60.0;
constexpr double MIN_PIXELS_PER_BEAT = 15.0;
constexpr double MAX_PIXELS_PER_BEAT = 120.0;

} // namespace Layout
} // namespace SurgeBox
