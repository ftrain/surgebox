/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "KernelEditorPanel.h"
#include "core/SurgeBoxEngine.h"
#include "core/PatternModel.h"
#include "gui/Theme.h"

namespace SurgeBox
{

static const char *noteNames[] = {"C", "C#", "D", "D#", "E", "F",
                                   "F#", "G", "G#", "A", "A#", "B"};

static const char *scaleTypeNames[] = {
    "Chromatic", "Major", "Natural Minor", "Harmonic Minor", "Melodic Minor",
    "Pentatonic", "Pentatonic Minor", "Blues", "Dorian", "Phrygian",
    "Lydian", "Mixolydian", "Locrian"};

static juce::String pitchToName(int pitch)
{
    int note = pitch % 12;
    int octave = pitch / 12 - 1;
    return juce::String(noteNames[note]) + juce::String(octave);
}

KernelEditorPanel::KernelEditorPanel()
{
    // Mode selector
    modeLabel_ = std::make_unique<juce::Label>("", "Mode");
    modeLabel_->setColour(juce::Label::textColourId, Theme::color(Theme::text));
    modeLabel_->setFont(juce::FontOptions(12.0f, juce::Font::bold));
    addAndMakeVisible(*modeLabel_);

    modeCombo_ = std::make_unique<juce::ComboBox>();
    modeCombo_->addItem("Off", 1);
    modeCombo_->addItem("Spawn", 2);
    modeCombo_->addItem("Transform", 3);
    modeCombo_->addItem("Invert", 4);
    modeCombo_->addItem("Retrograde", 5);
    modeCombo_->setSelectedId(1, juce::dontSendNotification);
    modeCombo_->addListener(this);
    addAndMakeVisible(*modeCombo_);

    // Preset selector
    presetLabel_ = std::make_unique<juce::Label>("", "Preset");
    presetLabel_->setColour(juce::Label::textColourId, Theme::color(Theme::text));
    presetLabel_->setFont(juce::FontOptions(12.0f, juce::Font::bold));
    addAndMakeVisible(*presetLabel_);

    presetCombo_ = std::make_unique<juce::ComboBox>();
    presetCombo_->addItem("Custom", 1);
    presetCombo_->addItem("Arpeggio Up", 2);
    presetCombo_->addItem("Arpeggio Down", 3);
    presetCombo_->addItem("Arpeggio Up/Down", 4);
    presetCombo_->addItem("Chord", 5);
    presetCombo_->addItem("Octave Double", 6);
    presetCombo_->addItem("Echo", 7);
    presetCombo_->addItem("Strum", 8);
    presetCombo_->addItem("Probability Thin", 9);
    presetCombo_->addItem("Rising Sequence", 10);
    presetCombo_->addItem("Invert Melody", 11);
    presetCombo_->setSelectedId(1, juce::dontSendNotification);
    presetCombo_->addListener(this);
    addAndMakeVisible(*presetCombo_);

    // Scale-aware toggle
    scaleAwareToggle_ = std::make_unique<juce::ToggleButton>("Scale-Aware");
    scaleAwareToggle_->setToggleState(true, juce::dontSendNotification);
    scaleAwareToggle_->addListener(this);
    addAndMakeVisible(*scaleAwareToggle_);

    // Scale root
    scaleRootLabel_ = std::make_unique<juce::Label>("", "Root");
    scaleRootLabel_->setColour(juce::Label::textColourId, Theme::color(Theme::textDim));
    scaleRootLabel_->setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(*scaleRootLabel_);

    scaleRootCombo_ = std::make_unique<juce::ComboBox>();
    for (int i = 0; i < 12; i++)
        scaleRootCombo_->addItem(noteNames[i], i + 1);
    scaleRootCombo_->setSelectedId(1, juce::dontSendNotification);
    scaleRootCombo_->addListener(this);
    addAndMakeVisible(*scaleRootCombo_);

    // Scale type
    scaleTypeLabel_ = std::make_unique<juce::Label>("", "Scale");
    scaleTypeLabel_->setColour(juce::Label::textColourId, Theme::color(Theme::textDim));
    scaleTypeLabel_->setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(*scaleTypeLabel_);

    scaleTypeCombo_ = std::make_unique<juce::ComboBox>();
    for (int i = 0; i < 13; i++)
        scaleTypeCombo_->addItem(scaleTypeNames[i], i + 1);
    scaleTypeCombo_->setSelectedId(1, juce::dontSendNotification);
    scaleTypeCombo_->addListener(this);
    addAndMakeVisible(*scaleTypeCombo_);

    // Pivot pitch (for Invert mode)
    pivotLabel_ = std::make_unique<juce::Label>("", "Pivot");
    pivotLabel_->setColour(juce::Label::textColourId, Theme::color(Theme::textDim));
    pivotLabel_->setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(*pivotLabel_);

    pivotSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                   juce::Slider::TextBoxRight);
    pivotSlider_->setRange(0, 127, 1);
    pivotSlider_->setValue(60, juce::dontSendNotification);
    pivotSlider_->setTextValueSuffix(" (" + pitchToName(60).toStdString() + ")");
    pivotSlider_->addListener(this);
    addAndMakeVisible(*pivotSlider_);

    // Accumulate semitones per iteration
    accumLabel_ = std::make_unique<juce::Label>("", "Accumulate (st/iter)");
    accumLabel_->setColour(juce::Label::textColourId, Theme::color(Theme::textDim));
    accumLabel_->setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(*accumLabel_);

    accumSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                   juce::Slider::TextBoxRight);
    accumSlider_->setRange(-24, 24, 1);
    accumSlider_->setValue(0, juce::dontSendNotification);
    accumSlider_->addListener(this);
    addAndMakeVisible(*accumSlider_);

    // Reset after N iterations
    resetLabel_ = std::make_unique<juce::Label>("", "Reset after (iters)");
    resetLabel_->setColour(juce::Label::textColourId, Theme::color(Theme::textDim));
    resetLabel_->setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(*resetLabel_);

    resetSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                   juce::Slider::TextBoxRight);
    resetSlider_->setRange(0, 32, 1);
    resetSlider_->setValue(0, juce::dontSendNotification);
    resetSlider_->setTextValueSuffix(" (0=never)");
    resetSlider_->addListener(this);
    addAndMakeVisible(*resetSlider_);

    // Seed
    seedLabel_ = std::make_unique<juce::Label>("", "Seed");
    seedLabel_->setColour(juce::Label::textColourId, Theme::color(Theme::textDim));
    seedLabel_->setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(*seedLabel_);

    seedSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                  juce::Slider::TextBoxRight);
    seedSlider_->setRange(0, 9999, 1);
    seedSlider_->setValue(0, juce::dontSendNotification);
    seedSlider_->setTextValueSuffix(" (0=random)");
    seedSlider_->addListener(this);
    addAndMakeVisible(*seedSlider_);

    // Follow chords toggle
    followChordsToggle_ = std::make_unique<juce::ToggleButton>("Follow Chords");
    followChordsToggle_->addListener(this);
    addAndMakeVisible(*followChordsToggle_);

    // Snap to chord toggle (per-pattern, separate from kernel)
    snapToChordToggle_ = std::make_unique<juce::ToggleButton>("Snap to Chord");
    snapToChordToggle_->addListener(this);
    addAndMakeVisible(*snapToChordToggle_);

    // Add cell button
    addCellButton_ = std::make_unique<juce::TextButton>("+ Add Cell");
    addCellButton_->addListener(this);
    addAndMakeVisible(*addCellButton_);
}

void KernelEditorPanel::setEngine(SurgeBoxEngine *engine)
{
    engine_ = engine;
    refreshFromPattern();
}

void KernelEditorPanel::refreshFromPattern()
{
    if (!engine_)
        return;

    loadKernelFromPattern();

    // Update mode combo
    modeCombo_->setSelectedId(static_cast<int>(editKernel_.mode) + 1, juce::dontSendNotification);
    presetCombo_->setSelectedId(1, juce::dontSendNotification); // Custom

    // Update parameter controls
    scaleAwareToggle_->setToggleState(editKernel_.scaleAware, juce::dontSendNotification);
    scaleRootCombo_->setSelectedId(editKernel_.scaleRoot + 1, juce::dontSendNotification);
    scaleTypeCombo_->setSelectedId(editKernel_.scaleType + 1, juce::dontSendNotification);
    pivotSlider_->setValue(editKernel_.pivotPitch, juce::dontSendNotification);
    accumSlider_->setValue(editKernel_.accumulateSemitones, juce::dontSendNotification);
    resetSlider_->setValue(editKernel_.resetAfterIterations, juce::dontSendNotification);
    seedSlider_->setValue(editKernel_.seed, juce::dontSendNotification);
    followChordsToggle_->setToggleState(editKernel_.followChords, juce::dontSendNotification);
    snapToChordToggle_->setToggleState(snapToChord_, juce::dontSendNotification);

    // Rebuild cell UIs to match the kernel's cells
    for (auto &cell : cellUIs_)
    {
        if (cell.pitchSlider) cell.pitchSlider->removeListener(this);
        if (cell.timeSlider) cell.timeSlider->removeListener(this);
        if (cell.velocitySlider) cell.velocitySlider->removeListener(this);
        if (cell.probabilitySlider) cell.probabilitySlider->removeListener(this);
        if (cell.removeButton) cell.removeButton->removeListener(this);
    }
    cellUIs_.clear();

    int numCells = std::min(static_cast<int>(editKernel_.cells.size()), MAX_VISIBLE_CELLS);
    for (int i = 0; i < numCells; i++)
    {
        CellUI ui;
        const auto &cell = editKernel_.cells[i];

        ui.indexLabel = std::make_unique<juce::Label>("", juce::String(i + 1));
        ui.indexLabel->setColour(juce::Label::textColourId, Theme::color(Theme::accent));
        ui.indexLabel->setFont(juce::FontOptions(11.0f, juce::Font::bold));
        ui.indexLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*ui.indexLabel);

        ui.pitchSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                         juce::Slider::TextBoxRight);
        ui.pitchSlider->setRange(-48, 48, 1);
        ui.pitchSlider->setValue(cell.pitchOffset, juce::dontSendNotification);
        ui.pitchSlider->setTextValueSuffix(" st");
        ui.pitchSlider->addListener(this);
        addAndMakeVisible(*ui.pitchSlider);

        ui.timeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                        juce::Slider::TextBoxRight);
        ui.timeSlider->setRange(-4.0, 4.0, 0.0625);
        ui.timeSlider->setValue(cell.timeOffset, juce::dontSendNotification);
        ui.timeSlider->setTextValueSuffix(" beats");
        ui.timeSlider->addListener(this);
        addAndMakeVisible(*ui.timeSlider);

        ui.velocitySlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                            juce::Slider::TextBoxRight);
        ui.velocitySlider->setRange(0.0, 1.0, 0.01);
        ui.velocitySlider->setValue(cell.velocityScale, juce::dontSendNotification);
        ui.velocitySlider->addListener(this);
        addAndMakeVisible(*ui.velocitySlider);

        ui.probabilitySlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                               juce::Slider::TextBoxRight);
        ui.probabilitySlider->setRange(0.0, 1.0, 0.01);
        ui.probabilitySlider->setValue(cell.probability, juce::dontSendNotification);
        ui.probabilitySlider->addListener(this);
        addAndMakeVisible(*ui.probabilitySlider);

        ui.removeButton = std::make_unique<juce::TextButton>("x");
        ui.removeButton->addListener(this);
        addAndMakeVisible(*ui.removeButton);

        cellUIs_.push_back(std::move(ui));
    }

    resized();
    repaint();
}

void KernelEditorPanel::loadKernelFromPattern()
{
    if (!engine_)
        return;

    int voice = engine_->getActiveVoice();
    const auto &pattern = engine_->getProject().voices[voice].pattern;
    editKernel_ = pattern.kernel;
    snapToChord_ = pattern.snapToChord;
}

void KernelEditorPanel::applyKernelToPattern()
{
    if (!engine_)
        return;

    int voice = engine_->getActiveVoice();
    engine_->getProject().voices[voice].pattern.kernel = editKernel_;
    engine_->getProject().voices[voice].pattern.snapToChord = snapToChord_;

    // Also update the PatternModel's snapToChord
    auto *model = engine_->getPatternModel(voice);
    if (model)
        model->setSnapToChord(snapToChord_);

    if (onKernelChanged)
        onKernelChanged();
}

void KernelEditorPanel::addCell()
{
    if (static_cast<int>(editKernel_.cells.size()) >= MAX_VISIBLE_CELLS)
        return;

    KernelCell cell;
    cell.pitchOffset = 0;
    cell.timeOffset = 0.0;
    cell.velocityScale = 1.0f;
    cell.probability = 1.0f;
    editKernel_.cells.push_back(cell);

    // If mode is Off and we're adding cells, switch to Spawn
    if (editKernel_.mode == KernelMode::Off)
    {
        editKernel_.mode = KernelMode::Spawn;
        modeCombo_->setSelectedId(2, juce::dontSendNotification);
    }

    applyKernelToPattern();
    refreshFromPattern();
}

void KernelEditorPanel::removeCell(int index)
{
    if (index >= 0 && index < static_cast<int>(editKernel_.cells.size()))
    {
        editKernel_.cells.erase(editKernel_.cells.begin() + index);
        applyKernelToPattern();
        refreshFromPattern();
    }
}

void KernelEditorPanel::paint(juce::Graphics &g)
{
    g.fillAll(Theme::color(Theme::background));

    // Title bar
    g.setColour(Theme::color(Theme::backgroundMid));
    g.fillRect(0, 0, getWidth(), 32);
    g.setColour(Theme::color(Theme::textBright));
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("Kernel Editor", 12, 0, 200, 32, juce::Justification::centredLeft);

    // Section separators
    g.setColour(Theme::color(Theme::border));

    // Separator after top controls
    g.drawHorizontalLine(72, 4.0f, getWidth() - 4.0f);

    // Separator before cells section
    int cellsY = 220;
    g.drawHorizontalLine(cellsY - 4, 4.0f, getWidth() - 4.0f);

    // Cell column headers
    if (!cellUIs_.empty())
    {
        g.setColour(Theme::color(Theme::textDim));
        g.setFont(juce::FontOptions(10.0f));
        int headerY = cellsY;
        int col1 = 28;
        int colW = (getWidth() - 60) / 4;
        g.drawText("Pitch", col1, headerY, colW, 16, juce::Justification::centredLeft);
        g.drawText("Time", col1 + colW, headerY, colW, 16, juce::Justification::centredLeft);
        g.drawText("Velocity", col1 + colW * 2, headerY, colW, 16, juce::Justification::centredLeft);
        g.drawText("Probability", col1 + colW * 3, headerY, colW, 16, juce::Justification::centredLeft);
    }
}

void KernelEditorPanel::resized()
{
    int pad = 8;
    int y = 36;
    int rowH = 26;
    int labelW = 60;
    int comboW = 150;
    int w = getWidth();

    // Row 1: Mode + Preset
    modeLabel_->setBounds(pad, y, labelW, rowH);
    modeCombo_->setBounds(pad + labelW, y, comboW, rowH - 2);
    presetLabel_->setBounds(pad + labelW + comboW + 16, y, labelW, rowH);
    presetCombo_->setBounds(pad + labelW * 2 + comboW + 16, y, comboW + 30, rowH - 2);

    y += rowH + 4;

    // Row 2: Snap to chord + Follow chords
    snapToChordToggle_->setBounds(pad, y, 140, rowH);
    followChordsToggle_->setBounds(pad + 148, y, 140, rowH);

    y = 76;

    // Parameters section
    int halfW = (w - pad * 3) / 2;
    int sliderH = 22;
    int paramLabelW = 130;

    // Left column
    int lx = pad;
    scaleAwareToggle_->setBounds(lx, y, 140, sliderH);
    y += sliderH + 2;

    scaleRootLabel_->setBounds(lx, y, 40, sliderH);
    scaleRootCombo_->setBounds(lx + 40, y, 80, sliderH);
    scaleTypeLabel_->setBounds(lx + 130, y, 40, sliderH);
    scaleTypeCombo_->setBounds(lx + 170, y, 130, sliderH);
    y += sliderH + 2;

    pivotLabel_->setBounds(lx, y, paramLabelW, sliderH);
    pivotSlider_->setBounds(lx + 50, y, halfW - 50, sliderH);
    y += sliderH + 2;

    // Right column parameters on same rows
    int ry = 76;
    int rx = pad + halfW + pad;

    accumLabel_->setBounds(rx, ry, paramLabelW, sliderH);
    accumSlider_->setBounds(rx + paramLabelW, ry, halfW - paramLabelW, sliderH);
    ry += sliderH + 2;

    resetLabel_->setBounds(rx, ry, paramLabelW, sliderH);
    resetSlider_->setBounds(rx + paramLabelW, ry, halfW - paramLabelW, sliderH);
    ry += sliderH + 2;

    seedLabel_->setBounds(rx, ry, paramLabelW, sliderH);
    seedSlider_->setBounds(rx + paramLabelW, ry, halfW - paramLabelW, sliderH);

    // Cells section
    int cellsY = 220;
    int cellRowH = 24;
    int cellY = cellsY + 18; // After header
    int col1 = 28;
    int colW = (w - 60) / 4;

    for (int i = 0; i < static_cast<int>(cellUIs_.size()); i++)
    {
        auto &ui = cellUIs_[i];
        ui.indexLabel->setBounds(pad, cellY, 18, cellRowH);
        ui.pitchSlider->setBounds(col1, cellY, colW - 4, cellRowH);
        ui.timeSlider->setBounds(col1 + colW, cellY, colW - 4, cellRowH);
        ui.velocitySlider->setBounds(col1 + colW * 2, cellY, colW - 4, cellRowH);
        ui.probabilitySlider->setBounds(col1 + colW * 3, cellY, colW - 28, cellRowH);
        ui.removeButton->setBounds(w - 28, cellY, 22, cellRowH);
        cellY += cellRowH + 2;
    }

    addCellButton_->setBounds(pad, cellY + 4, 100, 24);
}

void KernelEditorPanel::comboBoxChanged(juce::ComboBox *comboBox)
{
    if (comboBox == modeCombo_.get())
    {
        editKernel_.mode = static_cast<KernelMode>(comboBox->getSelectedId() - 1);
        presetCombo_->setSelectedId(1, juce::dontSendNotification); // Custom
        applyKernelToPattern();
    }
    else if (comboBox == presetCombo_.get())
    {
        int id = comboBox->getSelectedId();
        PatternKernel preset;
        switch (id)
        {
        case 2: preset = KernelPresets::arpeggioUp(); break;
        case 3: preset = KernelPresets::arpeggioDown(); break;
        case 4: preset = KernelPresets::arpeggioUpDown(); break;
        case 5: preset = KernelPresets::chord(); break;
        case 6: preset = KernelPresets::octaveDouble(); break;
        case 7: preset = KernelPresets::echo(); break;
        case 8: preset = KernelPresets::strum(); break;
        case 9: preset = KernelPresets::probabilityThin(); break;
        case 10: preset = KernelPresets::risingSequence(); break;
        case 11: preset = KernelPresets::invertMelody(); break;
        default: return; // Custom - no action
        }
        editKernel_ = preset;
        applyKernelToPattern();
        refreshFromPattern();
    }
    else if (comboBox == scaleRootCombo_.get())
    {
        editKernel_.scaleRoot = comboBox->getSelectedId() - 1;
        applyKernelToPattern();
    }
    else if (comboBox == scaleTypeCombo_.get())
    {
        editKernel_.scaleType = comboBox->getSelectedId() - 1;
        applyKernelToPattern();
    }
}

void KernelEditorPanel::sliderValueChanged(juce::Slider *slider)
{
    if (slider == pivotSlider_.get())
    {
        editKernel_.pivotPitch = static_cast<int>(slider->getValue());
        pivotSlider_->setTextValueSuffix(" (" + pitchToName(editKernel_.pivotPitch).toStdString() + ")");
        applyKernelToPattern();
        return;
    }
    if (slider == accumSlider_.get())
    {
        editKernel_.accumulateSemitones = static_cast<int>(slider->getValue());
        applyKernelToPattern();
        return;
    }
    if (slider == resetSlider_.get())
    {
        editKernel_.resetAfterIterations = static_cast<int>(slider->getValue());
        applyKernelToPattern();
        return;
    }
    if (slider == seedSlider_.get())
    {
        editKernel_.seed = static_cast<uint32_t>(slider->getValue());
        applyKernelToPattern();
        return;
    }

    // Check cell sliders
    for (int i = 0; i < static_cast<int>(cellUIs_.size()); i++)
    {
        if (i >= static_cast<int>(editKernel_.cells.size()))
            break;

        auto &ui = cellUIs_[i];
        if (slider == ui.pitchSlider.get())
        {
            editKernel_.cells[i].pitchOffset = static_cast<int>(slider->getValue());
            presetCombo_->setSelectedId(1, juce::dontSendNotification);
            applyKernelToPattern();
            return;
        }
        if (slider == ui.timeSlider.get())
        {
            editKernel_.cells[i].timeOffset = slider->getValue();
            presetCombo_->setSelectedId(1, juce::dontSendNotification);
            applyKernelToPattern();
            return;
        }
        if (slider == ui.velocitySlider.get())
        {
            editKernel_.cells[i].velocityScale = static_cast<float>(slider->getValue());
            presetCombo_->setSelectedId(1, juce::dontSendNotification);
            applyKernelToPattern();
            return;
        }
        if (slider == ui.probabilitySlider.get())
        {
            editKernel_.cells[i].probability = static_cast<float>(slider->getValue());
            presetCombo_->setSelectedId(1, juce::dontSendNotification);
            applyKernelToPattern();
            return;
        }
    }
}

void KernelEditorPanel::buttonClicked(juce::Button *button)
{
    if (button == scaleAwareToggle_.get())
    {
        editKernel_.scaleAware = button->getToggleState();
        applyKernelToPattern();
        return;
    }
    if (button == followChordsToggle_.get())
    {
        editKernel_.followChords = button->getToggleState();
        applyKernelToPattern();
        return;
    }
    if (button == snapToChordToggle_.get())
    {
        snapToChord_ = button->getToggleState();
        applyKernelToPattern();
        return;
    }
    if (button == addCellButton_.get())
    {
        addCell();
        return;
    }

    // Check remove buttons
    for (int i = 0; i < static_cast<int>(cellUIs_.size()); i++)
    {
        if (button == cellUIs_[i].removeButton.get())
        {
            removeCell(i);
            return;
        }
    }
}

} // namespace SurgeBox
