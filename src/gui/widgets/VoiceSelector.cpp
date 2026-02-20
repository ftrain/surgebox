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

VoiceSelector::VoiceSelector()
{
    setOpaque(false);

    multiSwitch_ = std::make_unique<Widgets::MultiSwitch>();
    multiSwitch_->setRows(1);
    multiSwitch_->setColumns(NUM_VOICES + 3);
    multiSwitch_->setLabels({"1", "2", "3", "4", "K", "CH", "FX"});

    multiSwitch_->onValueChanged = [this](int index) {
        if (index == NUM_VOICES + 2)
        {
            // FX button
            if (onKernelSelected)
                onKernelSelected(false);
            if (onChordTrackSelected)
                onChordTrackSelected(false);
            if (onFXSelected)
                onFXSelected(true);
        }
        else if (index == NUM_VOICES + 1)
        {
            // Chord track button
            if (onKernelSelected)
                onKernelSelected(false);
            if (onFXSelected)
                onFXSelected(false);
            if (onChordTrackSelected)
                onChordTrackSelected(true);
        }
        else if (index == NUM_VOICES)
        {
            // Kernel button - only swaps instrument viewport, keeps piano roll on current voice
            if (onFXSelected)
                onFXSelected(false);
            if (onChordTrackSelected)
                onChordTrackSelected(false);
            if (onKernelSelected)
                onKernelSelected(true);
        }
        else
        {
            // Voice button
            if (onKernelSelected)
                onKernelSelected(false);
            if (onFXSelected)
                onFXSelected(false);
            if (onChordTrackSelected)
                onChordTrackSelected(false);
            if (engine_)
                engine_->setActiveVoice(index);
        }
    };

    addAndMakeVisible(*multiSwitch_);
}

void VoiceSelector::setEngine(SurgeBoxEngine *engine)
{
    engine_ = engine;
    if (engine_)
    {
        multiSwitch_->setValue(engine_->getActiveVoice());
    }
}

void VoiceSelector::selectVoice(int voice)
{
    multiSwitch_->setValue(voice);
}

void VoiceSelector::selectFX()
{
    multiSwitch_->setValue(NUM_VOICES + 2);
}

void VoiceSelector::selectChordTrack()
{
    multiSwitch_->setValue(NUM_VOICES + 1);
}

void VoiceSelector::selectKernel()
{
    multiSwitch_->setValue(NUM_VOICES);
}

void VoiceSelector::resized()
{
    multiSwitch_->setBounds(getLocalBounds());
}

} // namespace SurgeBox
