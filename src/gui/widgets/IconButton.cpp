/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "IconButton.h"
#include "gui/Theme.h"

namespace SurgeBox
{
namespace Widgets
{

IconButton::IconButton(Icon icon) : icon_(icon)
{
    setRepaintsOnMouseActivity(true);
}

void IconButton::setToggleState(bool state)
{
    if (state != isToggled_)
    {
        isToggled_ = state;
        repaint();
    }
}

void IconButton::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    constexpr float corner = 2.0f;

    juce::Colour bg = isToggled_ ? Theme::color(Theme::playActive)
                                 : Theme::color(Theme::buttonBackground);
    if (icon_ == Stop && isToggled_)
        bg = Theme::color(Theme::stopActive);

    if (isHovered_ && !isToggled_)
        bg = Theme::color(Theme::buttonHoverFill);

    g.setColour(bg);
    g.fillRoundedRectangle(b, corner);

    g.setColour(Theme::color(Theme::buttonBorder));
    g.drawRoundedRectangle(b, corner, 1.0f);

    auto center = getLocalBounds().getCentre();
    g.setColour(juce::Colours::white);

    switch (icon_)
    {
    case Play:
    {
        juce::Path triangle;
        triangle.addTriangle(center.x - 5.0f, center.y - 7.0f, center.x - 5.0f,
                             center.y + 7.0f, center.x + 7.0f, center.y);
        g.fillPath(triangle);
        break;
    }
    case Stop:
    {
        g.fillRect(center.x - 5, center.y - 5, 10, 10);
        break;
    }
    case Record:
    {
        g.setColour(isToggled_ ? juce::Colours::red : juce::Colours::white);
        g.fillEllipse(center.x - 5.0f, center.y - 5.0f, 10.0f, 10.0f);
        break;
    }
    }
}

void IconButton::mouseDown(const juce::MouseEvent&)
{
    repaint();
}

void IconButton::mouseUp(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getPosition()))
        onClick();
}

void IconButton::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void IconButton::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    repaint();
}

} // namespace Widgets
} // namespace SurgeBox
