/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "TransportControls.h"
#include "core/SurgeBoxEngine.h"

namespace SurgeBox
{

TransportControls::TransportControls() { setOpaque(false); }

void TransportControls::setEngine(SurgeBoxEngine *engine) { engine_ = engine; }

void TransportControls::updateDisplay() { repaint(); }

void TransportControls::paint(juce::Graphics &g)
{
    // Background
    g.fillAll(bgColor_);

    bool isPlaying = engine_ && engine_->isPlaying();

    // Play button
    g.setColour(isPlaying ? playColor_ : buttonColor_);
    g.fillRoundedRectangle(playBounds_.toFloat(), 4.0f);
    g.setColour(textColor_);

    // Play triangle
    {
        auto center = playBounds_.getCentre();
        juce::Path triangle;
        triangle.addTriangle(static_cast<float>(center.x - 6), static_cast<float>(center.y - 8),
                            static_cast<float>(center.x - 6), static_cast<float>(center.y + 8),
                            static_cast<float>(center.x + 8), static_cast<float>(center.y));
        g.fillPath(triangle);
    }

    // Stop button
    g.setColour(!isPlaying ? stopColor_.darker() : buttonColor_);
    g.fillRoundedRectangle(stopBounds_.toFloat(), 4.0f);
    g.setColour(textColor_);

    // Stop square
    {
        auto center = stopBounds_.getCentre();
        g.fillRect(center.x - 6, center.y - 6, 12, 12);
    }

    // Tempo display
    g.setColour(buttonColor_);
    g.fillRoundedRectangle(tempoBounds_.toFloat(), 4.0f);

    double tempo = engine_ ? engine_->getProject().tempo : 120.0;
    g.setColour(textColor_);
    g.setFont(14.0f);
    g.drawText(juce::String(static_cast<int>(tempo)) + " BPM", tempoBounds_,
               juce::Justification::centred);

    // Labels
    g.setColour(textColor_.withAlpha(0.6f));
    g.setFont(10.0f);
    g.drawText("TRANSPORT", 0, getHeight() - 14, getWidth(), 14, juce::Justification::centred);
}

void TransportControls::resized()
{
    auto bounds = getLocalBounds().reduced(2);
    bounds.removeFromBottom(16);

    int buttonSize = bounds.getHeight();
    int tempoWidth = bounds.getWidth() - buttonSize * 2 - 8;

    playBounds_ = bounds.removeFromLeft(buttonSize).reduced(2);
    bounds.removeFromLeft(2);
    stopBounds_ = bounds.removeFromLeft(buttonSize).reduced(2);
    bounds.removeFromLeft(4);
    tempoBounds_ = bounds.reduced(2);
}

void TransportControls::mouseDown(const juce::MouseEvent &e)
{
    if (!engine_)
        return;

    if (playBounds_.contains(e.getPosition()))
    {
        engine_->play();
        repaint();
    }
    else if (stopBounds_.contains(e.getPosition()))
    {
        engine_->stop();
        engine_->getSequencer().rewind();
        repaint();
    }
    else if (tempoBounds_.contains(e.getPosition()))
    {
        // TODO: Show tempo edit popup
    }
}

} // namespace SurgeBox
