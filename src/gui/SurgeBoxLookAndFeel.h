/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace SurgeBox
{

class SurgeBoxLookAndFeel : public juce::LookAndFeel_V4
{
  public:
    SurgeBoxLookAndFeel()
    {
        // Dark theme colors matching Surge XT
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1a1a2e));

        // Buttons
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a5a));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00a0c0));
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);

        // ComboBox
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a4a));
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff4a4a6a));
        setColour(juce::ComboBox::arrowColourId, juce::Colours::white);

        // Labels
        setColour(juce::Label::textColourId, juce::Colours::white);

        // Sliders
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a4a));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff00d4ff));
        setColour(juce::Slider::trackColourId, juce::Colour(0xff4a4a6a));
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2a2a4a));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff4a4a6a));

        // Scrollbars
        setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff5a5a7a));
        setColour(juce::ScrollBar::trackColourId, juce::Colour(0xff2a2a4a));

        // PopupMenu
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff2a2a4a));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff00a0c0));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        // TextEditor
        setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a4a));
        setColour(juce::TextEditor::textColourId, juce::Colours::white);
        setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff4a4a6a));
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff00d4ff));
    }

    void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                              const juce::Colour &backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto corner = 2.0f;

        // Surge-style button drawing
        auto bgColor = juce::Colour(0xff2a3a4a);
        auto borderColor = juce::Colour(0xff4a5a6a);
        auto textBg = juce::Colour(0xff1a2a3a);

        bool isOn = button.getToggleState();

        if (isOn)
        {
            bgColor = juce::Colour(0xff00a0c0);
            borderColor = juce::Colour(0xff00c0e0);
            textBg = juce::Colour(0xff0080a0);
        }
        else if (shouldDrawButtonAsDown)
        {
            bgColor = bgColor.darker(0.3f);
            textBg = textBg.darker(0.3f);
        }
        else if (shouldDrawButtonAsHighlighted)
        {
            bgColor = bgColor.brighter(0.1f);
            borderColor = borderColor.brighter(0.2f);
        }

        // Draw background
        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds, corner);

        // Draw inner area (slightly darker)
        auto innerBounds = bounds.reduced(1.0f);
        g.setColour(textBg);
        g.fillRoundedRectangle(innerBounds, corner - 0.5f);

        // Draw border
        g.setColour(borderColor);
        g.drawRoundedRectangle(bounds, corner, 1.0f);
    }

    void drawButtonText(juce::Graphics &g, juce::TextButton &button,
                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto font = juce::Font(juce::FontOptions().withHeight(12.0f));
        g.setFont(font);

        auto textColor = button.getToggleState() ? juce::Colours::white : juce::Colour(0xffa0b0c0);
        if (shouldDrawButtonAsHighlighted && !button.getToggleState())
            textColor = juce::Colours::white;

        g.setColour(textColor);

        auto bounds = button.getLocalBounds();
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred, false);
    }

    void drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox &box) override
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);
        auto corner = 2.0f;

        // Surge-style combo box
        auto bgColor = juce::Colour(0xff1a2a3a);
        auto borderColor = juce::Colour(0xff4a5a6a);

        if (isButtonDown)
            bgColor = bgColor.darker(0.2f);

        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds, corner);

        g.setColour(borderColor);
        g.drawRoundedRectangle(bounds, corner, 1.0f);

        // Draw arrow
        juce::Path arrow;
        auto arrowX = width - 15.0f;
        auto arrowY = height * 0.5f;
        arrow.addTriangle(arrowX - 3.0f, arrowY - 2.0f,
                          arrowX + 3.0f, arrowY - 2.0f,
                          arrowX, arrowY + 2.0f);
        g.setColour(juce::Colour(0xff8090a0));
        g.fillPath(arrow);
    }

    void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider &slider) override
    {
        // Surge-style slider
        auto trackHeight = 4.0f;
        auto bounds = juce::Rectangle<float>(x, y + height * 0.5f - trackHeight * 0.5f,
                                              width, trackHeight);

        // Track background
        g.setColour(juce::Colour(0xff1a2a3a));
        g.fillRoundedRectangle(bounds, 2.0f);

        // Track border
        g.setColour(juce::Colour(0xff3a4a5a));
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);

        // Value fill
        auto fillWidth = sliderPos - x;
        if (fillWidth > 0)
        {
            auto fillBounds = bounds.withWidth(fillWidth);
            g.setColour(juce::Colour(0xff00a0c0));
            g.fillRoundedRectangle(fillBounds, 2.0f);
        }

        // Thumb
        auto thumbWidth = 10.0f;
        auto thumbHeight = 16.0f;
        auto thumbX = sliderPos - thumbWidth * 0.5f;
        auto thumbY = y + height * 0.5f - thumbHeight * 0.5f;
        auto thumbBounds = juce::Rectangle<float>(thumbX, thumbY, thumbWidth, thumbHeight);

        g.setColour(juce::Colour(0xff4a6a8a));
        g.fillRoundedRectangle(thumbBounds, 2.0f);
        g.setColour(juce::Colour(0xff6a8aaa));
        g.drawRoundedRectangle(thumbBounds, 2.0f, 1.0f);
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SurgeBoxLookAndFeel)
};

} // namespace SurgeBox
