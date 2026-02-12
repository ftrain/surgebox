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
    multiSwitch_->setColumns(NUM_VOICES);
    multiSwitch_->setLabels({"1", "2", "3", "4"});

    multiSwitch_->onValueChanged = [this](int voice) {
        if (engine_)
        {
            engine_->setActiveVoice(voice);
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

void VoiceSelector::resized()
{
    multiSwitch_->setBounds(getLocalBounds());
}

} // namespace SurgeBox
