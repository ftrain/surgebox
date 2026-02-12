/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Music theory utilities for scales and pitch quantization.
 */

#pragma once

#include <vector>

namespace SurgeBox
{

enum class ScaleType
{
    Chromatic,
    Major,
    NaturalMinor,
    HarmonicMinor,
    MelodicMinor,
    Pentatonic,
    PentatonicMinor,
    Blues,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Locrian
};

namespace MusicTheory
{

// Get the intervals (semitones from root) for a scale type
const std::vector<int>& getScaleIntervals(ScaleType type);

// Check if a pitch is in a given scale
bool isPitchInScale(int pitch, int root, ScaleType type);

// Find the nearest pitch that is in the scale
int findNearestScalePitch(int pitch, int root, ScaleType type);

// Build a list of pitches in a scale within a range
std::vector<int> buildScalePitches(int lowestNote, int highestNote, int root, ScaleType type);

// Convert pitch to column index given visible pitches
int pitchToColumn(int pitch, const std::vector<int>& visiblePitches, int lowestNote, ScaleType type);

// Convert column index to pitch given visible pitches
int columnToPitch(int column, const std::vector<int>& visiblePitches);

} // namespace MusicTheory
} // namespace SurgeBox
