/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Centralized color theme for SurgeBox UI.
 */

#pragma once

#include <cstdint>
#include <juce_gui_basics/juce_gui_basics.h>

namespace SurgeBox
{
namespace Theme
{

// Base colors
constexpr uint32_t background = 0xff1a1a2e;
constexpr uint32_t backgroundDark = 0xff0a1a2a;
constexpr uint32_t backgroundMid = 0xff16213e;
constexpr uint32_t section = 0xff2a2a4a;

// Borders
constexpr uint32_t border = 0xff3a3a5a;
constexpr uint32_t borderLight = 0xff4a4a6a;
constexpr uint32_t borderBright = 0xff5a5a7a;
constexpr uint32_t separator = 0xff2a2a4a;

// Accent colors
constexpr uint32_t accent = 0xff00a0c0;
constexpr uint32_t accentBright = 0xff00d4ff;
constexpr uint32_t accentHover = 0xff00b0d0;
constexpr uint32_t accentDark = 0xff0080a0;

// Text colors
constexpr uint32_t text = 0xffa0b0c0;
constexpr uint32_t textBright = 0xffffffff;
constexpr uint32_t textDim = 0xff8090a0;
constexpr uint32_t textDark = 0xffe0e0e0;

// Widget colors - switches
constexpr uint32_t switchBackground = background;
constexpr uint32_t switchBorder = border;
constexpr uint32_t switchSeparator = separator;
constexpr uint32_t switchText = text;
constexpr uint32_t switchTextHover = textBright;
constexpr uint32_t switchOnFill = accent;
constexpr uint32_t switchOnText = textBright;
constexpr uint32_t switchHoverFill = 0xff2a3a4a;
constexpr uint32_t switchHoverOnFill = accentHover;

// Widget colors - buttons
constexpr uint32_t buttonBackground = 0xff1a2a3a;
constexpr uint32_t buttonBorder = 0xff4a5a6a;
constexpr uint32_t buttonText = text;
constexpr uint32_t buttonHoverFill = switchHoverFill;
constexpr uint32_t buttonDownFill = backgroundDark;
constexpr uint32_t buttonOnFill = accent;
constexpr uint32_t buttonOnText = textBright;

// Widget colors - number fields
constexpr uint32_t numberBackground = background;
constexpr uint32_t numberBorder = border;
constexpr uint32_t numberText = textBright;
constexpr uint32_t numberHoverBorder = accent;

// Piano roll colors
constexpr uint32_t pianoRollBackground = background;
constexpr uint32_t gridLine = 0xff2a2a4e;
constexpr uint32_t beatLine = 0xff3a3a5e;
constexpr uint32_t barLine = borderBright;
constexpr uint32_t noteColor = accentBright;
constexpr uint32_t noteSelected = 0xffff6b6b;
constexpr uint32_t playhead = textBright;
constexpr uint32_t stepCursor = 0xffff4444;
constexpr uint32_t boxSelect = 0x4400d4ff;

// Piano key colors
constexpr uint32_t whiteKey = 0xffe8e8e8;
constexpr uint32_t blackKey = 0xff3a3a4e;
constexpr uint32_t playingKey = accentBright;

// Status colors
constexpr uint32_t playActive = 0xff4caf50;
constexpr uint32_t stopActive = 0xfff44336;
constexpr uint32_t recordActive = 0xffff4444;

// Command bar gradient
constexpr uint32_t commandBarTop = 0xff1e2a3a;
constexpr uint32_t commandBarBottom = backgroundMid;
constexpr uint32_t divider = 0xff0f3460;

// Helper to create juce::Colour from uint32_t
inline juce::Colour color(uint32_t argb)
{
    return juce::Colour(argb);
}

} // namespace Theme
} // namespace SurgeBox
