/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "MasterFXEditor.h"
#include "core/MasterFXChain.h"
#include "SurgeStorage.h"

namespace SurgeBox
{

MasterFXEditor::MasterFXEditor(MasterFXChain &fxChain) : fxChain_(fxChain)
{
    for (int s = 0; s < NUM_SLOTS; s++)
    {
        auto &slot = slots_[s];

        slot.slotLabel = std::make_unique<juce::Label>("", "FX " + juce::String(s + 1));
        slot.slotLabel->setColour(juce::Label::textColourId, juce::Colours::white);
        slot.slotLabel->setFont(juce::FontOptions(14.0f, juce::Font::bold));
        addAndMakeVisible(*slot.slotLabel);

        slot.enableButton = std::make_unique<juce::ToggleButton>("On");
        slot.enableButton->setToggleState(true, juce::dontSendNotification);
        slot.enableButton->addListener(this);
        addAndMakeVisible(*slot.enableButton);

        slot.typeCombo = std::make_unique<juce::ComboBox>();
        for (int t = 0; t < n_fx_types; t++)
        {
            // Skip audio input — not useful for master FX
            if (t == fxt_audio_input)
                continue;
            slot.typeCombo->addItem(fx_type_names[t], t + 1); // ComboBox IDs are 1-based
        }
        slot.typeCombo->setSelectedId(fxt_off + 1, juce::dontSendNotification);
        slot.typeCombo->addListener(this);
        addAndMakeVisible(*slot.typeCombo);

        for (int p = 0; p < NUM_PARAMS; p++)
        {
            slot.paramSliders[p] = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                                    juce::Slider::NoTextBox);
            slot.paramSliders[p]->setRange(0.0, 1.0, 0.001);
            slot.paramSliders[p]->addListener(this);
            addAndMakeVisible(*slot.paramSliders[p]);

            slot.paramLabels[p] = std::make_unique<juce::Label>("", "P" + juce::String(p + 1));
            slot.paramLabels[p]->setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
            slot.paramLabels[p]->setFont(juce::FontOptions(9.0f));
            slot.paramLabels[p]->setJustificationType(juce::Justification::centredTop);
            addAndMakeVisible(*slot.paramLabels[p]);
        }
    }

    refreshFromChain();
}

void MasterFXEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    // Draw slot separators
    for (int s = 1; s < NUM_SLOTS; s++)
    {
        int y = s * (getHeight() / NUM_SLOTS);
        g.setColour(juce::Colour(0xff3a3a5a));
        g.drawHorizontalLine(y, 4.0f, getWidth() - 4.0f);
    }
}

void MasterFXEditor::resized()
{
    int slotHeight = getHeight() / NUM_SLOTS;
    int pad = 4;

    for (int s = 0; s < NUM_SLOTS; s++)
    {
        auto &slot = slots_[s];
        auto slotArea = juce::Rectangle<int>(0, s * slotHeight, getWidth(), slotHeight).reduced(pad);

        auto topRow = slotArea.removeFromTop(24);
        slot.slotLabel->setBounds(topRow.removeFromLeft(40));
        slot.enableButton->setBounds(topRow.removeFromLeft(40));
        slot.typeCombo->setBounds(topRow.removeFromLeft(160).reduced(0, 2));

        slotArea.removeFromTop(4);

        // Parameter knobs in a row
        int knobSize = 40;
        int labelHeight = 14;
        int totalParamHeight = knobSize + labelHeight;
        auto paramArea = slotArea.removeFromTop(totalParamHeight);

        int availWidth = paramArea.getWidth();
        int knobSpacing = std::min(availWidth / NUM_PARAMS, knobSize + 8);

        for (int p = 0; p < NUM_PARAMS; p++)
        {
            int x = paramArea.getX() + p * knobSpacing;
            slot.paramSliders[p]->setBounds(x, paramArea.getY(), knobSize, knobSize);
            slot.paramLabels[p]->setBounds(x, paramArea.getY() + knobSize, knobSize, labelHeight);
        }
    }
}

void MasterFXEditor::comboBoxChanged(juce::ComboBox *comboBox)
{
    for (int s = 0; s < NUM_SLOTS; s++)
    {
        if (comboBox == slots_[s].typeCombo.get())
        {
            int fxType = comboBox->getSelectedId() - 1; // Convert 1-based to 0-based
            fxChain_.setEffectType(s, fxType);
            refreshFromChain();
            return;
        }
    }
}

void MasterFXEditor::sliderValueChanged(juce::Slider *slider)
{
    for (int s = 0; s < NUM_SLOTS; s++)
    {
        for (int p = 0; p < NUM_PARAMS; p++)
        {
            if (slider == slots_[s].paramSliders[p].get())
            {
                fxChain_.setParameter(s, p, static_cast<float>(slider->getValue()));
                return;
            }
        }
    }
}

void MasterFXEditor::buttonClicked(juce::Button *button)
{
    for (int s = 0; s < NUM_SLOTS; s++)
    {
        if (button == slots_[s].enableButton.get())
        {
            fxChain_.setSlotEnabled(s, button->getToggleState());
            return;
        }
    }
}

void MasterFXEditor::refreshFromChain()
{
    for (int s = 0; s < NUM_SLOTS; s++)
    {
        auto &slot = slots_[s];

        int fxType = fxChain_.getEffectType(s);
        slot.typeCombo->setSelectedId(fxType + 1, juce::dontSendNotification);
        slot.enableButton->setToggleState(fxChain_.isSlotEnabled(s), juce::dontSendNotification);

        bool active = (fxType != fxt_off);
        for (int p = 0; p < NUM_PARAMS; p++)
        {
            slot.paramSliders[p]->setValue(fxChain_.getParameter(s, p), juce::dontSendNotification);
            slot.paramSliders[p]->setEnabled(active);
        }
    }
}

} // namespace SurgeBox
