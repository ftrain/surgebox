/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "PianoRollRenderer.h"
#include "core/PatternModel.h"
#include "core/MusicTheory.h"
#include "gui/Theme.h"
#include <algorithm>

namespace SurgeBox
{
namespace PianoRoll
{

void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& area,
              PatternModel* model, const RenderParams& params)
{
    if (!model)
        return;

    int numNotes = params.visiblePitches ? static_cast<int>(params.visiblePitches->size()) : 0;
    double totalBeats = model->lengthInBeats();

    int patternHeight = static_cast<int>(totalBeats * params.pixelsPerBeat);

    // Draw alternating beat shading
    for (int beat = 0; beat < static_cast<int>(totalBeats); beat++)
    {
        int y1 = area.getY() + static_cast<int>(beat * params.pixelsPerBeat);
        int y2 = area.getY() + static_cast<int>((beat + 1) * params.pixelsPerBeat);

        if (beat % 2 == 1)
        {
            g.setColour(juce::Colour(0x08ffffff));
            g.fillRect(area.getX(), y1, area.getWidth(), y2 - y1);
        }
    }

    // Draw black key column shading
    if (params.visiblePitches)
    {
        for (int i = 0; i < numNotes; i++)
        {
            int note = (*params.visiblePitches)[i];
            int noteInOctave = note % 12;
            bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                               noteInOctave == 8 || noteInOctave == 10);

            if (isBlackKey)
            {
                int x = area.getX() + (i * params.noteWidth);
                g.setColour(juce::Colour(0x15000000));
                g.fillRect(x, area.getY(), params.noteWidth, patternHeight);
            }
        }
    }

    // Draw chord shading — highlight chord tone columns with a tinted overlay
    if (params.chordShadings && params.visiblePitches && !params.chordShadings->empty())
    {
        for (const auto& cs : *params.chordShadings)
        {
            int y1 = area.getY() + static_cast<int>(cs.startBeat * params.pixelsPerBeat);
            int y2 = area.getY() + static_cast<int>(cs.endBeat * params.pixelsPerBeat);
            int h = y2 - y1;
            if (h <= 0) continue;

            // Shade chord tone columns
            for (int i = 0; i < numNotes; i++)
            {
                int pitch = (*params.visiblePitches)[i];
                int pc = pitch % 12;
                bool isChordTone = std::find(cs.chordPitchClasses.begin(),
                                             cs.chordPitchClasses.end(), pc) !=
                                   cs.chordPitchClasses.end();
                if (isChordTone)
                {
                    int x = area.getX() + (i * params.noteWidth);
                    g.setColour(juce::Colour(0x18448866));  // Subtle green tint
                    g.fillRect(x, y1, params.noteWidth, h);
                }
            }

            // Draw chord name label at the left edge
            if (!cs.chordName.empty())
            {
                g.setColour(juce::Colour(0xaa88ccaa));
                g.setFont(juce::Font(11.0f).boldened());
                g.drawText(cs.chordName, area.getX() + 2, y1 + 1, 60, 14,
                           juce::Justification::topLeft);
            }
        }
    }

    // Draw vertical lines (pitch columns)
    for (int i = 0; i <= numNotes; i++)
    {
        int x = area.getX() + (i * params.noteWidth);
        int note = (i < numNotes && params.visiblePitches)
                       ? (*params.visiblePitches)[i]
                       : (params.visiblePitches && !params.visiblePitches->empty()
                              ? params.visiblePitches->back() + 1
                              : 0);
        int noteInOctave = note % 12;

        bool isC = (noteInOctave == 0);
        g.setColour(isC ? Theme::color(Theme::barLine) : Theme::color(Theme::gridLine));
        g.drawVerticalLine(x, static_cast<float>(area.getY()),
                           static_cast<float>(area.getY() + patternHeight));
    }

    // Draw horizontal lines (time/beats)
    for (double beat = 0; beat <= totalBeats; beat += params.gridSize)
    {
        int y = area.getY() + static_cast<int>(beat * params.pixelsPerBeat);

        bool isBar = (static_cast<int>(beat) % 4 == 0 && beat == static_cast<int>(beat));
        bool isBeat = (beat == static_cast<int>(beat));

        if (isBar)
        {
            g.setColour(Theme::color(Theme::barLine));
            g.fillRect(area.getX(), y - 1, area.getWidth(), 3);
        }
        else if (isBeat)
        {
            g.setColour(Theme::color(Theme::beatLine));
            g.fillRect(area.getX(), y, area.getWidth(), 2);
        }
        else
        {
            g.setColour(Theme::color(Theme::gridLine));
            g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                                 static_cast<float>(area.getRight()));
        }
    }

    // Draw pattern boundary
    int patternEndY = area.getY() + patternHeight;
    g.setColour(Theme::color(Theme::noteColor).withAlpha(0.5f));
    g.fillRect(area.getX(), patternEndY - 1, area.getWidth(), 2);
}

void drawNotes(juce::Graphics& g, const juce::Rectangle<int>& area,
               PatternModel* model, const RenderParams& params)
{
    if (!model)
        return;

    for (int i = 0; i < model->getNumNotes(); ++i)
    {
        auto noteRect = noteToScreen(i, area, model, params);
        if (noteRect.isEmpty() || !noteRect.intersects(area))
            continue;

        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(i, startBeat, duration, pitch, velocity);

        bool selected = params.selectedNotes && params.selectedNotes->count(i) > 0;
        bool inScale = params.isChromatic ||
                       MusicTheory::isPitchInScale(pitch, params.scaleRoot,
                                                   params.isChromatic ? ScaleType::Chromatic : ScaleType::Major);

        if (!inScale && !params.isChromatic)
        {
            g.setColour((selected ? Theme::color(Theme::noteSelected) : Theme::color(Theme::noteColor))
                            .withAlpha(0.4f));
        }
        else
        {
            g.setColour(selected ? Theme::color(Theme::noteSelected) : Theme::color(Theme::noteColor));
        }
        g.fillRoundedRectangle(noteRect.toFloat(), 3.0f);

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(noteRect.toFloat(), 3.0f, 1.0f);

        float velAlpha = 1.0f - (static_cast<float>(velocity) / 127.0f) * 0.5f;
        g.setColour(juce::Colours::black.withAlpha(velAlpha * 0.3f));
        g.fillRoundedRectangle(noteRect.toFloat(), 3.0f);
    }
}

void drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& area,
                  double playheadBeats, double pixelsPerBeat)
{
    int y = area.getY() + static_cast<int>(playheadBeats * pixelsPerBeat);

    if (y >= area.getY() && y <= area.getBottom())
    {
        g.setColour(Theme::color(Theme::playhead));
        g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                             static_cast<float>(area.getRight()));

        juce::Path triangle;
        triangle.addTriangle(static_cast<float>(area.getX()), static_cast<float>(y - 5),
                             static_cast<float>(area.getX()), static_cast<float>(y + 5),
                             static_cast<float>(area.getX() + 8), static_cast<float>(y));
        g.fillPath(triangle);
    }
}

void drawStepCursor(juce::Graphics& g, const juce::Rectangle<int>& area,
                    double stepPosition, double pixelsPerBeat)
{
    int y = area.getY() + static_cast<int>(stepPosition * pixelsPerBeat);

    if (y >= area.getY() && y <= area.getBottom())
    {
        g.setColour(Theme::color(Theme::stepCursor));
        g.drawHorizontalLine(y, static_cast<float>(area.getX()),
                             static_cast<float>(area.getRight()));

        g.fillRect(area.getRight() - 8, y - 4, 8, 8);

        g.setFont(10.0f);
        g.drawText("STEP", area.getRight() - 40, y - 15, 30, 12, juce::Justification::centred);
    }
}

void drawBoxSelection(juce::Graphics& g, const juce::Point<int>& start,
                      const juce::Point<int>& end, const juce::Rectangle<int>& area,
                      const RenderParams& params)
{
    auto selRect = juce::Rectangle<int>(start, end);

    // Compute which grid cells are inside the selection
    double beatStart = (std::min(start.y, end.y) - area.getY()) / params.pixelsPerBeat;
    double beatEnd = (std::max(start.y, end.y) - area.getY()) / params.pixelsPerBeat;
    int colStart = (std::min(start.x, end.x) - area.getX()) / params.noteWidth;
    int colEnd = (std::max(start.x, end.x) - area.getX()) / params.noteWidth;

    double gridBeatStart = std::floor(beatStart / params.gridSize) * params.gridSize;
    double gridBeatEnd = std::ceil(beatEnd / params.gridSize) * params.gridSize;

    int numCols = params.visiblePitches ? static_cast<int>(params.visiblePitches->size()) : 0;
    colStart = std::max(0, colStart);
    colEnd = std::min(colEnd, numCols - 1);

    // Highlight each grid cell within the selection
    auto cellColor = Theme::color(Theme::noteColor).withAlpha(0.15f);
    g.setColour(cellColor);

    for (double beat = gridBeatStart; beat < gridBeatEnd; beat += params.gridSize)
    {
        int cellY = area.getY() + static_cast<int>(beat * params.pixelsPerBeat);
        int cellH = static_cast<int>(params.gridSize * params.pixelsPerBeat);

        for (int col = colStart; col <= colEnd; ++col)
        {
            int cellX = area.getX() + col * params.noteWidth;
            g.fillRect(cellX + 1, cellY, params.noteWidth - 2, cellH);
        }
    }

    // Draw selection border
    g.setColour(Theme::color(Theme::noteColor).withAlpha(0.6f));
    g.drawRect(selRect, 1);
}

std::pair<double, int> screenToNote(juce::Point<int> pos, const juce::Rectangle<int>& area,
                                    const RenderParams& params)
{
    double beat = (pos.y - area.getY()) / params.pixelsPerBeat;
    int column = (pos.x - area.getX()) / params.noteWidth;

    int pitch = columnToPitch(column, params);
    if (pitch < 0)
    {
        pitch = std::clamp(params.lowestNote + column, params.lowestNote, params.highestNote - 1);
    }

    return {beat, pitch};
}

juce::Rectangle<int> noteToScreen(int noteIndex, const juce::Rectangle<int>& area,
                                  PatternModel* model, const RenderParams& params)
{
    if (!model || noteIndex < 0 || noteIndex >= model->getNumNotes())
        return {};

    double startBeat, duration;
    int pitch, velocity;
    model->getNoteAt(noteIndex, startBeat, duration, pitch, velocity);

    int noteCol = pitchToColumn(pitch, params);
    if (noteCol < 0)
    {
        noteCol = pitch - params.lowestNote;
    }

    int x = area.getX() + (noteCol * params.noteWidth);
    int y = area.getY() + static_cast<int>(startBeat * params.pixelsPerBeat);
    int h = static_cast<int>(duration * params.pixelsPerBeat);

    return juce::Rectangle<int>(x + 1, y, params.noteWidth - 2, std::max(6, h));
}

int pitchToColumn(int pitch, const RenderParams& params)
{
    if (params.isChromatic)
        return pitch - params.lowestNote;

    if (params.visiblePitches)
    {
        auto it = std::find(params.visiblePitches->begin(), params.visiblePitches->end(), pitch);
        if (it != params.visiblePitches->end())
            return static_cast<int>(std::distance(params.visiblePitches->begin(), it));
    }
    return -1;
}

int columnToPitch(int column, const RenderParams& params)
{
    if (!params.visiblePitches || column < 0 ||
        column >= static_cast<int>(params.visiblePitches->size()))
        return -1;
    return (*params.visiblePitches)[column];
}

void drawGhostNotes(juce::Graphics& g, const juce::Rectangle<int>& area,
                    const std::vector<GhostNote>& ghosts, const RenderParams& params)
{
    for (const auto& ghost : ghosts)
    {
        int noteCol = pitchToColumn(ghost.pitch, params);
        if (noteCol < 0)
            noteCol = ghost.pitch - params.lowestNote;

        int x = area.getX() + (noteCol * params.noteWidth) + 1;
        int y = area.getY() + static_cast<int>(ghost.beat * params.pixelsPerBeat);
        int h = std::max(6, static_cast<int>(ghost.duration * params.pixelsPerBeat));
        int w = params.noteWidth - 2;

        auto rect = juce::Rectangle<int>(x, y, w, h).toFloat();

        g.setColour(Theme::color(Theme::noteColor).withAlpha(0.25f));
        g.fillRoundedRectangle(rect, 3.0f);

        g.setColour(Theme::color(Theme::noteColor).withAlpha(0.15f));
        g.drawRoundedRectangle(rect, 3.0f, 1.0f);
    }
}

void drawLoopRegion(juce::Graphics& g, const juce::Rectangle<int>& area,
                    double loopStartBeat, double loopEndBeat,
                    int minPitch, int maxPitch,
                    double patternLengthBeats, const RenderParams& params,
                    bool selected)
{
    double loopLen = loopEndBeat - loopStartBeat;
    if (loopLen <= 0.0)
        return;

    // Compute horizontal bounds by finding the first and last visible pitch
    // within the loop's pitch range (handles scale changes gracefully)
    int minCol = -1, maxCol = -1;
    if (params.visiblePitches)
    {
        for (int i = 0; i < static_cast<int>(params.visiblePitches->size()); ++i)
        {
            int p = (*params.visiblePitches)[i];
            if (p >= minPitch && p <= maxPitch)
            {
                if (minCol < 0) minCol = i;
                maxCol = i;
            }
        }
    }
    if (minCol < 0 || maxCol < 0)
        return;

    int x = area.getX() + minCol * params.noteWidth;
    int w = (maxCol - minCol + 1) * params.noteWidth;

    // Draw source region box
    int sourceY = area.getY() + static_cast<int>(loopStartBeat * params.pixelsPerBeat);
    int sourceH = static_cast<int>(loopLen * params.pixelsPerBeat);

    float fillAlpha = selected ? 0.25f : 0.15f;
    float borderAlpha = selected ? 0.9f : 0.6f;
    int borderWidth = selected ? 3 : 2;

    g.setColour(Theme::color(Theme::accent).withAlpha(fillAlpha));
    g.fillRect(x, sourceY, w, sourceH);
    g.setColour(Theme::color(Theme::accent).withAlpha(borderAlpha));
    g.drawRect(x, sourceY, w, sourceH, borderWidth);

    // Draw edge handles when selected (fully inside the region)
    if (selected)
    {
        auto handleColor = Theme::color(Theme::accent).withAlpha(0.8f);
        g.setColour(handleColor);
        int hw = 12, hh = 5;
        // Top edge center (inside)
        g.fillRect(x + w / 2 - hw / 2, sourceY + borderWidth, hw, hh);
        // Bottom edge center (inside)
        g.fillRect(x + w / 2 - hw / 2, sourceY + sourceH - borderWidth - hh, hw, hh);
        // Left edge center (inside)
        g.fillRect(x + borderWidth, sourceY + sourceH / 2 - hw / 2, hh, hw);
        // Right edge center (inside)
        g.fillRect(x + w - borderWidth - hh, sourceY + sourceH / 2 - hw / 2, hh, hw);
    }

    // Draw repetition boxes
    for (double offset = loopLen; loopStartBeat + offset < patternLengthBeats; offset += loopLen)
    {
        double repStart = loopStartBeat + offset;
        double repEnd = std::min(repStart + loopLen, patternLengthBeats);

        int repY = area.getY() + static_cast<int>(repStart * params.pixelsPerBeat);
        int repH = static_cast<int>((repEnd - repStart) * params.pixelsPerBeat);

        g.setColour(Theme::color(Theme::accent).withAlpha(0.08f));
        g.fillRect(x, repY, w, repH);
        g.setColour(Theme::color(Theme::accent).withAlpha(0.3f));
        g.drawRect(x, repY, w, repH, 1);
    }
}

} // namespace PianoRoll
} // namespace SurgeBox
