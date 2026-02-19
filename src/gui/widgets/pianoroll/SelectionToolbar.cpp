/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SelectionToolbar.h"
#include "gui/Theme.h"

namespace SurgeBox
{
namespace PianoRoll
{

SelectionToolbar::SelectionToolbar()
{
    setAlwaysOnTop(true);
    setVisible(false);

    loopBtn_ = std::make_unique<Widgets::SelfDrawButton>("Loop");
    deleteBtn_ = std::make_unique<Widgets::SelfDrawButton>("Del");
    invertBtn_ = std::make_unique<Widgets::SelfDrawButton>("Inv");
    halveBtn_ = std::make_unique<Widgets::SelfDrawButton>("1/2x");
    doubleBtn_ = std::make_unique<Widgets::SelfDrawButton>("2x");
    cancelBtn_ = std::make_unique<Widgets::SelfDrawButton>("Esc");

    loopBtn_->onClick = [this]() { if (onLoop) onLoop(); };
    deleteBtn_->onClick = [this]() { if (onDelete) onDelete(); };
    invertBtn_->onClick = [this]() { if (onInvert) onInvert(); };
    halveBtn_->onClick = [this]() { if (onHalve) onHalve(); };
    doubleBtn_->onClick = [this]() { if (onDouble) onDouble(); };
    cancelBtn_->onClick = [this]() { if (onCancel) onCancel(); };

    addAndMakeVisible(*loopBtn_);
    addAndMakeVisible(*deleteBtn_);
    addAndMakeVisible(*invertBtn_);
    addAndMakeVisible(*halveBtn_);
    addAndMakeVisible(*doubleBtn_);
    addAndMakeVisible(*cancelBtn_);

    int totalWidth = kNumButtons * kButtonWidth + (kNumButtons + 1) * kPadding;
    int totalHeight = kButtonHeight + kPadding * 2;
    setSize(totalWidth, totalHeight);
}

void SelectionToolbar::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    g.setColour(Theme::color(Theme::section));
    g.fillRoundedRectangle(b, 4.0f);

    g.setColour(Theme::color(Theme::borderLight));
    g.drawRoundedRectangle(b, 4.0f, 1.0f);
}

void SelectionToolbar::resized()
{
    int x = kPadding;
    int y = kPadding;

    for (auto* btn : {loopBtn_.get(), deleteBtn_.get(), invertBtn_.get(),
                      halveBtn_.get(), doubleBtn_.get(), cancelBtn_.get()})
    {
        btn->setBounds(x, y, kButtonWidth, kButtonHeight);
        x += kButtonWidth + kPadding;
    }
}

void SelectionToolbar::showAt(juce::Point<int> position)
{
    // Offset slightly below and to the right of the cursor
    int x = position.x + 8;
    int y = position.y + 8;

    // Clamp to parent bounds
    if (auto* parent = getParentComponent())
    {
        auto parentBounds = parent->getLocalBounds();
        if (x + getWidth() > parentBounds.getRight())
            x = parentBounds.getRight() - getWidth();
        if (y + getHeight() > parentBounds.getBottom())
            y = position.y - getHeight() - 8;
        x = std::max(parentBounds.getX(), x);
        y = std::max(parentBounds.getY(), y);
    }

    setTopLeftPosition(x, y);
    setVisible(true);
    toFront(false);
}

void SelectionToolbar::dismiss()
{
    setVisible(false);
}

} // namespace PianoRoll
} // namespace SurgeBox
