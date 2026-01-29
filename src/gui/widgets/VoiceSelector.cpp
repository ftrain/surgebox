/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "VoiceSelector.h"
#include "core/SurgeBoxEngine.h"

namespace SurgeBox
{

VoiceSelector::VoiceSelector() { setOpaque(false); }

void VoiceSelector::setEngine(SurgeBoxEngine *engine) { engine_ = engine; }

void VoiceSelector::paint(juce::Graphics &g)
{
    // Background
    g.fillAll(bgColor_);

    int activeVoice = engine_ ? engine_->getActiveVoice() : 0;

    for (int i = 0; i < NUM_VOICES; i++)
    {
        auto &bounds = buttonBounds_[i];

        // Button background
        bool isActive = (i == activeVoice);
        g.setColour(isActive ? activeColor_ : buttonColor_);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        // Border
        g.setColour(isActive ? activeColor_.brighter() : textColor_.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

        // Voice number
        g.setColour(isActive ? juce::Colours::black : textColor_);
        g.setFont(14.0f);
        g.drawText(juce::String(i + 1), bounds, juce::Justification::centred);

        // TODO: Add voice activity indicator
        // Would need to count voices in synth->voices[0] and synth->voices[1] lists
    }

    // Label
    g.setColour(textColor_.withAlpha(0.6f));
    g.setFont(10.0f);
    g.drawText("VOICE", 0, getHeight() - 14, getWidth(), 14, juce::Justification::centred);
}

void VoiceSelector::resized()
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromBottom(16); // Space for label

    int buttonWidth = (bounds.getWidth() - 12) / NUM_VOICES;

    for (int i = 0; i < NUM_VOICES; i++)
    {
        buttonBounds_[i] = bounds.removeFromLeft(buttonWidth).reduced(2);
        if (i < NUM_VOICES - 1)
            bounds.removeFromLeft(4); // Gap
    }
}

void VoiceSelector::mouseDown(const juce::MouseEvent &e)
{
    if (!engine_)
        return;

    for (int i = 0; i < NUM_VOICES; i++)
    {
        if (buttonBounds_[i].contains(e.getPosition()))
        {
            engine_->setActiveVoice(i);
            repaint();
            return;
        }
    }
}

} // namespace SurgeBox
