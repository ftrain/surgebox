/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "TR808Editor.h"
#include "core/TR808Processor.h"

namespace SurgeBox
{

static const char *drumNames[] = {
    "KICK", "SNARE", "CH", "OH", "CLAP",
    "TOM H", "TOM M", "TOM L", "CYMBAL",
    "COWBELL", "RIM", "CLAVES", "MARACAS"
};

TR808Editor::TR808Editor(TR808Processor &processor)
    : AudioProcessorEditor(&processor), tr808_(processor)
{
    for (int i = 0; i < 13; i++)
        setupPad(i, drumNames[i]);

    setSize(900, 500);
}

void TR808Editor::setupPad(int index, const juce::String &name)
{
    auto &pad = pads_[index];

    pad.nameLabel = std::make_unique<juce::Label>("", name);
    pad.nameLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    pad.nameLabel->setJustificationType(juce::Justification::centred);
    pad.nameLabel->setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(*pad.nameLabel);

    pad.button = std::make_unique<juce::TextButton>(name);
    pad.button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    pad.button->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff6644));
    pad.button->onClick = [this, index]() { triggerDrum(index); };
    addAndMakeVisible(*pad.button);

    auto makeKnob = [this, index](std::unique_ptr<juce::Slider> &knob, const juce::String &tooltip,
                                   float *paramPtr) {
        knob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                               juce::Slider::NoTextBox);
        knob->setRange(0.0, 1.0, 0.01);
        knob->setValue(*paramPtr);
        knob->setTooltip(tooltip);
        knob->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff6688cc));
        knob->onValueChange = [paramPtr, &knob_ref = *knob]() {
            *paramPtr = static_cast<float>(knob_ref.getValue());
        };
        addAndMakeVisible(*knob);
    };

    auto &params = tr808_.getVoiceParams(index);
    makeKnob(pad.pitchKnob, "Pitch", &params.pitch);
    makeKnob(pad.decayKnob, "Decay", &params.decay);
    makeKnob(pad.toneKnob, "Tone", &params.tone);
    makeKnob(pad.driveKnob, "Drive", &params.drive);
}

void TR808Editor::triggerDrum(int index)
{
    auto drumVoice = static_cast<TR808Processor::DrumVoice>(index);
    int midiNote = TR808Processor::midiNoteForVoice(drumVoice);
    if (midiNote >= 0)
    {
        // Create a MIDI note-on/off pair to trigger the drum
        juce::MidiBuffer mb;
        mb.addEvent(juce::MidiMessage::noteOn(1, midiNote, (juce::uint8)100), 0);
        juce::AudioBuffer<float> tempBuf(2, 1);
        tempBuf.clear();
        tr808_.processBlock(tempBuf, mb);
    }
}

void TR808Editor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("TR-808", getLocalBounds().removeFromTop(35), juce::Justification::centred);
}

void TR808Editor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(35); // Title

    // Layout: 4 columns, rows of pads
    int cols = 4;
    int rows = (13 + cols - 1) / cols;
    int padWidth = bounds.getWidth() / cols;
    int padHeight = bounds.getHeight() / rows;

    for (int i = 0; i < 13; i++)
    {
        int col = i % cols;
        int row = i / cols;
        auto padArea = juce::Rectangle<int>(
            bounds.getX() + col * padWidth,
            bounds.getY() + row * padHeight,
            padWidth, padHeight).reduced(4);

        auto &pad = pads_[i];

        pad.nameLabel->setBounds(padArea.removeFromTop(16));
        pad.button->setBounds(padArea.removeFromTop(40).reduced(2));

        auto knobArea = padArea.reduced(2);
        int knobWidth = knobArea.getWidth() / 4;
        int knobHeight = std::min(knobArea.getHeight(), 40);

        pad.pitchKnob->setBounds(knobArea.getX(), knobArea.getY(), knobWidth, knobHeight);
        pad.decayKnob->setBounds(knobArea.getX() + knobWidth, knobArea.getY(), knobWidth, knobHeight);
        pad.toneKnob->setBounds(knobArea.getX() + knobWidth * 2, knobArea.getY(), knobWidth, knobHeight);
        pad.driveKnob->setBounds(knobArea.getX() + knobWidth * 3, knobArea.getY(), knobWidth, knobHeight);
    }
}

} // namespace SurgeBox
