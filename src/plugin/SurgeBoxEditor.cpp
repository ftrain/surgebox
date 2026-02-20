/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SurgeBoxEditor.h"
#include "SurgeSynthEditor.h"
#include "SurgeSynthProcessor.h"
#include "core/TR808Processor.h"
#include "gui/Theme.h"
#include "gui/Layout.h"

SurgeBoxEditor::SurgeBoxEditor(SurgeBoxProcessor& p)
    : AudioProcessorEditor(&p), processor_(p), engine_(p.getEngine())
{
    setLookAndFeel(&lookAndFeel_);

    engine_.onVoiceChanged = [this](int v) { onVoiceChanged(v); };
    engine_.onChordProgressionChanged = [this]() {
        if (pianoRoll_)
            pianoRoll_->rebuildChordShadings();
    };

    // Create command bar
    commandBar_ = std::make_unique<SurgeBox::CommandBar>(engine_);
    addAndMakeVisible(*commandBar_);

    // Set up command bar callbacks
    commandBar_->onStepRecordChanged = [this](bool enabled) {
        stepRecordEnabled_ = enabled;
        pianoRoll_->setStepRecordEnabled(enabled);
        if (enabled)
            pianoRoll_->resetStepPosition();
    };

    commandBar_->onGridSizeChanged = [this](double gridSize) {
        pianoRoll_->setGridSize(gridSize);
    };

    commandBar_->onScaleChanged = [this](int root, SurgeBox::ScaleType type) {
        pianoRoll_->setScale(root, type);
        pianoKeyboard_->setScale(root, type);
        resized();
    };

    commandBar_->onClearPattern = [this]() {
        pianoRoll_->clearSelection();
        repaint();
    };

    commandBar_->onMeasuresChanged = [this]() {
        resized();
    };

    commandBar_->getVoiceSelector().onFXSelected = [this](bool showFX) {
        showMasterFXEditor(showFX);
    };

    commandBar_->getVoiceSelector().onChordTrackSelected = [this](bool showChords) {
        showChordTrackEditor(showChords);
    };

    commandBar_->getVoiceSelector().onKernelSelected = [this](bool showKernel) {
        showKernelEditor(showKernel);
    };

    commandBar_->onSavePreset = [this]() {
        int voice = engine_.getActiveVoice();
        engine_.captureAllVoices();

        auto chooser = std::make_shared<juce::FileChooser>(
            "Save Voice Preset",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*" + juce::String(SurgeBox::VoicePreset::FILE_EXTENSION));

        chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                              juce::FileBrowserComponent::canSelectFiles,
            [this, voice, chooser](const juce::FileChooser &fc) {
                auto result = fc.getResult();
                if (result == juce::File())
                    return;

                auto path = result.getFullPathName().toStdString();
                if (result.getFileExtension().isEmpty())
                    path += SurgeBox::VoicePreset::FILE_EXTENSION;

                SurgeBox::VoicePreset::saveToFile(
                    engine_.getProject().voices[voice], voice, fs::path(path));
            });
    };

    commandBar_->onLoadPreset = [this]() {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Load Voice Preset",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*" + juce::String(SurgeBox::VoicePreset::FILE_EXTENSION));

        chooser->launchAsync(juce::FileBrowserComponent::openMode |
                              juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser &fc) {
                auto result = fc.getResult();
                if (result == juce::File())
                    return;

                int voice = engine_.getActiveVoice();
                auto &voiceState = engine_.getProject().voices[voice];
                auto oldType = voiceState.instrumentType;

                if (SurgeBox::VoicePreset::loadFromFile(
                        voiceState, fs::path(result.getFullPathName().toStdString())))
                {
                    // Switch instrument if type changed
                    if (voiceState.instrumentType != oldType)
                    {
                        if (!showingMasterFX_)
                        {
                            instrumentViewport_->setViewedComponent(nullptr, false);
                            instrumentEditorWrapper_.reset();
                            instrumentEditor_.reset();
                            currentEditorVoice_ = -1;
                        }
                        processor_.switchInstrument(voice, voiceState.instrumentType);
                    }

                    // Restore patch data to processor
                    voiceState.restoreToProcessor(processor_.getProcessor(voice));

                    // Sync pattern model
                    auto *model = engine_.getPatternModel(voice);
                    if (model)
                        model->loadFromPattern(voiceState.pattern);

                    // Sync project model
                    engine_.getProjectModel().syncFromProject();

                    // Rebuild editor
                    if (!showingMasterFX_)
                        rebuildInstrumentEditor();

                    commandBar_->updateMeasuresLabel();
                    commandBar_->updateInstrumentSelector();
                    resized();
                }
            });
    };

    commandBar_->onInstrumentChanged = [this](SurgeBox::InstrumentType type) {
        int voice = engine_.getActiveVoice();

        // Destroy the old editor BEFORE switching instruments.
        // The Surge editor's destructor accesses SurgeSynthesizer*, which
        // switchInstrument() will destroy.
        if (!showingMasterFX_)
        {
            instrumentViewport_->setViewedComponent(nullptr, false);
            instrumentEditorWrapper_.reset();
            instrumentEditor_.reset();
            currentEditorVoice_ = -1;
        }

        processor_.switchInstrument(voice, type);

        // Update drum mode / pitch restriction for the new instrument
        if (type == SurgeBox::InstrumentType::TR808)
        {
            std::vector<int> drumPitches;
            for (int i = 0; i < SurgeBox::TR808Processor::NUM_DRUM_VOICES; i++)
            {
                auto dv = static_cast<SurgeBox::TR808Processor::DrumVoice>(i);
                drumPitches.push_back(SurgeBox::TR808Processor::midiNoteForVoice(dv));
            }
            std::sort(drumPitches.begin(), drumPitches.end());
            pianoRoll_->setFixedPitches(drumPitches);
            pianoKeyboard_->setDrumMode(true);
        }
        else
        {
            pianoRoll_->setFixedPitches({});
            pianoKeyboard_->setDrumMode(false);
        }

        // Rebuild editor for the new instrument
        if (showingMasterFX_)
            showMasterFXEditor(false);
        else
            rebuildInstrumentEditor();

        resized();
    };

    // Create scrollable viewport for instrument editor
    instrumentViewport_ = std::make_unique<juce::Viewport>();
    instrumentViewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*instrumentViewport_);

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

    // Wire up keyboard note preview callbacks (instrument-agnostic via sendNoteToActiveVoice)
    pianoKeyboard_->onNoteOn = [this](int pitch, int velocity) {
        engine_.sendNoteToActiveVoice(pitch, velocity, true);

        if (stepRecordEnabled_ && pianoRoll_)
            pianoRoll_->addNoteAtCurrentStep(pitch, velocity);
    };
    pianoKeyboard_->onNoteOff = [this](int pitch) {
        engine_.sendNoteToActiveVoice(pitch, 0, false);
    };

    // Listen to viewport scroll changes to sync keyboard
    pianoRollViewport_->getHorizontalScrollBar().addListener(this);

    // Set initial pattern to 1 bar
    auto* model = engine_.getActivePatternModel();
    if (model)
    {
        model->setBars(1);
        commandBar_->updateMeasuresLabel();
    }

    setResizable(true, true);
    setResizeLimits(800, 500, 2400, 1600);
    setSize(1200, 800);

    // Sync toolbar to reflect restored engine state
    commandBar_->getVoiceSelector().selectVoice(engine_.getActiveVoice());
    commandBar_->updateInstrumentSelector();

    // Sync piano roll pitch restriction for drum machines
    {
        int v = engine_.getActiveVoice();
        if (engine_.getInstrumentType(v) == SurgeBox::InstrumentType::TR808)
        {
            std::vector<int> drumPitches;
            for (int i = 0; i < SurgeBox::TR808Processor::NUM_DRUM_VOICES; i++)
            {
                auto dv = static_cast<SurgeBox::TR808Processor::DrumVoice>(i);
                drumPitches.push_back(SurgeBox::TR808Processor::midiNoteForVoice(dv));
            }
            std::sort(drumPitches.begin(), drumPitches.end());
            pianoRoll_->setFixedPitches(drumPitches);
            pianoKeyboard_->setDrumMode(true);
        }
    }

    rebuildInstrumentEditor();
    updateKeyboardListener();

    startTimerHz(30);

    setWantsKeyboardFocus(true);
}

SurgeBoxEditor::~SurgeBoxEditor()
{
    stopTimer();
    engine_.onVoiceChanged = nullptr;
    engine_.onChordProgressionChanged = nullptr;

    if (pianoKeyboard_)
    {
        pianoKeyboard_->onNoteOn = nullptr;
        pianoKeyboard_->onNoteOff = nullptr;
    }

    if (pianoRoll_)
    {
        pianoRoll_->onNoteOn = nullptr;
        pianoRoll_->onNoteOff = nullptr;
    }

    pianoRollViewport_->getHorizontalScrollBar().removeListener(this);

    setLookAndFeel(nullptr);

    // Remove keyboard listeners from Surge processors
    for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
    {
        auto* surgeProc = processor_.getSurgeProcessor(i);
        if (surgeProc)
            surgeProc->midiKeyboardState.removeListener(this);
    }

    if (kernelEditor_)
    {
        kernelEditor_->onKernelChanged = nullptr;
    }

    if (instrumentEditor_)
    {
        instrumentViewport_->setViewedComponent(nullptr, false);
        instrumentEditorWrapper_.reset();
        instrumentEditor_.reset();
    }

    pianoRollViewport_->setViewedComponent(nullptr, false);
}

void SurgeBoxEditor::updateKeyboardListener()
{
    // Attach keyboard state listeners to Surge processors (for step recording via Surge's keyboard)
    for (int i = 0; i < SurgeBox::NUM_VOICES; i++)
    {
        auto* surgeProc = processor_.getSurgeProcessor(i);
        if (surgeProc)
        {
            surgeProc->midiKeyboardState.removeListener(this);
            surgeProc->midiKeyboardState.addListener(this);
        }
    }
}

void SurgeBoxEditor::handleNoteOn(juce::MidiKeyboardState*, int /*midiChannel*/,
                                   int midiNoteNumber, float velocity)
{
    if (stepRecordEnabled_ && pianoRoll_)
    {
        int vel = static_cast<int>(velocity * 127.0f);
        pianoRoll_->addNoteAtCurrentStep(midiNoteNumber, vel);
    }
}

void SurgeBoxEditor::handleNoteOff(juce::MidiKeyboardState*, int /*midiChannel*/,
                                    int /*midiNoteNumber*/, float /*velocity*/)
{
}

void SurgeBoxEditor::paint(juce::Graphics& g)
{
    g.fillAll(SurgeBox::Theme::color(SurgeBox::Theme::background));

    // Menu bar background (top)
    auto menuBarBounds = getCommandBarBounds();
    g.setGradientFill(juce::ColourGradient(
        SurgeBox::Theme::color(SurgeBox::Theme::commandBarTop), menuBarBounds.getX(), menuBarBounds.getY(),
        SurgeBox::Theme::color(SurgeBox::Theme::commandBarBottom), menuBarBounds.getX(), menuBarBounds.getBottom(),
        false));
    g.fillRect(menuBarBounds);

    g.setColour(juce::Colour(0xff3a4a5a));
    g.drawHorizontalLine(menuBarBounds.getY(), 0, getWidth());
    g.setColour(juce::Colour(0xff0a1020));
    g.drawHorizontalLine(menuBarBounds.getBottom() - 1, 0, getWidth());

    auto dividerBounds = getDividerBounds();
    g.setColour(SurgeBox::Theme::color(SurgeBox::Theme::divider));
    g.fillRect(dividerBounds);

    g.setColour(juce::Colour(0xff3a3a5a));
    int midY = dividerBounds.getCentreY();
    g.drawHorizontalLine(midY - 1, dividerBounds.getX() + 100.0f, dividerBounds.getRight() - 100.0f);
    g.drawHorizontalLine(midY + 1, dividerBounds.getX() + 100.0f, dividerBounds.getRight() - 100.0f);
}

juce::Rectangle<int> SurgeBoxEditor::getCommandBarBounds() const
{
    return juce::Rectangle<int>(0, 0, getWidth(), SurgeBox::Layout::MENU_BAR_HEIGHT);
}

void SurgeBoxEditor::resized()
{
    auto bounds = getLocalBounds();

    // Menu bar at top
    auto menuBarArea = bounds.removeFromTop(SurgeBox::Layout::MENU_BAR_HEIGHT);
    commandBar_->setBounds(menuBarArea);

    pianoRollHeight_ = std::clamp(pianoRollHeight_, SurgeBox::Layout::MIN_PIANO_ROLL_HEIGHT,
                                   bounds.getHeight() - SurgeBox::Layout::MIN_SYNTH_HEIGHT -
                                   SurgeBox::Layout::DIVIDER_HEIGHT);

    auto pianoRollArea = bounds.removeFromBottom(pianoRollHeight_);
    bounds.removeFromBottom(SurgeBox::Layout::DIVIDER_HEIGHT);

    instrumentViewport_->setBounds(bounds);

    auto keyboardArea = pianoRollArea.removeFromTop(SurgeBox::Layout::PIANO_KEYBOARD_HEIGHT);
    pianoKeyboard_->setBounds(keyboardArea);

    pianoRollViewport_->setBounds(pianoRollArea);

    auto* model = engine_.getActivePatternModel();
    int bars = model ? model->getBars() : 1;
    int numNotes = pianoRoll_->getVisibleNoteCount();
    int noteWidth = pianoRoll_->isDrumMode() ? 48 : 18;
    pianoRoll_->setNoteWidth(noteWidth);
    pianoKeyboard_->setNoteWidth(noteWidth);
    double pixelsPerBeat = pianoRoll_->getPixelsPerBeat();
    int pianoRollContentWidth = numNotes * noteWidth;
    int pianoRollContentHeight = static_cast<int>(bars * 4 * pixelsPerBeat) + 10;
    int pianoRollInternalWidth = std::max(pianoRollContentWidth, pianoRollArea.getWidth());
    int pianoRollInternalHeight = std::max(pianoRollContentHeight, pianoRollArea.getHeight());
    pianoRoll_->setSize(pianoRollInternalWidth, pianoRollInternalHeight);

    pianoKeyboard_->setVisiblePitches(pianoRoll_->getVisiblePitches());
    pianoKeyboard_->setScrollOffset(pianoRollViewport_->getViewPositionX());

    updateInstrumentEditorScale();

    pianoRoll_->setStepRecordEnabled(stepRecordEnabled_);
}

void SurgeBoxEditor::timerCallback()
{
    if (engine_.isPlaying())
    {
        double playhead = engine_.getActiveVoicePlayheadBeats();
        pianoRoll_->updatePlayhead(playhead);
        pianoKeyboard_->setPlayingNotes(engine_.getActivePlayingNotes());
    }
    else
    {
        pianoRoll_->hidePlayhead();
        pianoKeyboard_->setPlayingNotes({});
    }

    commandBar_->getTransportControls().updateDisplay();
}

void SurgeBoxEditor::scrollBarMoved(juce::ScrollBar* scrollBar, double /*newRangeStart*/)
{
    if (scrollBar == &pianoRollViewport_->getHorizontalScrollBar())
    {
        int scrollX = pianoRollViewport_->getViewPositionX();
        pianoKeyboard_->setScrollOffset(scrollX);
    }
}

bool SurgeBoxEditor::keyPressed(const juce::KeyPress& key)
{
    auto& undoManager = engine_.getUndoManager();

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
    int dividerY = getHeight() - pianoRollHeight_ - SurgeBox::Layout::DIVIDER_HEIGHT;
    return juce::Rectangle<int>(0, dividerY, getWidth(), SurgeBox::Layout::DIVIDER_HEIGHT);
}

void SurgeBoxEditor::mouseDown(const juce::MouseEvent& e)
{
    if (getDividerBounds().contains(e.getPosition()))
    {
        draggingDivider_ = true;
    }
}

void SurgeBoxEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingDivider_)
    {
        int newPianoRollHeight = getHeight() - e.y - SurgeBox::Layout::DIVIDER_HEIGHT / 2;
        pianoRollHeight_ = std::clamp(newPianoRollHeight, SurgeBox::Layout::MIN_PIANO_ROLL_HEIGHT,
                                       getHeight() - SurgeBox::Layout::MIN_SYNTH_HEIGHT -
                                       SurgeBox::Layout::DIVIDER_HEIGHT -
                                       SurgeBox::Layout::MENU_BAR_HEIGHT);
        resized();
        repaint();
    }
}

void SurgeBoxEditor::mouseUp(const juce::MouseEvent&)
{
    draggingDivider_ = false;
}

void SurgeBoxEditor::mouseMove(const juce::MouseEvent& e)
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

void SurgeBoxEditor::rebuildInstrumentEditor()
{
    int newVoice = engine_.getActiveVoice();

    if (newVoice == currentEditorVoice_ && instrumentEditor_)
        return;

    if (instrumentEditor_)
    {
        instrumentViewport_->setViewedComponent(nullptr, false);
        instrumentEditorWrapper_.reset();
        instrumentEditor_.reset();
    }

    auto* proc = processor_.getProcessor(newVoice);
    if (proc)
    {
        instrumentEditor_.reset(proc->createEditor());

        if (instrumentEditor_)
        {
            // For Surge XT editors, disable extended controls
            if (auto* surgeSynthEditor = dynamic_cast<SurgeSynthEditor*>(instrumentEditor_.get()))
            {
                surgeSynthEditor->drawExtendedControls = false;
                surgeSynthEditor->resized();
            }
            else
            {
                // Non-Surge editors (Dexed, etc.) should use the default JUCE
                // LookAndFeel so their custom knobs/sliders render correctly,
                // rather than inheriting SurgeBoxLookAndFeel.
                instrumentEditor_->setLookAndFeel(&juce::LookAndFeel::getDefaultLookAndFeel());
            }

            instrumentEditorWrapper_ = std::make_unique<juce::Component>();
            instrumentEditorWrapper_->addAndMakeVisible(*instrumentEditor_);

            instrumentViewport_->setViewedComponent(instrumentEditorWrapper_.get(), false);
            currentEditorVoice_ = newVoice;

            updateInstrumentEditorScale();
        }
        else
        {
            // Processor returned no editor — show placeholder message
            auto placeholder = std::make_unique<juce::Component>();
            auto* label = new juce::Label("placeholder",
                "Instrument not available\n\n" + proc->getName());
            label->setJustificationType(juce::Justification::centred);
            label->setFont(juce::Font(18.0f));
            label->setColour(juce::Label::textColourId, juce::Colours::grey);
            label->setBounds(0, 0, 600, 200);
            placeholder->addAndMakeVisible(label);
            placeholder->setSize(600, 200);

            instrumentViewport_->setViewedComponent(placeholder.release(), true);
            currentEditorVoice_ = newVoice;
        }
    }
}

void SurgeBoxEditor::updateInstrumentEditorScale()
{
    if (!instrumentEditor_ || !instrumentEditorWrapper_)
        return;

    int viewportWidth = instrumentViewport_->getWidth();
    int editorWidth = instrumentEditor_->getWidth();
    int editorHeight = instrumentEditor_->getHeight();

    if (viewportWidth > 0 && editorWidth > 0)
    {
        float scale = static_cast<float>(viewportWidth) / static_cast<float>(editorWidth);

        instrumentEditor_->setTransform(juce::AffineTransform::scale(scale));

        int scaledHeight = static_cast<int>(editorHeight * scale);
        instrumentEditorWrapper_->setSize(viewportWidth, scaledHeight);

        instrumentEditor_->setBounds(0, 0, editorWidth, editorHeight);
    }
}

void SurgeBoxEditor::onVoiceChanged(int voice)
{
    auto* model = engine_.getActivePatternModel();
    pianoRoll_->setPatternModel(model);

    // Restrict piano roll to valid drum pitches when TR-808 is active
    if (engine_.getInstrumentType(voice) == SurgeBox::InstrumentType::TR808)
    {
        std::vector<int> drumPitches;
        for (int i = 0; i < SurgeBox::TR808Processor::NUM_DRUM_VOICES; i++)
        {
            auto dv = static_cast<SurgeBox::TR808Processor::DrumVoice>(i);
            drumPitches.push_back(SurgeBox::TR808Processor::midiNoteForVoice(dv));
        }
        std::sort(drumPitches.begin(), drumPitches.end());
        pianoRoll_->setFixedPitches(drumPitches);
        pianoKeyboard_->setDrumMode(true);
    }
    else
    {
        pianoRoll_->setFixedPitches({});
        pianoKeyboard_->setDrumMode(false);
    }

    commandBar_->updateMeasuresLabel();

    double multiplier = engine_.getProject().voices[voice].tempoMultiplier.load();
    commandBar_->updateTempoMultiplier(multiplier);
    commandBar_->updateInstrumentSelector();

    // If showing special editors, keep them visible; otherwise rebuild instrument editor
    if (showingKernelEditor_ && kernelEditor_)
        kernelEditor_->refreshFromPattern();
    else if (!showingMasterFX_)
        rebuildInstrumentEditor();

    commandBar_->getVoiceSelector().selectVoice(voice);
    resized();
}

void SurgeBoxEditor::showChordTrackEditor(bool show)
{
    showingChordTrack_ = show;
    engine_.setChordTrackSelected(show);

    if (show)
    {
        // Remove instrument editor from viewport
        if (instrumentEditor_)
        {
            instrumentViewport_->setViewedComponent(nullptr, false);
            instrumentEditorWrapper_.reset();
            instrumentEditor_.reset();
            currentEditorVoice_ = -1;
        }

        // Show chord track placeholder in instrument viewport
        auto placeholder = std::make_unique<juce::Component>();
        auto* label = new juce::Label("chord-placeholder",
            "Chord Track\n\nEnter chords in the piano roll below.\nChords are auto-detected from notes.");
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(18.0f));
        label->setColour(juce::Label::textColourId, juce::Colours::grey);
        label->setBounds(0, 0, 600, 200);
        placeholder->addAndMakeVisible(label);
        placeholder->setSize(600, 200);
        instrumentViewport_->setViewedComponent(placeholder.release(), true);

        // Set piano roll to chord track model with limited range
        auto* model = engine_.getChordTrackModel();
        pianoRoll_->setPatternModel(model);

        // Use fixed pitches for chord entry range (C3-C5, 2 octaves)
        std::vector<int> chordPitches;
        for (int p = SurgeBox::ChordTrack::LOWEST_NOTE; p <= SurgeBox::ChordTrack::HIGHEST_NOTE; ++p)
            chordPitches.push_back(p);
        pianoRoll_->setFixedPitches(chordPitches);
        pianoKeyboard_->setDrumMode(false);  // Use normal key display

        commandBar_->updateMeasuresLabel();
        resized();
    }
    else
    {
        // Restore to current voice
        instrumentViewport_->setViewedComponent(nullptr, false);
        onVoiceChanged(engine_.getActiveVoice());
    }
}

void SurgeBoxEditor::showKernelEditor(bool show)
{
    showingKernelEditor_ = show;

    if (show)
    {
        // Remove instrument editor from viewport
        if (instrumentEditor_)
        {
            instrumentViewport_->setViewedComponent(nullptr, false);
            instrumentEditorWrapper_.reset();
            instrumentEditor_.reset();
            currentEditorVoice_ = -1;
        }

        // Create kernel editor and show in viewport
        if (!kernelEditor_)
        {
            kernelEditor_ = std::make_unique<SurgeBox::KernelEditorPanel>();
            kernelEditor_->setEngine(&engine_);
            kernelEditor_->onKernelChanged = [this]() {
                if (pianoRoll_)
                {
                    pianoRoll_->rebuildGhostNotes();
                    pianoRoll_->rebuildChordShadings();
                }
            };
        }

        kernelEditor_->refreshFromPattern();
        kernelEditor_->setSize(instrumentViewport_->getWidth(),
                                std::max(400, instrumentViewport_->getHeight()));
        instrumentViewport_->setViewedComponent(kernelEditor_.get(), false);
    }
    else
    {
        // Remove kernel editor
        instrumentViewport_->setViewedComponent(nullptr, false);
        kernelEditor_.reset();

        // Restore instrument editor
        rebuildInstrumentEditor();
    }
}

void SurgeBoxEditor::showMasterFXEditor(bool show)
{
    showingMasterFX_ = show;

    if (show)
    {
        // Remove instrument editor from viewport
        if (instrumentEditor_)
        {
            instrumentViewport_->setViewedComponent(nullptr, false);
            instrumentEditorWrapper_.reset();
            instrumentEditor_.reset();
            currentEditorVoice_ = -1;
        }

        // Create master FX editor and show in viewport
        if (!masterFXEditor_)
            masterFXEditor_ = std::make_unique<SurgeBox::MasterFXEditor>(engine_.getMasterFXChain());

        masterFXEditor_->refreshFromChain();
        masterFXEditor_->setSize(instrumentViewport_->getWidth(),
                                  std::max(400, instrumentViewport_->getHeight()));
        instrumentViewport_->setViewedComponent(masterFXEditor_.get(), false);
    }
    else
    {
        // Remove master FX editor
        instrumentViewport_->setViewedComponent(nullptr, false);
        masterFXEditor_.reset();

        // Restore instrument editor
        rebuildInstrumentEditor();
    }
}
