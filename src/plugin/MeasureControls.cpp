/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "MeasureControls.h"
#include "core/PatternModel.h"
#include <vector>
#include <tuple>

namespace SurgeBox
{
namespace MeasureControls
{

void doubleMeasures(PatternModel* model)
{
    if (!model)
        return;

    int currentBars = model->getBars();
    if (currentBars >= 64)
        return;

    model->beginTransaction("Double Measures");

    double currentLength = currentBars * 4.0;
    int numNotes = model->getNumNotes();

    std::vector<std::tuple<double, double, int, int>> notesToClone;
    for (int i = 0; i < numNotes; ++i)
    {
        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(i, startBeat, duration, pitch, velocity);
        notesToClone.emplace_back(startBeat + currentLength, duration, pitch, velocity);
    }

    int newBars = currentBars * 2;
    model->setBars(newBars);

    for (const auto& [start, dur, pitch, vel] : notesToClone)
    {
        model->addNote(start, dur, pitch, vel);
    }
}

void halveMeasures(PatternModel* model)
{
    if (!model)
        return;

    int currentBars = model->getBars();
    if (currentBars <= 1)
        return;

    model->beginTransaction("Halve Measures");

    int newBars = currentBars / 2;
    double newLength = newBars * 4.0;

    for (int i = model->getNumNotes() - 1; i >= 0; --i)
    {
        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(i, startBeat, duration, pitch, velocity);
        if (startBeat >= newLength)
            model->removeNote(i);
    }

    model->setBars(newBars);
}

void addMeasure(PatternModel* model)
{
    if (!model)
        return;

    int currentBars = model->getBars();
    if (currentBars >= 64)
        return;

    model->beginTransaction("Add Measure");
    model->setBars(currentBars + 1);
}

void subtractMeasure(PatternModel* model)
{
    if (!model)
        return;

    int currentBars = model->getBars();
    if (currentBars <= 1)
        return;

    model->beginTransaction("Remove Measure");

    int newBars = currentBars - 1;
    double newLength = newBars * 4.0;

    for (int i = model->getNumNotes() - 1; i >= 0; --i)
    {
        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(i, startBeat, duration, pitch, velocity);
        if (startBeat >= newLength)
            model->removeNote(i);
    }

    model->setBars(newBars);
}

void clearPattern(PatternModel* model)
{
    if (!model)
        return;

    model->beginTransaction("Clear Pattern");
    model->clear();
}

juce::String getMeasuresLabel(PatternModel* model)
{
    if (!model)
        return "0 bars";

    int bars = model->getBars();
    return juce::String(bars) + (bars == 1 ? " bar" : " bars");
}

} // namespace MeasureControls
} // namespace SurgeBox
