/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SelfDrawButton.h"
#include "gui/Theme.h"

namespace SurgeBox
{
namespace Widgets
{

SelfDrawButton::SelfDrawButton(const std::string& label) : label_(label)
{
    setRepaintsOnMouseActivity(true);
}

void SelfDrawButton::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    constexpr float corner = 2.0f;

    juce::Colour bg(Theme::buttonBackground);
    juce::Colour border(Theme::buttonBorder);
    juce::Colour text(Theme::buttonText);

    if (isDown_)
    {
        bg = Theme::color(Theme::buttonDownFill);
    }
    else if (isHovered_)
    {
        bg = Theme::color(Theme::buttonHoverFill);
        text = Theme::color(Theme::switchTextHover);
    }

    g.setColour(bg);
    g.fillRoundedRectangle(b, corner);

    g.setColour(border);
    g.drawRoundedRectangle(b, corner, 1.0f);

    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    g.drawText(label_, getLocalBounds(), juce::Justification::centred);
}

void SelfDrawButton::mouseDown(const juce::MouseEvent&)
{
    isDown_ = true;
    repaint();
}

void SelfDrawButton::mouseUp(const juce::MouseEvent& e)
{
    isDown_ = false;
    repaint();
    if (getLocalBounds().contains(e.getPosition()))
        onClick();
}

void SelfDrawButton::mouseEnter(const juce::MouseEvent&)
{
    isHovered_ = true;
    repaint();
}

void SelfDrawButton::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    isDown_ = false;
    repaint();
}

} // namespace Widgets
} // namespace SurgeBox
