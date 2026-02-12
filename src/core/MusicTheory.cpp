/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "MusicTheory.h"
#include <algorithm>

namespace SurgeBox
{
namespace MusicTheory
{

const std::vector<int>& getScaleIntervals(ScaleType type)
{
    static const std::vector<int> chromatic = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    static const std::vector<int> major = {0, 2, 4, 5, 7, 9, 11};
    static const std::vector<int> naturalMinor = {0, 2, 3, 5, 7, 8, 10};
    static const std::vector<int> harmonicMinor = {0, 2, 3, 5, 7, 8, 11};
    static const std::vector<int> melodicMinor = {0, 2, 3, 5, 7, 9, 11};
    static const std::vector<int> pentatonic = {0, 2, 4, 7, 9};
    static const std::vector<int> pentatonicMinor = {0, 3, 5, 7, 10};
    static const std::vector<int> blues = {0, 3, 5, 6, 7, 10};
    static const std::vector<int> dorian = {0, 2, 3, 5, 7, 9, 10};
    static const std::vector<int> phrygian = {0, 1, 3, 5, 7, 8, 10};
    static const std::vector<int> lydian = {0, 2, 4, 6, 7, 9, 11};
    static const std::vector<int> mixolydian = {0, 2, 4, 5, 7, 9, 10};
    static const std::vector<int> locrian = {0, 1, 3, 5, 6, 8, 10};

    switch (type)
    {
        case ScaleType::Major: return major;
        case ScaleType::NaturalMinor: return naturalMinor;
        case ScaleType::HarmonicMinor: return harmonicMinor;
        case ScaleType::MelodicMinor: return melodicMinor;
        case ScaleType::Pentatonic: return pentatonic;
        case ScaleType::PentatonicMinor: return pentatonicMinor;
        case ScaleType::Blues: return blues;
        case ScaleType::Dorian: return dorian;
        case ScaleType::Phrygian: return phrygian;
        case ScaleType::Lydian: return lydian;
        case ScaleType::Mixolydian: return mixolydian;
        case ScaleType::Locrian: return locrian;
        default: return chromatic;
    }
}

bool isPitchInScale(int pitch, int root, ScaleType type)
{
    if (type == ScaleType::Chromatic)
        return true;

    int noteInOctave = ((pitch - root) % 12 + 12) % 12;
    const auto& intervals = getScaleIntervals(type);
    return std::find(intervals.begin(), intervals.end(), noteInOctave) != intervals.end();
}

int findNearestScalePitch(int pitch, int root, ScaleType type)
{
    if (type == ScaleType::Chromatic)
        return pitch;

    const auto& intervals = getScaleIntervals(type);

    // Check if already in scale
    int noteInOctave = ((pitch - root) % 12 + 12) % 12;
    if (std::find(intervals.begin(), intervals.end(), noteInOctave) != intervals.end())
        return pitch;

    // Find nearest scale pitch by checking up and down
    for (int offset = 1; offset <= 6; ++offset)
    {
        // Check pitch below
        int lowerPitch = pitch - offset;
        int lowerNoteInOctave = ((lowerPitch - root) % 12 + 12) % 12;
        bool lowerInScale = std::find(intervals.begin(), intervals.end(), lowerNoteInOctave) != intervals.end();

        // Check pitch above
        int upperPitch = pitch + offset;
        int upperNoteInOctave = ((upperPitch - root) % 12 + 12) % 12;
        bool upperInScale = std::find(intervals.begin(), intervals.end(), upperNoteInOctave) != intervals.end();

        if (lowerInScale && upperInScale)
        {
            // Both are in scale at same distance - prefer lower
            return lowerPitch;
        }
        else if (lowerInScale)
        {
            return lowerPitch;
        }
        else if (upperInScale)
        {
            return upperPitch;
        }
    }

    return pitch; // Fallback
}

std::vector<int> buildScalePitches(int lowestNote, int highestNote, int root, ScaleType type)
{
    std::vector<int> pitches;

    if (type == ScaleType::Chromatic)
    {
        for (int pitch = lowestNote; pitch < highestNote; ++pitch)
            pitches.push_back(pitch);
    }
    else
    {
        for (int pitch = lowestNote; pitch < highestNote; ++pitch)
        {
            if (isPitchInScale(pitch, root, type))
                pitches.push_back(pitch);
        }
    }

    return pitches;
}

int pitchToColumn(int pitch, const std::vector<int>& visiblePitches, int lowestNote, ScaleType type)
{
    if (type == ScaleType::Chromatic)
        return pitch - lowestNote;

    auto it = std::find(visiblePitches.begin(), visiblePitches.end(), pitch);
    if (it != visiblePitches.end())
        return static_cast<int>(std::distance(visiblePitches.begin(), it));
    return -1;  // Not in scale
}

int columnToPitch(int column, const std::vector<int>& visiblePitches)
{
    if (column < 0 || column >= static_cast<int>(visiblePitches.size()))
        return -1;
    return visiblePitches[column];
}

} // namespace MusicTheory
} // namespace SurgeBox
