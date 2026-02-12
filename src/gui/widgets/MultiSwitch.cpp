/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "MultiSwitch.h"
#include "gui/Theme.h"

namespace SurgeBox
{
namespace Widgets
{

MultiSwitch::MultiSwitch()
{
    setRepaintsOnMouseActivity(true);
    setWantsKeyboardFocus(true);
}

void MultiSwitch::setLabels(const std::vector<std::string>& labels)
{
    labels_ = labels;
    if (rows_ * columns_ != static_cast<int>(labels_.size()))
    {
        if (rows_ == 0 && columns_ == 0)
        {
            rows_ = 1;
            columns_ = static_cast<int>(labels_.size());
        }
    }
    repaint();
}

void MultiSwitch::setValue(int v)
{
    if (v != value_)
    {
        value_ = v;
        repaint();
    }
}

void MultiSwitch::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    constexpr float corner = 2.0f;

    g.setColour(Theme::color(Theme::switchBackground));
    g.fillRoundedRectangle(b, corner);

    g.setColour(Theme::color(Theme::switchBorder));
    g.drawRoundedRectangle(b, corner, 1.0f);

    if (rows_ <= 0 || columns_ <= 0 || labels_.empty())
        return;

    auto cw = (getWidth() - 2.0f) / columns_;
    auto ch = (getHeight() - 2.0f) / rows_;

    int idx = 0;
    for (int r = 0; r < rows_; ++r)
    {
        for (int c = 0; c < columns_; ++c)
        {
            if (idx >= static_cast<int>(labels_.size()))
                break;

            auto rc = juce::Rectangle<float>(c * cw + 1, r * ch + 1, cw, ch);
            auto fc = rc.reduced(1.5f);

            bool isOn = (idx == value_);
            bool isHo = isHovered_ && (hoverIndex_ == idx);

            juce::Colour fg(Theme::switchText);

            if (isOn && isHo)
            {
                g.setColour(Theme::color(Theme::switchHoverOnFill));
                g.fillRoundedRectangle(fc, 1.5f);
                fg = Theme::color(Theme::switchOnText);
            }
            else if (isOn)
            {
                g.setColour(Theme::color(Theme::switchOnFill));
                g.fillRoundedRectangle(fc, 1.5f);
                fg = Theme::color(Theme::switchOnText);
            }
            else if (isHo)
            {
                g.setColour(Theme::color(Theme::switchHoverFill));
                g.fillRoundedRectangle(fc, 1.5f);
                fg = Theme::color(Theme::switchTextHover);
            }

            g.setColour(fg);
            g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
            g.drawText(labels_[idx], rc, juce::Justification::centred);

            idx++;
        }
    }

    g.setColour(Theme::color(Theme::switchSeparator));
    if (rows_ == 1)
    {
        for (int c = 1; c < columns_; ++c)
        {
            g.drawLine(cw * c + 0.5f, 2.5f, cw * c + 0.5f, getHeight() - 2.5f, 1.0f);
        }
    }
    else if (columns_ == 1)
    {
        for (int r = 1; r < rows_; ++r)
        {
            g.drawLine(2.5f, ch * r + 0.5f, getWidth() - 2.5f, ch * r + 0.5f, 1.0f);
        }
    }
}

void MultiSwitch::mouseDown(const juce::MouseEvent& e)
{
    int sel = coordinateToIndex(e.x, e.y);
    if (sel >= 0 && sel < static_cast<int>(labels_.size()) && sel != value_)
    {
        value_ = sel;
        repaint();
        onValueChanged(value_);
    }
}

void MultiSwitch::mouseMove(const juce::MouseEvent& e)
{
    int oldHover = hoverIndex_;
    hoverIndex_ = coordinateToIndex(e.x, e.y);
    isHovered_ = true;
    if (oldHover != hoverIndex_)
        repaint();
}

void MultiSwitch::mouseEnter(const juce::MouseEvent& e)
{
    isHovered_ = true;
    hoverIndex_ = coordinateToIndex(e.x, e.y);
    repaint();
}

void MultiSwitch::mouseExit(const juce::MouseEvent&)
{
    isHovered_ = false;
    repaint();
}

int MultiSwitch::coordinateToIndex(int x, int y) const
{
    if (rows_ <= 0 || columns_ <= 0)
        return -1;

    auto cw = getWidth() / columns_;
    auto ch = getHeight() / rows_;

    int c = x / cw;
    int r = y / ch;

    c = juce::jlimit(0, columns_ - 1, c);
    r = juce::jlimit(0, rows_ - 1, r);

    return r * columns_ + c;
}

} // namespace Widgets
} // namespace SurgeBox
