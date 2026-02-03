/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SurgeBoxEditor.h"
#include "SurgeSynthEditor.h"

SurgeBoxEditor::SurgeBoxEditor(SurgeBoxProcessor &p)
    : AudioProcessorEditor(&p), processor_(p), engine_(p.getEngine())
{
    // Apply our dark look-and-feel
    setLookAndFeel(&lookAndFeel_);

    // Set up callbacks
    engine_.onVoiceChanged = [this](int v) { onVoiceChanged(v); };

    // Create command bar components
    voiceSelector_ = std::make_unique<SurgeBox::VoiceSelector>();
    voiceSelector_->setEngine(&engine_);
    addAndMakeVisible(*voiceSelector_);

    transport_ = std::make_unique<SurgeBox::TransportControls>();
    transport_->setEngine(&engine_);
    addAndMakeVisible(*transport_);

    // Step record button
    stepRecordButton_ = std::make_unique<juce::TextButton>("STEP");
    stepRecordButton_->addListener(this);
    stepRecordButton_->setClickingTogglesState(true);
    stepRecordButton_->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a6a));
    stepRecordButton_->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff4444));
    stepRecordButton_->setTooltip("Step Record Mode");
    addAndMakeVisible(*stepRecordButton_);

    // Measure control buttons
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

    // Grid size dropdown
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

    // Clear pattern button
    clearPatternBtn_ = std::make_unique<juce::TextButton>("CLR");
    clearPatternBtn_->addListener(this);
    clearPatternBtn_->setTooltip("Clear pattern");
    addAndMakeVisible(*clearPatternBtn_);

    // Scale picker
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

    // Tempo control
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

    // Tempo multiplier (speed control)
    tempoMultiplierCombo_ = std::make_unique<juce::ComboBox>();
    tempoMultiplierCombo_->addItem("4x", 1);
    tempoMultiplierCombo_->addItem("2x", 2);
    tempoMultiplierCombo_->addItem("1x", 3);
    tempoMultiplierCombo_->addItem("1/2", 4);
    tempoMultiplierCombo_->addItem("1/4", 5);
    tempoMultiplierCombo_->addItem("1/8", 6);
    tempoMultiplierCombo_->addItem("1/16", 7);
    tempoMultiplierCombo_->setSelectedId(3);  // Default to 1x
    tempoMultiplierCombo_->setTooltip("Playback speed multiplier");
    tempoMultiplierCombo_->addListener(this);
    addAndMakeVisible(*tempoMultiplierCombo_);

    // Create scrollable viewport for Surge editor
    surgeViewport_ = std::make_unique<juce::Viewport>();
    surgeViewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*surgeViewport_);

    // Create scrollable viewport for piano roll
    pianoRollViewport_ = std::make_unique<juce::Viewport>();
    pianoRollViewport_->setScrollBarsShown(true, true);
    addAndMakeVisible(*pianoRollViewport_);

    // Create piano roll
    pianoRoll_ = std::make_unique<SurgeBox::PianoRollWidget>();
    pianoRoll_->setEngine(&engine_);
    pianoRoll_->setPatternModel(engine_.getActivePatternModel());
    pianoRollViewport_->setViewedComponent(pianoRoll_.get(), false);

    // Create sticky piano keyboard above the viewport
    pianoKeyboard_ = std::make_unique<SurgeBox::PianoKeyboardWidget>();
    addAndMakeVisible(*pianoKeyboard_);

    // Wire up keyboard note preview callbacks
    pianoKeyboard_->onNoteOn = [this](int pitch, int velocity) {
        auto *synth = engine_.getActiveSynth();
        if (synth)
            synth->playNote(0, pitch, velocity, 0, -1);

        // In step record mode, also add a note to the pattern
        if (stepRecordEnabled_ && pianoRoll_)
            pianoRoll_->addNoteAtCurrentStep(pitch, velocity);
    };
    pianoKeyboard_->onNoteOff = [this](int pitch) {
        auto *synth = engine_.getActiveSynth();
        if (synth)
            synth->releaseNote(0, pitch, 0);
    };

    // Listen to viewport scroll changes to sync keyboard
    pianoRollViewport_->getHorizontalScrollBar().addListener(this);

    // Set initial pattern to 1 bar
    auto *model = engine_.getActivePatternModel();
    if (model)
    {
        model->setBars(1);
        updateMeasuresLabel();
    }

    // Make window resizable
    setResizable(true, true);
    setResizeLimits(800, 500, 2400, 1600);

    // Initial size
    setSize(1200, 800);

    // Build the Surge editor
    rebuildSurgeEditor();

    // Set up keyboard listener for step recording
    updateKeyboardListener();

    // Start timer for UI updates
    startTimerHz(30);

    // Enable keyboard focus for undo/redo shortcuts
    setWantsKeyboardFocus(true);
}

SurgeBoxEditor::~SurgeBoxEditor()
{
    stopTimer();
    engine_.onVoiceChanged = nullptr;

    // Clear piano keyboard callbacks to prevent access during destruction
    if (pianoKeyboard_)
    {
        pianoKeyboard_->onNoteOn = nullptr;
        pianoKeyboard_->onNoteOff = nullptr;
    }

    // Clear piano roll callbacks
    if (pianoRoll_)
    {
        pianoRoll_->onNoteOn = nullptr;
        pianoRoll_->onNoteOff = nullptr;
    }

    // Remove scroll bar listener
    pianoRollViewport_->getHorizontalScrollBar().removeListener(this);

    // Clear look-and-feel before destruction
    setLookAndFeel(nullptr);

    // Remove keyboard listener from all processors
    for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
    {
        auto *proc = processor_.getProcessor(i);
        if (proc)
            proc->midiKeyboardState.removeListener(this);
    }

    if (surgeEditor_)
    {
        surgeViewport_->setViewedComponent(nullptr, false);
        surgeEditorWrapper_.reset();
        surgeEditor_.reset();
    }

    pianoRollViewport_->setViewedComponent(nullptr, false);
}

void SurgeBoxEditor::updateKeyboardListener()
{
    for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
    {
        auto *proc = processor_.getProcessor(i);
        if (proc)
        {
            proc->midiKeyboardState.removeListener(this);
            proc->midiKeyboardState.addListener(this);
        }
    }
}

void SurgeBoxEditor::handleNoteOn(juce::MidiKeyboardState *, int /*midiChannel*/,
                                   int midiNoteNumber, float velocity)
{
    if (stepRecordEnabled_ && pianoRoll_)
    {
        int vel = static_cast<int>(velocity * 127.0f);
        pianoRoll_->addNoteAtCurrentStep(midiNoteNumber, vel);
    }
}

void SurgeBoxEditor::handleNoteOff(juce::MidiKeyboardState *, int /*midiChannel*/,
                                    int /*midiNoteNumber*/, float /*velocity*/)
{
}

void SurgeBoxEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    // Command bar background - use Surge-like gradient
    auto commandBarBounds = getCommandBarBounds();
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1e2a3a), commandBarBounds.getX(), commandBarBounds.getY(),
        juce::Colour(0xff16213e), commandBarBounds.getX(), commandBarBounds.getBottom(),
        false));
    g.fillRect(commandBarBounds);

    // Command bar top/bottom lines (Surge style)
    g.setColour(juce::Colour(0xff3a4a5a));
    g.drawHorizontalLine(commandBarBounds.getY(), 0, getWidth());
    g.setColour(juce::Colour(0xff0a1020));
    g.drawHorizontalLine(commandBarBounds.getBottom() - 1, 0, getWidth());

    // Divider bar (now above command bar)
    auto dividerBounds = getDividerBounds();
    g.setColour(juce::Colour(0xff0f3460));
    g.fillRect(dividerBounds);

    // Divider grip lines
    g.setColour(juce::Colour(0xff3a3a5a));
    int midY = dividerBounds.getCentreY();
    g.drawHorizontalLine(midY - 1, dividerBounds.getX() + 100.0f, dividerBounds.getRight() - 100.0f);
    g.drawHorizontalLine(midY + 1, dividerBounds.getX() + 100.0f, dividerBounds.getRight() - 100.0f);
}

juce::Rectangle<int> SurgeBoxEditor::getCommandBarBounds() const
{
    // Command bar is directly above piano roll
    int commandBarTop = getHeight() - pianoRollHeight_ - COMMAND_BAR_HEIGHT;
    return juce::Rectangle<int>(0, commandBarTop, getWidth(), COMMAND_BAR_HEIGHT);
}

void SurgeBoxEditor::resized()
{
    auto bounds = getLocalBounds();

    // Layout from bottom up:
    // 1. Piano roll at bottom
    // 2. Command bar above piano roll
    // 3. Divider above command bar (user-resizable)
    // 4. Surge editor at top
    pianoRollHeight_ = std::clamp(pianoRollHeight_, MIN_PIANO_ROLL_HEIGHT,
                                   bounds.getHeight() - MIN_SYNTH_HEIGHT - DIVIDER_HEIGHT - COMMAND_BAR_HEIGHT);

    auto pianoRollArea = bounds.removeFromBottom(pianoRollHeight_);
    auto commandBar = bounds.removeFromBottom(COMMAND_BAR_HEIGHT);
    auto dividerArea = bounds.removeFromBottom(DIVIDER_HEIGHT);
    (void)dividerArea;

    // Command bar layout - use fixed heights, minimal reduction
    int pad = 2;

    voiceSelector_->setBounds(commandBar.removeFromLeft(160).reduced(pad, pad));
    transport_->setBounds(commandBar.removeFromLeft(120).reduced(pad, pad));
    stepRecordButton_->setBounds(commandBar.removeFromLeft(60).reduced(pad, pad));

    // Measure controls
    commandBar.removeFromLeft(10);
    measuresHalfBtn_->setBounds(commandBar.removeFromLeft(36).reduced(pad, pad));
    measuresSubBtn_->setBounds(commandBar.removeFromLeft(36).reduced(pad, pad));
    measuresLabel_->setBounds(commandBar.removeFromLeft(60).reduced(pad, pad));
    measuresAddBtn_->setBounds(commandBar.removeFromLeft(36).reduced(pad, pad));
    measuresDoubleBtn_->setBounds(commandBar.removeFromLeft(36).reduced(pad, pad));

    // Grid size
    commandBar.removeFromLeft(10);
    gridSizeLabel_->setBounds(commandBar.removeFromLeft(40).reduced(pad, pad));
    gridSizeCombo_->setBounds(commandBar.removeFromLeft(70).reduced(pad, pad));

    // Tempo control
    commandBar.removeFromLeft(10);
    tempoLabel_->setBounds(commandBar.removeFromLeft(40).reduced(pad, pad));
    tempoSlider_->setBounds(commandBar.removeFromLeft(140).reduced(pad, pad));
    tempoMultiplierCombo_->setBounds(commandBar.removeFromLeft(55).reduced(pad, pad));

    // Clear button
    commandBar.removeFromLeft(10);
    clearPatternBtn_->setBounds(commandBar.removeFromLeft(40).reduced(pad, pad));

    // Scale picker
    commandBar.removeFromLeft(10);
    scaleLabel_->setBounds(commandBar.removeFromLeft(45).reduced(pad, pad));
    scaleRootCombo_->setBounds(commandBar.removeFromLeft(55).reduced(pad, pad));
    scaleTypeCombo_->setBounds(commandBar.removeFromLeft(100).reduced(pad, pad));

    // 3. Surge viewport fills the rest (top)
    surgeViewport_->setBounds(bounds);

    // Piano keyboard above the viewport
    auto keyboardArea = pianoRollArea.removeFromTop(PIANO_KEYBOARD_HEIGHT);
    pianoKeyboard_->setBounds(keyboardArea);

    // Viewport below keyboard
    pianoRollViewport_->setBounds(pianoRollArea);

    auto *model = engine_.getActivePatternModel();
    int bars = model ? model->getBars() : 1;
    int numNotes = pianoRoll_->getVisibleNoteCount();
    int noteWidth = 18;
    double pixelsPerBeat = pianoRoll_->getPixelsPerBeat();
    int pianoRollInternalWidth = numNotes * noteWidth;
    // Height based on pattern length (no piano key area anymore)
    int pianoRollInternalHeight = static_cast<int>(bars * 4 * pixelsPerBeat) + 10;
    pianoRoll_->setSize(pianoRollInternalWidth, pianoRollInternalHeight);

    // Sync keyboard with piano roll's visible pitches and scroll position
    pianoKeyboard_->setVisiblePitches(pianoRoll_->getVisiblePitches());
    pianoKeyboard_->setScrollOffset(pianoRollViewport_->getViewPositionX());

    // Rescale Surge editor
    updateSurgeEditorScale();

    // Update step record state
    pianoRoll_->setStepRecordEnabled(stepRecordEnabled_);
}

void SurgeBoxEditor::timerCallback()
{
    if (engine_.isPlaying())
    {
        pianoRoll_->repaint();
        // Sync playing notes to keyboard
        pianoKeyboard_->setPlayingNotes(engine_.getActivePlayingNotes());
    }
    else
    {
        pianoKeyboard_->setPlayingNotes({});
    }

    transport_->updateDisplay();
}

void SurgeBoxEditor::buttonClicked(juce::Button *button)
{
    if (button == stepRecordButton_.get())
    {
        stepRecordEnabled_ = stepRecordButton_->getToggleState();
        pianoRoll_->setStepRecordEnabled(stepRecordEnabled_);
        if (stepRecordEnabled_)
            pianoRoll_->resetStepPosition();
    }
    else if (button == measuresDoubleBtn_.get())
    {
        doubleMeasures();
    }
    else if (button == measuresHalfBtn_.get())
    {
        halveMeasures();
    }
    else if (button == measuresAddBtn_.get())
    {
        addMeasure();
    }
    else if (button == measuresSubBtn_.get())
    {
        subtractMeasure();
    }
    else if (button == clearPatternBtn_.get())
    {
        clearPattern();
    }
}

void SurgeBoxEditor::comboBoxChanged(juce::ComboBox *comboBox)
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
        pianoRoll_->setGridSize(gridSize);
    }
    else if (comboBox == scaleRootCombo_.get() || comboBox == scaleTypeCombo_.get())
    {
        int root = scaleRootCombo_->getSelectedId() - 1;  // 0-11
        SurgeBox::ScaleType type = SurgeBox::ScaleType::Chromatic;
        switch (scaleTypeCombo_->getSelectedId())
        {
            case 1: type = SurgeBox::ScaleType::Chromatic; break;
            case 2: type = SurgeBox::ScaleType::Major; break;
            case 3: type = SurgeBox::ScaleType::NaturalMinor; break;
            case 4: type = SurgeBox::ScaleType::HarmonicMinor; break;
            case 5: type = SurgeBox::ScaleType::MelodicMinor; break;
            case 6: type = SurgeBox::ScaleType::Pentatonic; break;
            case 7: type = SurgeBox::ScaleType::PentatonicMinor; break;
            case 8: type = SurgeBox::ScaleType::Blues; break;
            case 9: type = SurgeBox::ScaleType::Dorian; break;
            case 10: type = SurgeBox::ScaleType::Phrygian; break;
            case 11: type = SurgeBox::ScaleType::Lydian; break;
            case 12: type = SurgeBox::ScaleType::Mixolydian; break;
            case 13: type = SurgeBox::ScaleType::Locrian; break;
        }
        pianoRoll_->setScale(root, type);
        pianoKeyboard_->setScale(root, type);
        resized();  // Update piano roll width
    }
    else if (comboBox == tempoMultiplierCombo_.get())
    {
        double multiplier = 1.0;
        switch (tempoMultiplierCombo_->getSelectedId())
        {
            case 1: multiplier = 4.0; break;    // 4x
            case 2: multiplier = 2.0; break;    // 2x
            case 3: multiplier = 1.0; break;    // 1x
            case 4: multiplier = 0.5; break;    // 1/2
            case 5: multiplier = 0.25; break;   // 1/4
            case 6: multiplier = 0.125; break;  // 1/8
            case 7: multiplier = 0.0625; break; // 1/16
        }
        // Set as pending - will be applied at next bar boundary
        int activeVoice = engine_.getActiveVoice();
        engine_.getProject().voices[activeVoice].pendingTempoMultiplier.store(multiplier);

        // If not playing, apply immediately
        if (!engine_.isPlaying())
        {
            engine_.getProject().voices[activeVoice].tempoMultiplier.store(multiplier);
        }
    }
}

void SurgeBoxEditor::sliderValueChanged(juce::Slider *slider)
{
    if (slider == tempoSlider_.get())
    {
        engine_.getProject().tempo = static_cast<float>(tempoSlider_->getValue());
    }
}

void SurgeBoxEditor::scrollBarMoved(juce::ScrollBar *scrollBar, double /*newRangeStart*/)
{
    if (scrollBar == &pianoRollViewport_->getHorizontalScrollBar())
    {
        int scrollX = pianoRollViewport_->getViewPositionX();
        pianoKeyboard_->setScrollOffset(scrollX);
    }
}

void SurgeBoxEditor::updateMeasuresLabel()
{
    auto *model = engine_.getActivePatternModel();
    if (model)
    {
        int bars = model->getBars();
        measuresLabel_->setText(juce::String(bars) + (bars == 1 ? " bar" : " bars"),
                                juce::dontSendNotification);
    }
}

void SurgeBoxEditor::doubleMeasures()
{
    auto *model = engine_.getActivePatternModel();
    if (!model)
        return;

    int currentBars = model->getBars();
    if (currentBars >= 64)
        return;

    model->beginTransaction("Double Measures");

    // Clone existing notes to the new section
    double currentLength = currentBars * 4.0;
    int numNotes = model->getNumNotes();

    // First collect all notes to clone
    std::vector<std::tuple<double, double, int, int>> notesToClone;
    for (int i = 0; i < numNotes; ++i)
    {
        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(i, startBeat, duration, pitch, velocity);
        notesToClone.emplace_back(startBeat + currentLength, duration, pitch, velocity);
    }

    // Double the bars
    int newBars = currentBars * 2;
    model->setBars(newBars);

    // Add cloned notes
    for (const auto &[start, dur, pitch, vel] : notesToClone)
    {
        model->addNote(start, dur, pitch, vel);
    }

    updateMeasuresLabel();
    resized();
}

void SurgeBoxEditor::halveMeasures()
{
    auto *model = engine_.getActivePatternModel();
    if (!model)
        return;

    int currentBars = model->getBars();
    if (currentBars <= 1)
        return;

    model->beginTransaction("Halve Measures");

    int newBars = currentBars / 2;
    double newLength = newBars * 4.0;

    // Remove notes beyond the new length
    for (int i = model->getNumNotes() - 1; i >= 0; --i)
    {
        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(i, startBeat, duration, pitch, velocity);
        if (startBeat >= newLength)
            model->removeNote(i);
    }

    model->setBars(newBars);
    updateMeasuresLabel();
    resized();
}

void SurgeBoxEditor::addMeasure()
{
    auto *model = engine_.getActivePatternModel();
    if (!model)
        return;

    int currentBars = model->getBars();
    if (currentBars >= 64)
        return;

    model->beginTransaction("Add Measure");
    int newBars = currentBars + 1;
    model->setBars(newBars);
    updateMeasuresLabel();
    resized();
}

void SurgeBoxEditor::subtractMeasure()
{
    auto *model = engine_.getActivePatternModel();
    if (!model)
        return;

    int currentBars = model->getBars();
    if (currentBars <= 1)
        return;

    model->beginTransaction("Remove Measure");

    int newBars = currentBars - 1;
    double newLength = newBars * 4.0;

    // Remove notes beyond the new length
    for (int i = model->getNumNotes() - 1; i >= 0; --i)
    {
        double startBeat, duration;
        int pitch, velocity;
        model->getNoteAt(i, startBeat, duration, pitch, velocity);
        if (startBeat >= newLength)
            model->removeNote(i);
    }

    model->setBars(newBars);
    updateMeasuresLabel();
    resized();
}

void SurgeBoxEditor::clearPattern()
{
    auto *model = engine_.getActivePatternModel();
    if (!model)
        return;

    model->beginTransaction("Clear Pattern");
    model->clear();

    if (pianoRoll_)
        pianoRoll_->clearSelection();

    repaint();
}

bool SurgeBoxEditor::keyPressed(const juce::KeyPress &key)
{
    auto &undoManager = engine_.getUndoManager();

    if (key.isKeyCode('Z') && key.getModifiers().isCommandDown() &&
        !key.getModifiers().isShiftDown())
    {
        if (undoManager.canUndo())
        {
            undoManager.undo();
            return true;
        }
    }

    if (key.isKeyCode('Z') && key.getModifiers().isCommandDown() &&
        key.getModifiers().isShiftDown())
    {
        if (undoManager.canRedo())
        {
            undoManager.redo();
            return true;
        }
    }

    if (key.isKeyCode('Y') && key.getModifiers().isCommandDown())
    {
        if (undoManager.canRedo())
        {
            undoManager.redo();
            return true;
        }
    }

    return false;
}

juce::Rectangle<int> SurgeBoxEditor::getDividerBounds() const
{
    // Divider is above command bar (between surge editor and command bar)
    int dividerY = getHeight() - pianoRollHeight_ - COMMAND_BAR_HEIGHT - DIVIDER_HEIGHT;
    return juce::Rectangle<int>(0, dividerY, getWidth(), DIVIDER_HEIGHT);
}

void SurgeBoxEditor::mouseDown(const juce::MouseEvent &e)
{
    if (getDividerBounds().contains(e.getPosition()))
    {
        draggingDivider_ = true;
    }
}

void SurgeBoxEditor::mouseDrag(const juce::MouseEvent &e)
{
    if (draggingDivider_)
    {
        // Divider is above command bar, so calculate piano roll height accordingly
        int newPianoRollHeight = getHeight() - e.y - DIVIDER_HEIGHT / 2 - COMMAND_BAR_HEIGHT;
        pianoRollHeight_ = std::clamp(newPianoRollHeight, MIN_PIANO_ROLL_HEIGHT,
                                       getHeight() - MIN_SYNTH_HEIGHT - DIVIDER_HEIGHT - COMMAND_BAR_HEIGHT);
        resized();
        repaint();
    }
}

void SurgeBoxEditor::mouseUp(const juce::MouseEvent &)
{
    draggingDivider_ = false;
}

void SurgeBoxEditor::mouseMove(const juce::MouseEvent &e)
{
    if (getDividerBounds().contains(e.getPosition()))
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void SurgeBoxEditor::rebuildSurgeEditor()
{
    int newVoice = engine_.getActiveVoice();

    if (newVoice == currentSurgeVoice_ && surgeEditor_)
        return;

    if (surgeEditor_)
    {
        surgeViewport_->setViewedComponent(nullptr, false);
        surgeEditorWrapper_.reset();
        surgeEditor_.reset();
    }

    auto *surgeProcessor = processor_.getProcessor(newVoice);
    if (surgeProcessor)
    {
        surgeEditor_.reset(surgeProcessor->createEditor());

        if (surgeEditor_)
        {
            // Hide Surge's virtual keyboard - we have our own piano roll
            if (auto *surgeSynthEditor = dynamic_cast<SurgeSynthEditor *>(surgeEditor_.get()))
            {
                surgeSynthEditor->drawExtendedControls = false;
                surgeSynthEditor->resized();
            }

            surgeEditorWrapper_ = std::make_unique<juce::Component>();
            surgeEditorWrapper_->addAndMakeVisible(*surgeEditor_);

            surgeViewport_->setViewedComponent(surgeEditorWrapper_.get(), false);
            currentSurgeVoice_ = newVoice;

            updateSurgeEditorScale();
        }
    }
}

void SurgeBoxEditor::updateSurgeEditorScale()
{
    if (!surgeEditor_ || !surgeEditorWrapper_)
        return;

    int viewportWidth = surgeViewport_->getWidth();
    int editorWidth = surgeEditor_->getWidth();
    int editorHeight = surgeEditor_->getHeight();

    if (viewportWidth > 0 && editorWidth > 0)
    {
        float scale = static_cast<float>(viewportWidth) / static_cast<float>(editorWidth);

        surgeEditor_->setTransform(juce::AffineTransform::scale(scale));

        int scaledHeight = static_cast<int>(editorHeight * scale);
        surgeEditorWrapper_->setSize(viewportWidth, scaledHeight);

        surgeEditor_->setBounds(0, 0, editorWidth, editorHeight);
    }
}

void SurgeBoxEditor::onVoiceChanged(int voice)
{
    auto *model = engine_.getActivePatternModel();
    pianoRoll_->setPatternModel(model);

    updateMeasuresLabel();

    // Update tempo multiplier display for the new voice
    double multiplier = engine_.getProject().voices[voice].tempoMultiplier.load();
    int comboId = 3;  // Default to 1x
    if (multiplier >= 3.5) comboId = 1;       // 4x
    else if (multiplier >= 1.5) comboId = 2;  // 2x
    else if (multiplier >= 0.75) comboId = 3; // 1x
    else if (multiplier >= 0.375) comboId = 4; // 1/2
    else if (multiplier >= 0.1875) comboId = 5; // 1/4
    else if (multiplier >= 0.09) comboId = 6; // 1/8
    else comboId = 7; // 1/16
    tempoMultiplierCombo_->setSelectedId(comboId, juce::dontSendNotification);

    rebuildSurgeEditor();
    voiceSelector_->repaint();
    resized();
}
