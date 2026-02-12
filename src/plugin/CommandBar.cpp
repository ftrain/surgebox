/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "CommandBar.h"
#include "MeasureControls.h"
#include "core/SurgeBoxEngine.h"
#include "core/MusicTheory.h"

namespace SurgeBox
{

CommandBar::CommandBar(SurgeBoxEngine& engine) : engine_(engine)
{
    voiceSelector_ = std::make_unique<VoiceSelector>();
    voiceSelector_->setEngine(&engine_);
    addAndMakeVisible(*voiceSelector_);

    transport_ = std::make_unique<TransportControls>();
    transport_->setEngine(&engine_);
    addAndMakeVisible(*transport_);

    stepRecordButton_ = std::make_unique<juce::TextButton>("STEP");
    stepRecordButton_->addListener(this);
    stepRecordButton_->setClickingTogglesState(true);
    stepRecordButton_->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a6a));
    stepRecordButton_->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff4444));
    stepRecordButton_->setTooltip("Step Record Mode");
    addAndMakeVisible(*stepRecordButton_);

    measuresLabel_ = std::make_unique<juce::Label>("", "1 bar");
    measuresLabel_->setColour(juce::Label::textColourId, juce::Colours::white);
    measuresLabel_->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*measuresLabel_);

    measuresDoubleBtn_ = std::make_unique<juce::TextButton>("x2");
    measuresDoubleBtn_->addListener(this);
    measuresDoubleBtn_->setTooltip("Double measures (clone pattern)");
    addAndMakeVisible(*measuresDoubleBtn_);

    measuresHalfBtn_ = std::make_unique<juce::TextButton>("/2");
    measuresHalfBtn_->addListener(this);
    measuresHalfBtn_->setTooltip("Halve measures");
    addAndMakeVisible(*measuresHalfBtn_);

    measuresAddBtn_ = std::make_unique<juce::TextButton>("+");
    measuresAddBtn_->addListener(this);
    measuresAddBtn_->setTooltip("Add one measure");
    addAndMakeVisible(*measuresAddBtn_);

    measuresSubBtn_ = std::make_unique<juce::TextButton>("-");
    measuresSubBtn_->addListener(this);
    measuresSubBtn_->setTooltip("Remove one measure");
    addAndMakeVisible(*measuresSubBtn_);

    gridSizeLabel_ = std::make_unique<juce::Label>("", "Grid:");
    gridSizeLabel_->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*gridSizeLabel_);

    gridSizeCombo_ = std::make_unique<juce::ComboBox>();
    gridSizeCombo_->addItem("1/4", 1);
    gridSizeCombo_->addItem("1/8", 2);
    gridSizeCombo_->addItem("1/16", 3);
    gridSizeCombo_->addItem("1/32", 4);
    gridSizeCombo_->setSelectedId(3);
    gridSizeCombo_->addListener(this);
    addAndMakeVisible(*gridSizeCombo_);

    clearPatternBtn_ = std::make_unique<juce::TextButton>("CLR");
    clearPatternBtn_->addListener(this);
    clearPatternBtn_->setTooltip("Clear pattern");
    addAndMakeVisible(*clearPatternBtn_);

    scaleLabel_ = std::make_unique<juce::Label>("", "Scale:");
    scaleLabel_->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*scaleLabel_);

    scaleRootCombo_ = std::make_unique<juce::ComboBox>();
    scaleRootCombo_->addItem("C", 1);
    scaleRootCombo_->addItem("C#", 2);
    scaleRootCombo_->addItem("D", 3);
    scaleRootCombo_->addItem("D#", 4);
    scaleRootCombo_->addItem("E", 5);
    scaleRootCombo_->addItem("F", 6);
    scaleRootCombo_->addItem("F#", 7);
    scaleRootCombo_->addItem("G", 8);
    scaleRootCombo_->addItem("G#", 9);
    scaleRootCombo_->addItem("A", 10);
    scaleRootCombo_->addItem("A#", 11);
    scaleRootCombo_->addItem("B", 12);
    scaleRootCombo_->setSelectedId(1);
    scaleRootCombo_->addListener(this);
    addAndMakeVisible(*scaleRootCombo_);

    scaleTypeCombo_ = std::make_unique<juce::ComboBox>();
    scaleTypeCombo_->addItem("Chromatic", 1);
    scaleTypeCombo_->addItem("Major", 2);
    scaleTypeCombo_->addItem("Minor", 3);
    scaleTypeCombo_->addItem("Harmonic Min", 4);
    scaleTypeCombo_->addItem("Melodic Min", 5);
    scaleTypeCombo_->addItem("Pentatonic", 6);
    scaleTypeCombo_->addItem("Pent. Minor", 7);
    scaleTypeCombo_->addItem("Blues", 8);
    scaleTypeCombo_->addItem("Dorian", 9);
    scaleTypeCombo_->addItem("Phrygian", 10);
    scaleTypeCombo_->addItem("Lydian", 11);
    scaleTypeCombo_->addItem("Mixolydian", 12);
    scaleTypeCombo_->addItem("Locrian", 13);
    scaleTypeCombo_->setSelectedId(1);
    scaleTypeCombo_->addListener(this);
    addAndMakeVisible(*scaleTypeCombo_);

    tempoLabel_ = std::make_unique<juce::Label>("", "BPM:");
    tempoLabel_->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*tempoLabel_);

    tempoSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                   juce::Slider::TextBoxRight);
    tempoSlider_->setRange(20.0, 300.0, 1.0);
    tempoSlider_->setValue(engine_.getProject().tempo);
    tempoSlider_->setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 20);
    tempoSlider_->addListener(this);
    addAndMakeVisible(*tempoSlider_);

    tempoMultiplierCombo_ = std::make_unique<juce::ComboBox>();
    tempoMultiplierCombo_->addItem("4x", 1);
    tempoMultiplierCombo_->addItem("2x", 2);
    tempoMultiplierCombo_->addItem("1x", 3);
    tempoMultiplierCombo_->addItem("1/2", 4);
    tempoMultiplierCombo_->addItem("1/4", 5);
    tempoMultiplierCombo_->addItem("1/8", 6);
    tempoMultiplierCombo_->addItem("1/16", 7);
    tempoMultiplierCombo_->setSelectedId(3);
    tempoMultiplierCombo_->setTooltip("Playback speed multiplier");
    tempoMultiplierCombo_->addListener(this);
    addAndMakeVisible(*tempoMultiplierCombo_);
}

void CommandBar::resized()
{
    auto commandBar = getLocalBounds();
    int pad = 2;

    voiceSelector_->setBounds(commandBar.removeFromLeft(160).reduced(pad, pad));
    transport_->setBounds(commandBar.removeFromLeft(120).reduced(pad, pad));
    stepRecordButton_->setBounds(commandBar.removeFromLeft(60).reduced(pad, pad));

    commandBar.removeFromLeft(10);
    measuresHalfBtn_->setBounds(commandBar.removeFromLeft(36).reduced(pad, pad));
    measuresSubBtn_->setBounds(commandBar.removeFromLeft(36).reduced(pad, pad));
    measuresLabel_->setBounds(commandBar.removeFromLeft(60).reduced(pad, pad));
    measuresAddBtn_->setBounds(commandBar.removeFromLeft(36).reduced(pad, pad));
    measuresDoubleBtn_->setBounds(commandBar.removeFromLeft(36).reduced(pad, pad));

    commandBar.removeFromLeft(10);
    gridSizeLabel_->setBounds(commandBar.removeFromLeft(40).reduced(pad, pad));
    gridSizeCombo_->setBounds(commandBar.removeFromLeft(70).reduced(pad, pad));

    commandBar.removeFromLeft(10);
    tempoLabel_->setBounds(commandBar.removeFromLeft(40).reduced(pad, pad));
    tempoSlider_->setBounds(commandBar.removeFromLeft(140).reduced(pad, pad));
    tempoMultiplierCombo_->setBounds(commandBar.removeFromLeft(55).reduced(pad, pad));

    commandBar.removeFromLeft(10);
    clearPatternBtn_->setBounds(commandBar.removeFromLeft(40).reduced(pad, pad));

    commandBar.removeFromLeft(10);
    scaleLabel_->setBounds(commandBar.removeFromLeft(45).reduced(pad, pad));
    scaleRootCombo_->setBounds(commandBar.removeFromLeft(55).reduced(pad, pad));
    scaleTypeCombo_->setBounds(commandBar.removeFromLeft(100).reduced(pad, pad));
}

void CommandBar::buttonClicked(juce::Button* button)
{
    auto* model = engine_.getActivePatternModel();

    if (button == stepRecordButton_.get())
    {
        if (onStepRecordChanged)
            onStepRecordChanged(stepRecordButton_->getToggleState());
    }
    else if (button == measuresDoubleBtn_.get())
    {
        MeasureControls::doubleMeasures(model);
        updateMeasuresLabel();
        if (onMeasuresChanged)
            onMeasuresChanged();
    }
    else if (button == measuresHalfBtn_.get())
    {
        MeasureControls::halveMeasures(model);
        updateMeasuresLabel();
        if (onMeasuresChanged)
            onMeasuresChanged();
    }
    else if (button == measuresAddBtn_.get())
    {
        MeasureControls::addMeasure(model);
        updateMeasuresLabel();
        if (onMeasuresChanged)
            onMeasuresChanged();
    }
    else if (button == measuresSubBtn_.get())
    {
        MeasureControls::subtractMeasure(model);
        updateMeasuresLabel();
        if (onMeasuresChanged)
            onMeasuresChanged();
    }
    else if (button == clearPatternBtn_.get())
    {
        MeasureControls::clearPattern(model);
        if (onClearPattern)
            onClearPattern();
    }
}

void CommandBar::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == gridSizeCombo_.get())
    {
        double gridSize = 0.25;
        switch (gridSizeCombo_->getSelectedId())
        {
            case 1: gridSize = 1.0; break;
            case 2: gridSize = 0.5; break;
            case 3: gridSize = 0.25; break;
            case 4: gridSize = 0.125; break;
        }
        if (onGridSizeChanged)
            onGridSizeChanged(gridSize);
    }
    else if (comboBox == scaleRootCombo_.get() || comboBox == scaleTypeCombo_.get())
    {
        int root = scaleRootCombo_->getSelectedId() - 1;
        ScaleType type = ScaleType::Chromatic;
        switch (scaleTypeCombo_->getSelectedId())
        {
            case 1: type = ScaleType::Chromatic; break;
            case 2: type = ScaleType::Major; break;
            case 3: type = ScaleType::NaturalMinor; break;
            case 4: type = ScaleType::HarmonicMinor; break;
            case 5: type = ScaleType::MelodicMinor; break;
            case 6: type = ScaleType::Pentatonic; break;
            case 7: type = ScaleType::PentatonicMinor; break;
            case 8: type = ScaleType::Blues; break;
            case 9: type = ScaleType::Dorian; break;
            case 10: type = ScaleType::Phrygian; break;
            case 11: type = ScaleType::Lydian; break;
            case 12: type = ScaleType::Mixolydian; break;
            case 13: type = ScaleType::Locrian; break;
        }
        if (onScaleChanged)
            onScaleChanged(root, type);
    }
    else if (comboBox == tempoMultiplierCombo_.get())
    {
        double multiplier = 1.0;
        switch (tempoMultiplierCombo_->getSelectedId())
        {
            case 1: multiplier = 4.0; break;
            case 2: multiplier = 2.0; break;
            case 3: multiplier = 1.0; break;
            case 4: multiplier = 0.5; break;
            case 5: multiplier = 0.25; break;
            case 6: multiplier = 0.125; break;
            case 7: multiplier = 0.0625; break;
        }
        int activeVoice = engine_.getActiveVoice();
        engine_.getProject().voices[activeVoice].pendingTempoMultiplier.store(multiplier);

        if (!engine_.isPlaying())
        {
            engine_.getProject().voices[activeVoice].tempoMultiplier.store(multiplier);
        }
    }
}

void CommandBar::sliderValueChanged(juce::Slider* slider)
{
    if (slider == tempoSlider_.get())
    {
        engine_.getProject().tempo = static_cast<float>(tempoSlider_->getValue());
    }
}

void CommandBar::updateMeasuresLabel()
{
    measuresLabel_->setText(MeasureControls::getMeasuresLabel(engine_.getActivePatternModel()),
                            juce::dontSendNotification);
}

void CommandBar::updateTempoMultiplier(double multiplier)
{
    int comboId = 3;
    if (multiplier >= 3.5) comboId = 1;
    else if (multiplier >= 1.5) comboId = 2;
    else if (multiplier >= 0.75) comboId = 3;
    else if (multiplier >= 0.375) comboId = 4;
    else if (multiplier >= 0.1875) comboId = 5;
    else if (multiplier >= 0.09) comboId = 6;
    else comboId = 7;
    tempoMultiplierCombo_->setSelectedId(comboId, juce::dontSendNotification);
}

} // namespace SurgeBox
