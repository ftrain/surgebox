/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "ToggleButton.h"
#include "gui/Theme.h"

namespace SurgeBox
{
namespace Widgets
{

ToggleButton::ToggleButton(const std::string& label) : label_(label)
{
    setRepaintsOnMouseActivity(true);
}

void ToggleButton::setToggleState(bool state)
{
    if (state != isToggled_)
    {
        isToggled_ = state;
        repaint();
    }
}

void ToggleButton::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    constexpr float corner = 2.0f;

    juce::Colour bg = isToggled_ ? Theme::color(Theme::buttonOnFill)
                                 : Theme::color(Theme::buttonBackground);
    juce::Colour border(Theme::buttonBorder);
    juce::Colour text = isToggled_ ? Theme::color(Theme::buttonOnText)
                                   : Theme::color(Theme::buttonText);

    if (isHovered_ && !isToggled_)
    {
        bg = Theme::color(Theme::buttonHoverFill);
        text = Theme::color(Theme::switchTextHover);
    }
    else if (isHovered_ && isToggled_)
    {
        bg = Theme::color(Theme::switchHoverOnFill);
    }

    g.setColour(bg);
    g.fillRoundedRectangle(b, corner);

    g.setColour(border);
    g.drawRoundedRectangle(b, corner, 1.0f);

    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.drawText(label_, getLocalBounds(), juce::Justification::centred);
}

void ToggleButton::mouseDown(const juce::MouseEvent&)
{
    repaint();
}

void ToggleButton::mouseUp(const juce::MouseEvent& e)
{
    if (getLocalBounds().contains(e.getPosition()))
    {
        isToggled_ = !isToggled_;
        repaint();
        onToggle(isToggled_);
    }
}

void ToggleButton::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void ToggleButton::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    repaint();
}

} // namespace Widgets
} // namespace SurgeBox
