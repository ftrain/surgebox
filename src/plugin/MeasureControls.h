/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Static helpers for measure operations.
 */

#pragma once

#include <juce_core/juce_core.h>

namespace SurgeBox
{

class PatternModel;

namespace MeasureControls
{

// Double the number of measures and clone the pattern
void doubleMeasures(PatternModel* model);

// Halve the number of measures (removes notes beyond new length)
void halveMeasures(PatternModel* model);

// Add one measure
void addMeasure(PatternModel* model);

// Remove one measure (removes notes beyond new length)
void subtractMeasure(PatternModel* model);

// Clear all notes from the pattern
void clearPattern(PatternModel* model);

// Get a text label for the current number of bars
juce::String getMeasuresLabel(PatternModel* model);

} // namespace MeasureControls
} // namespace SurgeBox
