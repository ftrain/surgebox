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
        auto baseColour = backgroundColour;

        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.1f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(baseColour.brighter(0.2f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    void drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox &box) override
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);

        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw arrow
        juce::Path arrow;
        auto arrowX = width - 20.0f;
        auto arrowY = height * 0.5f;
        arrow.addTriangle(arrowX, arrowY - 3.0f,
                          arrowX + 6.0f, arrowY - 3.0f,
                          arrowX + 3.0f, arrowY + 3.0f);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId));
        g.fillPath(arrow);
    }

    void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider &slider) override
    {
        auto trackWidth = 4.0f;

        juce::Point<float> startPoint(slider.isHorizontal() ? x : x + width * 0.5f,
                                      slider.isHorizontal() ? y + height * 0.5f : height + y);
        juce::Point<float> endPoint(slider.isHorizontal() ? width + x : startPoint.x,
                                    slider.isHorizontal() ? startPoint.y : y);

        // Track background
        juce::Path backgroundTrack;
        backgroundTrack.startNewSubPath(startPoint);
        backgroundTrack.lineTo(endPoint);
        g.setColour(slider.findColour(juce::Slider::backgroundColourId));
        g.strokePath(backgroundTrack, {trackWidth, juce::PathStrokeType::curved,
                                       juce::PathStrokeType::rounded});

        // Active track
        juce::Path valueTrack;
        juce::Point<float> thumbPoint(slider.isHorizontal() ? sliderPos : startPoint.x,
                                      slider.isHorizontal() ? startPoint.y : sliderPos);
        valueTrack.startNewSubPath(startPoint);
        valueTrack.lineTo(thumbPoint);
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.strokePath(valueTrack, {trackWidth, juce::PathStrokeType::curved,
                                  juce::PathStrokeType::rounded});

        // Thumb
        auto thumbWidth = 14.0f;
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SurgeBoxLookAndFeel)
};

} // namespace SurgeBox
