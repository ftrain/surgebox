/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include "PianoRollLayer.h"

namespace SurgeBox
{

class NoteLayer : public PianoRollLayer
{
  public:
    NoteLayer() = default;
    void paint(juce::Graphics& g) override;
    void repaintNote(int noteIndex);

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteLayer)
};

} // namespace SurgeBox
