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
    // Menu buttons
    for (auto* btn : {&patternMenuBtn_, &editMenuBtn_, &scaleMenuBtn_, &viewMenuBtn_})
    {
        addAndMakeVisible(btn);
        btn->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }

    patternMenuBtn_.onClick = [this]() { showPatternMenu(); };
    editMenuBtn_.onClick = [this]() { showEditMenu(); };
    scaleMenuBtn_.onClick = [this]() { showScaleMenu(); };
    viewMenuBtn_.onClick = [this]() { showViewMenu(); };

    // Voice selector
    voiceSelector_ = std::make_unique<VoiceSelector>();
    voiceSelector_->setEngine(&engine_);
    addAndMakeVisible(*voiceSelector_);

    // Instrument selector
    instrumentLabel_ = std::make_unique<juce::Label>("", "Inst:");
    instrumentLabel_->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*instrumentLabel_);

    instrumentCombo_ = std::make_unique<juce::ComboBox>();
    instrumentCombo_->addItem("Surge XT", 1);
    instrumentCombo_->addItem("Dexed", 2);
    instrumentCombo_->addItem("TR-808", 3);
    instrumentCombo_->setSelectedId(1, juce::dontSendNotification);
    instrumentCombo_->onChange = [this]() {
        SurgeBox::InstrumentType type = SurgeBox::InstrumentType::SurgeXT;
        switch (instrumentCombo_->getSelectedId())
        {
            case 1: type = SurgeBox::InstrumentType::SurgeXT; break;
            case 2: type = SurgeBox::InstrumentType::Dexed; break;
            case 3: type = SurgeBox::InstrumentType::TR808; break;
        }
        if (onInstrumentChanged)
            onInstrumentChanged(type);
    };
    addAndMakeVisible(*instrumentCombo_);

    // Transport
    transport_ = std::make_unique<TransportControls>();
    transport_->setEngine(&engine_);
    addAndMakeVisible(*transport_);

    currentTempo_ = engine_.getProject().tempo;
    currentBars_ = 1;
}

void CommandBar::resized()
{
    auto bounds = getLocalBounds();
    int pad = 2;

    // Menu buttons on the left
    patternMenuBtn_.setBounds(bounds.removeFromLeft(70).reduced(pad, pad));
    editMenuBtn_.setBounds(bounds.removeFromLeft(50).reduced(pad, pad));
    scaleMenuBtn_.setBounds(bounds.removeFromLeft(55).reduced(pad, pad));
    viewMenuBtn_.setBounds(bounds.removeFromLeft(55).reduced(pad, pad));

    // Direct widgets on the right (right to left)
    transport_->setBounds(bounds.removeFromRight(120).reduced(pad, pad));
    instrumentCombo_->setBounds(bounds.removeFromRight(90).reduced(pad, pad));
    instrumentLabel_->setBounds(bounds.removeFromRight(35).reduced(pad, pad));
    voiceSelector_->setBounds(bounds.removeFromRight(200).reduced(pad, pad));
}

// ============================================================================
// Pattern menu
// ============================================================================
void CommandBar::showPatternMenu()
{
    juce::PopupMenu menu;

    juce::String barsLabel = (currentBars_ == 1) ? "1 bar" : juce::String(currentBars_) + " bars";
    menu.addItem(juce::PopupMenu::Item("Measures: " + barsLabel).setEnabled(false));
    menu.addItem(100, "Add Measure");
    menu.addItem(101, "Remove Measure");
    menu.addItem(102, "Double Measures (x2)");
    menu.addItem(103, "Halve Measures (/2)");
    menu.addSeparator();
    menu.addItem(104, "Clear Pattern");
    menu.addSeparator();

    // Grid Size submenu
    juce::PopupMenu gridMenu;
    gridMenu.addItem(200, "1/4",  true, gridSizeId_ == 1);
    gridMenu.addItem(201, "1/8",  true, gridSizeId_ == 2);
    gridMenu.addItem(202, "1/16", true, gridSizeId_ == 3);
    gridMenu.addItem(203, "1/32", true, gridSizeId_ == 4);
    menu.addSubMenu("Grid Size", gridMenu);

    menu.addItem(210, "Step Record", true, stepRecordEnabled_);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(patternMenuBtn_),
        [this](int result) {
            auto* model = engine_.getActivePatternModel();
            switch (result)
            {
                case 100:
                    MeasureControls::addMeasure(model);
                    updateMeasuresLabel();
                    if (onMeasuresChanged) onMeasuresChanged();
                    break;
                case 101:
                    MeasureControls::subtractMeasure(model);
                    updateMeasuresLabel();
                    if (onMeasuresChanged) onMeasuresChanged();
                    break;
                case 102:
                    MeasureControls::doubleMeasures(model);
                    updateMeasuresLabel();
                    if (onMeasuresChanged) onMeasuresChanged();
                    break;
                case 103:
                    MeasureControls::halveMeasures(model);
                    updateMeasuresLabel();
                    if (onMeasuresChanged) onMeasuresChanged();
                    break;
                case 104:
                    MeasureControls::clearPattern(model);
                    if (onClearPattern) onClearPattern();
                    break;
                case 200: gridSizeId_ = 1; if (onGridSizeChanged) onGridSizeChanged(1.0);   break;
                case 201: gridSizeId_ = 2; if (onGridSizeChanged) onGridSizeChanged(0.5);   break;
                case 202: gridSizeId_ = 3; if (onGridSizeChanged) onGridSizeChanged(0.25);  break;
                case 203: gridSizeId_ = 4; if (onGridSizeChanged) onGridSizeChanged(0.125); break;
                case 210:
                    stepRecordEnabled_ = !stepRecordEnabled_;
                    if (onStepRecordChanged) onStepRecordChanged(stepRecordEnabled_);
                    break;
            }
        });
}

// ============================================================================
// Edit menu
// ============================================================================
void CommandBar::showEditMenu()
{
    juce::PopupMenu menu;

    menu.addItem(300, "Undo  " + juce::String(juce::CharPointer_UTF8("\xe2\x8c\x98")) + "Z");
    menu.addItem(301, "Redo  " + juce::String(juce::CharPointer_UTF8("\xe2\x8c\x98\xe2\x87\xa7")) + "Z");
    menu.addSeparator();
    menu.addItem(302, "Save Voice Preset...");
    menu.addItem(303, "Load Voice Preset...");
    menu.addSeparator();

    // Tempo submenu
    juce::String tempoStr = juce::String(static_cast<int>(currentTempo_)) + " BPM";
    juce::PopupMenu tempoMenu;
    for (int bpm : {60, 80, 100, 120, 140, 160, 180, 200})
    {
        tempoMenu.addItem(400 + bpm, juce::String(bpm),
                           true, static_cast<int>(currentTempo_) == bpm);
    }
    tempoMenu.addSeparator();
    tempoMenu.addItem(699, "Custom...");
    menu.addSubMenu("Tempo: " + tempoStr, tempoMenu);

    // Tempo multiplier submenu
    juce::PopupMenu multMenu;
    multMenu.addItem(700, "4x",   true, tempoMultiplierId_ == 1);
    multMenu.addItem(701, "2x",   true, tempoMultiplierId_ == 2);
    multMenu.addItem(702, "1x",   true, tempoMultiplierId_ == 3);
    multMenu.addItem(703, "1/2",  true, tempoMultiplierId_ == 4);
    multMenu.addItem(704, "1/4",  true, tempoMultiplierId_ == 5);
    multMenu.addItem(705, "1/8",  true, tempoMultiplierId_ == 6);
    multMenu.addItem(706, "1/16", true, tempoMultiplierId_ == 7);
    menu.addSubMenu("Tempo Multiplier", multMenu);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(editMenuBtn_),
        [this](int result) {
            if (result == 300)
            {
                auto& um = engine_.getUndoManager();
                if (um.canUndo()) um.undo();
            }
            else if (result == 301)
            {
                auto& um = engine_.getUndoManager();
                if (um.canRedo()) um.redo();
            }
            else if (result == 302)
            {
                if (onSavePreset) onSavePreset();
            }
            else if (result == 303)
            {
                if (onLoadPreset) onLoadPreset();
            }
            else if (result >= 400 && result < 699)
            {
                int bpm = result - 400;
                currentTempo_ = static_cast<double>(bpm);
                engine_.getProjectModel().setTempo(currentTempo_);
            }
            else if (result == 699)
            {
                // Custom tempo dialog
                auto* aw = new juce::AlertWindow("Set Tempo", "Enter BPM (20-300):",
                                                  juce::MessageBoxIconType::NoIcon);
                aw->addTextEditor("bpm", juce::String(static_cast<int>(currentTempo_)), "BPM:");
                aw->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                aw->enterModalState(true, juce::ModalCallbackFunction::create(
                    [this, aw](int result) {
                        if (result == 1)
                        {
                            double bpm = aw->getTextEditorContents("bpm").getDoubleValue();
                            bpm = std::clamp(bpm, 20.0, 300.0);
                            currentTempo_ = bpm;
                            engine_.getProjectModel().setTempo(currentTempo_);
                        }
                        delete aw;
                    }), true);
            }
            else if (result >= 700 && result <= 706)
            {
                tempoMultiplierId_ = result - 700 + 1;
                double multiplier = 1.0;
                switch (tempoMultiplierId_)
                {
                    case 1: multiplier = 4.0;    break;
                    case 2: multiplier = 2.0;    break;
                    case 3: multiplier = 1.0;    break;
                    case 4: multiplier = 0.5;    break;
                    case 5: multiplier = 0.25;   break;
                    case 6: multiplier = 0.125;  break;
                    case 7: multiplier = 0.0625; break;
                }
                int activeVoice = engine_.getActiveVoice();
                engine_.getProject().voices[activeVoice].pendingTempoMultiplier.store(multiplier);
                if (!engine_.isPlaying())
                    engine_.getProject().voices[activeVoice].tempoMultiplier.store(multiplier);
            }
        });
}

// ============================================================================
// Scale menu
// ============================================================================
void CommandBar::showScaleMenu()
{
    juce::PopupMenu menu;

    // Root note submenu
    juce::PopupMenu rootMenu;
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for (int i = 0; i < 12; ++i)
        rootMenu.addItem(800 + i, noteNames[i], true, scaleRootId_ == i + 1);
    menu.addSubMenu("Root Note", rootMenu);

    // Scale type submenu
    juce::PopupMenu typeMenu;
    const char* scaleNames[] = {
        "Chromatic", "Major", "Minor", "Harmonic Min", "Melodic Min",
        "Pentatonic", "Pent. Minor", "Blues", "Dorian", "Phrygian",
        "Lydian", "Mixolydian", "Locrian"
    };
    for (int i = 0; i < 13; ++i)
        typeMenu.addItem(900 + i, scaleNames[i], true, scaleTypeId_ == i + 1);
    menu.addSubMenu("Type", typeMenu);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(scaleMenuBtn_),
        [this](int result) {
            if (result >= 800 && result < 812)
            {
                scaleRootId_ = result - 800 + 1;
            }
            else if (result >= 900 && result < 913)
            {
                scaleTypeId_ = result - 900 + 1;
            }
            else
            {
                return;
            }

            int root = scaleRootId_ - 1;
            ScaleType type = ScaleType::Chromatic;
            switch (scaleTypeId_)
            {
                case 1:  type = ScaleType::Chromatic; break;
                case 2:  type = ScaleType::Major; break;
                case 3:  type = ScaleType::NaturalMinor; break;
                case 4:  type = ScaleType::HarmonicMinor; break;
                case 5:  type = ScaleType::MelodicMinor; break;
                case 6:  type = ScaleType::Pentatonic; break;
                case 7:  type = ScaleType::PentatonicMinor; break;
                case 8:  type = ScaleType::Blues; break;
                case 9:  type = ScaleType::Dorian; break;
                case 10: type = ScaleType::Phrygian; break;
                case 11: type = ScaleType::Lydian; break;
                case 12: type = ScaleType::Mixolydian; break;
                case 13: type = ScaleType::Locrian; break;
            }
            if (onScaleChanged)
                onScaleChanged(root, type);
        });
}

// ============================================================================
// View menu
// ============================================================================
void CommandBar::showViewMenu()
{
    juce::PopupMenu menu;

    menu.addItem(1001, "MIDI Learn", true, midiLearnActive_);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(viewMenuBtn_),
        [this](int result) {
            if (result == 1001)
            {
                midiLearnActive_ = !midiLearnActive_;
                auto& mapping = engine_.getMidiMappingEngine();
                if (midiLearnActive_)
                {
                    int v = engine_.getActiveVoice();
                    auto target = static_cast<MappingTarget>(
                        static_cast<int>(MappingTarget::Voice1Volume) + v);
                    mapping.startLearn(target);
                    mapping.onMappingsChanged = [this]() {
                        juce::MessageManager::callAsync([this]() {
                            midiLearnActive_ = false;
                        });
                    };
                }
                else
                {
                    mapping.cancelLearn();
                }
            }
        });
}

// ============================================================================
// State updates
// ============================================================================
void CommandBar::updateMeasuresLabel()
{
    auto* model = engine_.getActivePatternModel();
    if (model)
        currentBars_ = model->getBars();
}

void CommandBar::updateInstrumentSelector()
{
    int activeVoice = engine_.getActiveVoice();
    auto type = engine_.getInstrumentType(activeVoice);
    int comboId = 1;
    switch (type)
    {
        case SurgeBox::InstrumentType::SurgeXT: comboId = 1; break;
        case SurgeBox::InstrumentType::Dexed: comboId = 2; break;
        case SurgeBox::InstrumentType::TR808: comboId = 3; break;
        case SurgeBox::InstrumentType::Unknown: comboId = 1; break;
    }
    instrumentCombo_->setSelectedId(comboId, juce::dontSendNotification);
}

void CommandBar::updateTempoMultiplier(double multiplier)
{
    if (multiplier >= 3.5) tempoMultiplierId_ = 1;
    else if (multiplier >= 1.5) tempoMultiplierId_ = 2;
    else if (multiplier >= 0.75) tempoMultiplierId_ = 3;
    else if (multiplier >= 0.375) tempoMultiplierId_ = 4;
    else if (multiplier >= 0.1875) tempoMultiplierId_ = 5;
    else if (multiplier >= 0.09) tempoMultiplierId_ = 6;
    else tempoMultiplierId_ = 7;
}

void CommandBar::setTempo(double bpm)
{
    currentTempo_ = bpm;
}

} // namespace SurgeBox
