/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "SurgeBoxEditor.h"
#include "SurgeSynthesizer.h"

SurgeBoxEditor::SurgeBoxEditor(SurgeBoxProcessor &p)
    : AudioProcessorEditor(&p), processor_(p), engine_(p.getEngine())
{
    // Set up callbacks
    engine_.onVoiceChanged = [this](int v) { onVoiceChanged(v); };

    // Create header components
    voiceSelector_ = std::make_unique<SurgeBox::VoiceSelector>();
    voiceSelector_->setEngine(&engine_);
    addAndMakeVisible(*voiceSelector_);

    transport_ = std::make_unique<SurgeBox::TransportControls>();
    transport_->setEngine(&engine_);
    addAndMakeVisible(*transport_);

    // Create piano roll
    pianoRoll_ = std::make_unique<SurgeBox::PianoRollWidget>();
    pianoRoll_->setEngine(&engine_);
    addAndMakeVisible(*pianoRoll_);

    // Initial size (will be adjusted when Surge editor is created)
    setSize(904, 620 + HEADER_HEIGHT + PIANO_ROLL_HEIGHT);

    // Start timer for UI updates
    startTimerHz(30);
}

SurgeBoxEditor::~SurgeBoxEditor()
{
    stopTimer();
    engine_.onVoiceChanged = nullptr;
}

void SurgeBoxEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    // Header background
    g.setColour(juce::Colour(0xff16213e));
    g.fillRect(0, 0, getWidth(), HEADER_HEIGHT);

    // Separator lines
    g.setColour(juce::Colour(0xff0f3460));
    g.drawHorizontalLine(HEADER_HEIGHT, 0, static_cast<float>(getWidth()));
    g.drawHorizontalLine(getHeight() - PIANO_ROLL_HEIGHT, 0, static_cast<float>(getWidth()));
}

void SurgeBoxEditor::resized()
{
    auto bounds = getLocalBounds();

    // Header
    auto header = bounds.removeFromTop(HEADER_HEIGHT);
    voiceSelector_->setBounds(header.removeFromLeft(200).reduced(4));
    transport_->setBounds(header.removeFromLeft(150).reduced(4));

    // Piano roll at bottom
    auto pianoRollArea = bounds.removeFromBottom(PIANO_ROLL_HEIGHT);
    pianoRoll_->setBounds(pianoRollArea.reduced(4));

    // Surge editor fills the rest
    if (surgeEditor_)
        surgeEditor_->setBounds(bounds);
}

void SurgeBoxEditor::timerCallback()
{
    // Update playhead position
    if (engine_.isPlaying())
        pianoRoll_->repaint();

    // Update transport display
    transport_->updateDisplay();
}

void SurgeBoxEditor::rebuildSurgeEditor()
{
    // In a full implementation, we would embed Surge's GUI here
    // For now, we'll create a placeholder that shows voice info

    surgeEditor_.reset();

    auto *synth = engine_.getActiveSynth();
    if (!synth)
        return;

    // Create a simple info panel for now
    // TODO: Embed actual SurgeGUIEditor here
    auto *infoPanel = new juce::Component();
    surgeEditor_.reset(infoPanel);

    addAndMakeVisible(*surgeEditor_);
    resized();
}

void SurgeBoxEditor::onVoiceChanged(int /*voice*/)
{
    // Update piano roll to show new voice's pattern
    pianoRoll_->setPattern(&engine_.getProject().voices[engine_.getActiveVoice()].pattern);
    pianoRoll_->repaint();

    // Rebuild Surge editor for new voice
    rebuildSurgeEditor();

    // Update voice selector
    voiceSelector_->repaint();
}
