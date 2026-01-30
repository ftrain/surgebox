/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SynthControlPanel.h"
#include "core/SurgeBoxEngine.h"
#include "SurgeSynthesizer.h"
#include "SurgeStorage.h"

namespace SurgeBox
{

SynthControlPanel::SynthControlPanel()
{
    // Oscillator types
    juce::StringArray oscTypes{"Classic", "Sine",    "Wavetable", "Window", "FM2",   "FM3",
                                "String",  "Twist",   "Alias",     "S&H",    "Audio", "Modern"};

    createComboBox(osc1Type_, oscTypes);
    createComboBox(osc2Type_, oscTypes);

    createSlider(osc1Level_, "", 0.0, 1.0, 0.8);
    createSlider(osc1Pitch_, "", -24.0, 24.0, 0.0);
    createSlider(osc2Level_, "", 0.0, 1.0, 0.0);
    createSlider(osc2Pitch_, "", -24.0, 24.0, 0.0);

    // Filter types (simplified list)
    juce::StringArray filterTypes{"LP 12", "LP 24", "LP Legacy", "BP 12", "BP 24", "HP 12", "HP 24",
                                   "Notch", "Comb",  "S&H"};
    createComboBox(filterType_, filterTypes);

    createSlider(filterCutoff_, "", 0.0, 1.0, 0.8);
    createSlider(filterResonance_, "", 0.0, 1.0, 0.0);
    createSlider(filterEnvMod_, "", -1.0, 1.0, 0.0);
    createSlider(filterKeytrack_, "", 0.0, 1.0, 0.5);

    // Envelopes
    createSlider(aegAttack_, "", 0.0, 1.0, 0.0);
    createSlider(aegDecay_, "", 0.0, 1.0, 0.3);
    createSlider(aegSustain_, "", 0.0, 1.0, 0.7);
    createSlider(aegRelease_, "", 0.0, 1.0, 0.2);

    createSlider(fegAttack_, "", 0.0, 1.0, 0.0);
    createSlider(fegDecay_, "", 0.0, 1.0, 0.5);
    createSlider(fegSustain_, "", 0.0, 1.0, 0.0);
    createSlider(fegRelease_, "", 0.0, 1.0, 0.3);

    // LFO
    juce::StringArray lfoShapes{"Sine", "Triangle", "Square", "Sawtooth", "Noise", "S&H"};
    createComboBox(lfoShape_, lfoShapes);
    createSlider(lfoRate_, "", 0.0, 1.0, 0.5);

    // Output
    createSlider(volume_, "", 0.0, 1.0, 0.8);
    createSlider(pan_, "", -1.0, 1.0, 0.0);

    // Labels
    auto setupLabel = [this](juce::Label &label, float fontSize = 10.0f)
    {
        label.setColour(juce::Label::textColourId, textColor_.withAlpha(0.6f));
        label.setFont(fontSize);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };

    setupLabel(oscLabel_);
    setupLabel(filterLabel_);
    setupLabel(aegLabel_);
    setupLabel(fegLabel_);
    setupLabel(lfoLabel_);
    setupLabel(outputLabel_);

    voiceLabel_.setColour(juce::Label::textColourId, accentColor_);
    voiceLabel_.setFont(18.0f);
    voiceLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(voiceLabel_);
}

void SynthControlPanel::createSlider(juce::Slider &slider, const juce::String &, double min,
                                      double max, double defaultVal)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(min, max, 0.001);
    slider.setValue(defaultVal);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accentColor_);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a4e));
    slider.setColour(juce::Slider::thumbColourId, accentColor_);
    slider.addListener(this);
    addAndMakeVisible(slider);
}

void SynthControlPanel::createComboBox(juce::ComboBox &box, const juce::StringArray &items)
{
    for (int i = 0; i < items.size(); i++)
        box.addItem(items[i], i + 1);
    box.setSelectedId(1);
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a4e));
    box.setColour(juce::ComboBox::textColourId, textColor_);
    box.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    box.addListener(this);
    addAndMakeVisible(box);
}

void SynthControlPanel::setEngine(SurgeBoxEngine *engine)
{
    engine_ = engine;
    if (engine_)
    {
        setVoice(engine_->getActiveVoice());
    }
}

void SynthControlPanel::setVoice(int voice)
{
    voiceIndex_ = voice;
    voiceLabel_.setText("Voice " + juce::String(voice + 1), juce::dontSendNotification);

    if (engine_)
    {
        synth_ = engine_->getSynth(voice);
        updateFromSynth();
    }
}

void SynthControlPanel::updateFromSynth()
{
    if (!synth_)
        return;

    auto &patch = synth_->storage.getPatch();
    auto &scene = patch.scene[0];

    // Oscillators
    osc1Type_.setSelectedId(static_cast<int>(scene.osc[0].type.val.i) + 1, juce::dontSendNotification);
    osc1Level_.setValue(scene.level_o1.get_value_f01(), juce::dontSendNotification);
    osc2Type_.setSelectedId(static_cast<int>(scene.osc[1].type.val.i) + 1, juce::dontSendNotification);
    osc2Level_.setValue(scene.level_o2.get_value_f01(), juce::dontSendNotification);

    // Filter
    filterCutoff_.setValue(scene.filterunit[0].cutoff.get_value_f01(), juce::dontSendNotification);
    filterResonance_.setValue(scene.filterunit[0].resonance.get_value_f01(),
                               juce::dontSendNotification);
    filterEnvMod_.setValue(scene.filterunit[0].envmod.get_value_f01() * 2.0 - 1.0,
                           juce::dontSendNotification);

    // AEG
    aegAttack_.setValue(scene.adsr[0].a.get_value_f01(), juce::dontSendNotification);
    aegDecay_.setValue(scene.adsr[0].d.get_value_f01(), juce::dontSendNotification);
    aegSustain_.setValue(scene.adsr[0].s.get_value_f01(), juce::dontSendNotification);
    aegRelease_.setValue(scene.adsr[0].r.get_value_f01(), juce::dontSendNotification);

    // FEG
    fegAttack_.setValue(scene.adsr[1].a.get_value_f01(), juce::dontSendNotification);
    fegDecay_.setValue(scene.adsr[1].d.get_value_f01(), juce::dontSendNotification);
    fegSustain_.setValue(scene.adsr[1].s.get_value_f01(), juce::dontSendNotification);
    fegRelease_.setValue(scene.adsr[1].r.get_value_f01(), juce::dontSendNotification);

    // Output
    volume_.setValue(scene.volume.get_value_f01(), juce::dontSendNotification);
    pan_.setValue(scene.pan.get_value_f01() * 2.0 - 1.0, juce::dontSendNotification);
}

void SynthControlPanel::sliderValueChanged(juce::Slider *slider)
{
    updateSynthParameter(slider);
}

void SynthControlPanel::comboBoxChanged(juce::ComboBox *comboBox)
{
    if (!synth_)
        return;

    auto &patch = synth_->storage.getPatch();
    auto &scene = patch.scene[0];

    if (comboBox == &osc1Type_)
    {
        auto id = synth_->idForParameter(&scene.osc[0].type);
        synth_->setParameter01(id, (comboBox->getSelectedId() - 1) / 11.0f);
    }
    else if (comboBox == &osc2Type_)
    {
        auto id = synth_->idForParameter(&scene.osc[1].type);
        synth_->setParameter01(id, (comboBox->getSelectedId() - 1) / 11.0f);
    }
}

void SynthControlPanel::updateSynthParameter(juce::Slider *slider)
{
    if (!synth_)
        return;

    auto &patch = synth_->storage.getPatch();
    auto &scene = patch.scene[0];

    float value = static_cast<float>(slider->getValue());
    Parameter *param = nullptr;

    // Oscillators
    if (slider == &osc1Level_)
        param = &scene.level_o1;
    else if (slider == &osc2Level_)
        param = &scene.level_o2;

    // Filter
    else if (slider == &filterCutoff_)
        param = &scene.filterunit[0].cutoff;
    else if (slider == &filterResonance_)
        param = &scene.filterunit[0].resonance;
    else if (slider == &filterEnvMod_)
    {
        param = &scene.filterunit[0].envmod;
        value = (value + 1.0f) * 0.5f;
    }
    else if (slider == &filterKeytrack_)
        param = &scene.filterunit[0].keytrack;

    // AEG
    else if (slider == &aegAttack_)
        param = &scene.adsr[0].a;
    else if (slider == &aegDecay_)
        param = &scene.adsr[0].d;
    else if (slider == &aegSustain_)
        param = &scene.adsr[0].s;
    else if (slider == &aegRelease_)
        param = &scene.adsr[0].r;

    // FEG
    else if (slider == &fegAttack_)
        param = &scene.adsr[1].a;
    else if (slider == &fegDecay_)
        param = &scene.adsr[1].d;
    else if (slider == &fegSustain_)
        param = &scene.adsr[1].s;
    else if (slider == &fegRelease_)
        param = &scene.adsr[1].r;

    // Output
    else if (slider == &volume_)
        param = &scene.volume;
    else if (slider == &pan_)
    {
        param = &scene.pan;
        value = (value + 1.0f) * 0.5f;
    }

    if (param)
    {
        auto id = synth_->idForParameter(param);
        synth_->setParameter01(id, value);
    }
}

void SynthControlPanel::paint(juce::Graphics &g)
{
    g.fillAll(bgColor_);

    // Section backgrounds
    int margin = 8;
    int y = 35;
    int sectionHeight = getHeight() - y - margin;

    // Calculate section widths
    int oscWidth = 160;
    int filterWidth = 130;
    int envWidth = 110;
    int lfoWidth = 80;
    int outputWidth = 80;

    int x = margin;

    // Draw section backgrounds
    g.setColour(sectionColor_);

    // OSC
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(oscWidth), static_cast<float>(sectionHeight), 6);
    x += oscWidth + margin;

    // Filter
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(filterWidth), static_cast<float>(sectionHeight), 6);
    x += filterWidth + margin;

    // AEG
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(envWidth), static_cast<float>(sectionHeight), 6);
    x += envWidth + margin;

    // FEG
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(envWidth), static_cast<float>(sectionHeight), 6);
    x += envWidth + margin;

    // LFO
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(lfoWidth), static_cast<float>(sectionHeight), 6);
    x += lfoWidth + margin;

    // Output
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                            static_cast<float>(outputWidth), static_cast<float>(sectionHeight), 6);
}

void SynthControlPanel::resized()
{
    int margin = 8;
    int labelHeight = 16;
    int knobSize = 45;
    int comboHeight = 22;
    int rowGap = 4;

    auto bounds = getLocalBounds().reduced(margin);

    // Voice label at top
    voiceLabel_.setBounds(bounds.removeFromTop(28));

    // Section widths
    int oscWidth = 160;
    int filterWidth = 130;
    int envWidth = 110;
    int lfoWidth = 80;
    int outputWidth = 80;

    // OSC Section
    auto oscBounds = bounds.removeFromLeft(oscWidth);
    oscLabel_.setBounds(oscBounds.removeFromTop(labelHeight));
    oscBounds.removeFromTop(rowGap);

    auto oscRow1 = oscBounds.removeFromTop(comboHeight);
    osc1Type_.setBounds(oscRow1.removeFromLeft(75).reduced(2, 0));
    osc2Type_.setBounds(oscRow1.removeFromLeft(75).reduced(2, 0));
    oscBounds.removeFromTop(rowGap);

    auto oscRow2 = oscBounds.removeFromTop(knobSize);
    osc1Level_.setBounds(oscRow2.removeFromLeft(40));
    osc1Pitch_.setBounds(oscRow2.removeFromLeft(40));
    osc2Level_.setBounds(oscRow2.removeFromLeft(40));
    osc2Pitch_.setBounds(oscRow2.removeFromLeft(40));

    bounds.removeFromLeft(margin);

    // Filter Section
    auto filterBounds = bounds.removeFromLeft(filterWidth);
    filterLabel_.setBounds(filterBounds.removeFromTop(labelHeight));
    filterBounds.removeFromTop(rowGap);

    filterType_.setBounds(filterBounds.removeFromTop(comboHeight).reduced(2, 0));
    filterBounds.removeFromTop(rowGap);

    auto filterRow = filterBounds.removeFromTop(knobSize);
    filterCutoff_.setBounds(filterRow.removeFromLeft(32));
    filterResonance_.setBounds(filterRow.removeFromLeft(32));
    filterEnvMod_.setBounds(filterRow.removeFromLeft(32));
    filterKeytrack_.setBounds(filterRow.removeFromLeft(32));

    bounds.removeFromLeft(margin);

    // AEG Section
    auto aegBounds = bounds.removeFromLeft(envWidth);
    aegLabel_.setBounds(aegBounds.removeFromTop(labelHeight));
    aegBounds.removeFromTop(rowGap + comboHeight + rowGap);

    auto aegRow = aegBounds.removeFromTop(knobSize);
    aegAttack_.setBounds(aegRow.removeFromLeft(27));
    aegDecay_.setBounds(aegRow.removeFromLeft(27));
    aegSustain_.setBounds(aegRow.removeFromLeft(27));
    aegRelease_.setBounds(aegRow.removeFromLeft(27));

    bounds.removeFromLeft(margin);

    // FEG Section
    auto fegBounds = bounds.removeFromLeft(envWidth);
    fegLabel_.setBounds(fegBounds.removeFromTop(labelHeight));
    fegBounds.removeFromTop(rowGap + comboHeight + rowGap);

    auto fegRow = fegBounds.removeFromTop(knobSize);
    fegAttack_.setBounds(fegRow.removeFromLeft(27));
    fegDecay_.setBounds(fegRow.removeFromLeft(27));
    fegSustain_.setBounds(fegRow.removeFromLeft(27));
    fegRelease_.setBounds(fegRow.removeFromLeft(27));

    bounds.removeFromLeft(margin);

    // LFO Section
    auto lfoBounds = bounds.removeFromLeft(lfoWidth);
    lfoLabel_.setBounds(lfoBounds.removeFromTop(labelHeight));
    lfoBounds.removeFromTop(rowGap);

    lfoShape_.setBounds(lfoBounds.removeFromTop(comboHeight).reduced(2, 0));
    lfoBounds.removeFromTop(rowGap);

    lfoRate_.setBounds(lfoBounds.removeFromTop(knobSize).withWidth(knobSize));

    bounds.removeFromLeft(margin);

    // Output Section
    auto outputBounds = bounds.removeFromLeft(outputWidth);
    outputLabel_.setBounds(outputBounds.removeFromTop(labelHeight));
    outputBounds.removeFromTop(rowGap + comboHeight + rowGap);

    auto outputRow = outputBounds.removeFromTop(knobSize);
    volume_.setBounds(outputRow.removeFromLeft(40));
    pan_.setBounds(outputRow.removeFromLeft(40));
}

} // namespace SurgeBox
