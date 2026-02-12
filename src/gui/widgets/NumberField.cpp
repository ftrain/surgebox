/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "NumberField.h"
#include "gui/Theme.h"

namespace SurgeBox
{
namespace Widgets
{

NumberField::NumberField()
{
    setRepaintsOnMouseActivity(true);
    setWantsKeyboardFocus(true);
}

void NumberField::setRange(int minVal, int maxVal)
{
    minValue_ = minVal;
    maxValue_ = maxVal;
    value_ = juce::jlimit(minValue_, maxValue_, value_);
    repaint();
}

void NumberField::setValue(int v)
{
    v = juce::jlimit(minValue_, maxValue_, v);
    if (v != value_)
    {
        value_ = v;
        repaint();
    }
}

void NumberField::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    constexpr float corner = 2.0f;

    g.setColour(Theme::color(Theme::numberBackground));
    g.fillRoundedRectangle(b, corner);

    g.setColour(isHovered_ ? Theme::color(Theme::numberHoverBorder)
                           : Theme::color(Theme::numberBorder));
    g.drawRoundedRectangle(b, corner, 1.0f);

    g.setColour(Theme::color(Theme::numberText));
    g.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));

    auto text = juce::String(value_) + suffix_;
    g.drawText(text, getLocalBounds(), juce::Justification::centred);
}

void NumberField::mouseDown(const juce::MouseEvent& e)
{
    dragStartY_ = e.y;
    dragStartValue_ = value_;
}

void NumberField::mouseDrag(const juce::MouseEvent& e)
{
    int delta = (dragStartY_ - e.y) / 4;
    int newVal = juce::jlimit(minValue_, maxValue_, dragStartValue_ + delta);
    if (newVal != value_)
    {
        value_ = newVal;
        repaint();
        onValueChanged(value_);
    }
}

void NumberField::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    int delta = (wheel.deltaY > 0) ? 1 : (wheel.deltaY < 0) ? -1 : 0;
    int newVal = juce::jlimit(minValue_, maxValue_, value_ + delta);
    if (newVal != value_)
    {
        value_ = newVal;
        repaint();
        onValueChanged(value_);
    }
}

void NumberField::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    repaint();
}

void NumberField::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

} // namespace Widgets
} // namespace SurgeBox
