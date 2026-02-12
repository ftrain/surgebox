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

TransportControls::TransportControls()
{
    setOpaque(false);

    playButton_ = std::make_unique<Widgets::IconButton>(Widgets::IconButton::Play);
    playButton_->onClick = [this]() {
        if (engine_)
        {
            engine_->play();
            updateDisplay();
        }
    };
    addAndMakeVisible(*playButton_);

    stopButton_ = std::make_unique<Widgets::IconButton>(Widgets::IconButton::Stop);
    stopButton_->onClick = [this]() {
        if (engine_)
        {
            engine_->stop();
            engine_->getSequencer().rewind();
            updateDisplay();
        }
    };
    addAndMakeVisible(*stopButton_);

    tempoField_ = std::make_unique<Widgets::NumberField>();
    tempoField_->setRange(20, 300);
    tempoField_->setSuffix(" BPM");
    tempoField_->onValueChanged = [this](int tempo) {
        if (engine_)
        {
            engine_->getProject().tempo = static_cast<double>(tempo);
        }
    };
    addAndMakeVisible(*tempoField_);
}

void TransportControls::setEngine(SurgeBoxEngine *engine)
{
    engine_ = engine;
    updateDisplay();
}

void TransportControls::updateDisplay()
{
    if (engine_)
    {
        playButton_->setToggleState(engine_->isPlaying());
        stopButton_->setToggleState(!engine_->isPlaying());
        tempoField_->setValue(static_cast<int>(engine_->getProject().tempo));
    }
    repaint();
}

void TransportControls::resized()
{
    auto bounds = getLocalBounds().reduced(2);
    int buttonSize = bounds.getHeight();

    playButton_->setBounds(bounds.removeFromLeft(buttonSize).reduced(2));
    bounds.removeFromLeft(2);
    stopButton_->setBounds(bounds.removeFromLeft(buttonSize).reduced(2));
    bounds.removeFromLeft(4);
    tempoField_->setBounds(bounds.reduced(2));
}

} // namespace SurgeBox
