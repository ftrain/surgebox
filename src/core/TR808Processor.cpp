/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "TR808Processor.h"
#include "gui/widgets/TR808Editor.h"

namespace SurgeBox
{

TR808Processor::TR808Processor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Set default pitches for toms
    voiceParams_[TomHi].pitch = 0.7f;
    voiceParams_[TomMid].pitch = 0.5f;
    voiceParams_[TomLow].pitch = 0.3f;

    // Open hi-hat gets longer decay
    openHH_.setOpen(true);
}

int TR808Processor::drumVoiceForMidiNote(int note)
{
    for (int i = 0; i < NUM_DRUM_VOICES; i++)
    {
        if (midiNoteForVoice(static_cast<DrumVoice>(i)) == note)
            return i;
    }
    return -1;
}

TR808Processor::VoiceParams &TR808Processor::getVoiceParams(int voiceIndex)
{
    return voiceParams_[std::clamp(voiceIndex, 0, static_cast<int>(NUM_DRUM_VOICES) - 1)];
}

const TR808Processor::VoiceParams &TR808Processor::getVoiceParams(int voiceIndex) const
{
    return voiceParams_[std::clamp(voiceIndex, 0, static_cast<int>(NUM_DRUM_VOICES) - 1)];
}

void TR808Processor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    sampleRate_ = sampleRate;
    float sr = static_cast<float>(sampleRate);

    kick_.setSampleRate(sr);
    snare_.setSampleRate(sr);
    closedHH_.setSampleRate(sr);
    openHH_.setSampleRate(sr);
    clap_.setSampleRate(sr);
    tomHi_.setSampleRate(sr);
    tomMid_.setSampleRate(sr);
    tomLow_.setSampleRate(sr);
    cymbal_.setSampleRate(sr);
    cowbell_.setSampleRate(sr);
    rimshot_.setSampleRate(sr);
    claves_.setSampleRate(sr);
    maracas_.setSampleRate(sr);

    applyParams();
}

void TR808Processor::releaseResources() {}

juce::AudioProcessorEditor *TR808Processor::createEditor()
{
    return new TR808Editor(*this);
}

void TR808Processor::applyParams()
{
    auto &kp = voiceParams_[Kick];
    kick_.setPitch(kp.pitch);
    kick_.setDecay(kp.decay);
    kick_.setTone(kp.tone);
    kick_.setDrive(kp.drive);

    auto &sp = voiceParams_[Snare];
    snare_.setPitch(sp.pitch);
    snare_.setDecay(sp.decay);
    snare_.setSnappy(sp.drive); // drive maps to snappy
    snare_.setTone(sp.tone);

    auto &chp = voiceParams_[ClosedHH];
    closedHH_.setPitch(chp.pitch);
    closedHH_.setDecay(chp.decay);
    closedHH_.setTone(chp.tone);

    auto &ohp = voiceParams_[OpenHH];
    openHH_.setPitch(ohp.pitch);
    openHH_.setDecay(ohp.decay);
    openHH_.setTone(ohp.tone);

    auto &cp = voiceParams_[Clap];
    clap_.setDecay(cp.decay);
    clap_.setTone(cp.tone);
    clap_.setSpread(cp.drive); // drive maps to spread

    auto &thp = voiceParams_[TomHi];
    tomHi_.setPitch(thp.pitch);
    tomHi_.setDecay(thp.decay);
    tomHi_.setTone(thp.tone);

    auto &tmp = voiceParams_[TomMid];
    tomMid_.setPitch(tmp.pitch);
    tomMid_.setDecay(tmp.decay);
    tomMid_.setTone(tmp.tone);

    auto &tlp = voiceParams_[TomLow];
    tomLow_.setPitch(tlp.pitch);
    tomLow_.setDecay(tlp.decay);
    tomLow_.setTone(tlp.tone);

    auto &cyp = voiceParams_[Cymbal];
    cymbal_.setPitch(cyp.pitch);
    cymbal_.setDecay(cyp.decay);
    cymbal_.setTone(cyp.tone);
    cymbal_.setMix(cyp.drive); // drive maps to mix

    auto &cbp = voiceParams_[Cowbell];
    cowbell_.setPitch(cbp.pitch);
    cowbell_.setDecay(cbp.decay);

    auto &rsp = voiceParams_[Rimshot];
    rimshot_.setPitch(rsp.pitch);
    rimshot_.setDecay(rsp.decay);

    auto &clp = voiceParams_[Claves];
    claves_.setPitch(clp.pitch);
    claves_.setDecay(clp.decay);

    auto &mp = voiceParams_[Maracas];
    maracas_.setTone(mp.tone);
    maracas_.setDecay(mp.decay);
}

void TR808Processor::handleNoteOn(int note, int velocity)
{
    float vel = static_cast<float>(velocity) / 127.0f;
    int voice = drumVoiceForMidiNote(note);

    switch (voice)
    {
        case Kick:     kick_.trigger(vel); break;
        case Snare:    snare_.trigger(vel); break;
        case ClosedHH:
            closedHH_.trigger(vel);
            openHH_.choke(); // Choke group: closed HH chokes open HH
            break;
        case OpenHH:
            openHH_.trigger(vel);
            closedHH_.choke(); // Choke group
            break;
        case Clap:     clap_.trigger(vel); break;
        case TomHi:    tomHi_.trigger(vel); break;
        case TomMid:   tomMid_.trigger(vel); break;
        case TomLow:   tomLow_.trigger(vel); break;
        case Cymbal:   cymbal_.trigger(vel); break;
        case Cowbell:  cowbell_.trigger(vel); break;
        case Rimshot:  rimshot_.trigger(vel); break;
        case Claves:   claves_.trigger(vel); break;
        case Maracas:  maracas_.trigger(vel); break;
        default: break;
    }
}

void TR808Processor::handleNoteOff(int /*note*/)
{
    // Drum voices are self-decaying, no note-off handling needed
}

void TR808Processor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    // Apply any parameter changes
    applyParams();

    int numSamples = buffer.getNumSamples();

    // Ensure stereo output
    if (buffer.getNumChannels() < 2)
        return;

    float *outL = buffer.getWritePointer(0);
    float *outR = buffer.getWritePointer(1);

    // Process sample-by-sample, triggering MIDI at correct sample positions
    auto midiIt = midiMessages.cbegin();

    for (int i = 0; i < numSamples; i++)
    {
        // Dispatch any MIDI events scheduled at this sample position
        while (midiIt != midiMessages.cend())
        {
            auto metadata = *midiIt;
            if (metadata.samplePosition > i)
                break;

            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                handleNoteOn(msg.getNoteNumber(), msg.getVelocity());
            else if (msg.isNoteOff())
                handleNoteOff(msg.getNoteNumber());

            ++midiIt;
        }

        float l = 0.0f, r = 0.0f;
        float vl, vr;

        // Sum all active drum voices
        kick_.process(vl, vr);     l += vl; r += vr;
        snare_.process(vl, vr);    l += vl; r += vr;
        closedHH_.process(vl, vr); l += vl; r += vr;
        openHH_.process(vl, vr);   l += vl; r += vr;
        clap_.process(vl, vr);     l += vl; r += vr;
        tomHi_.process(vl, vr);    l += vl; r += vr;
        tomMid_.process(vl, vr);   l += vl; r += vr;
        tomLow_.process(vl, vr);   l += vl; r += vr;
        cymbal_.process(vl, vr);   l += vl; r += vr;
        cowbell_.process(vl, vr);  l += vl; r += vr;
        rimshot_.process(vl, vr);  l += vl; r += vr;
        claves_.process(vl, vr);   l += vl; r += vr;
        maracas_.process(vl, vr);  l += vl; r += vr;

        outL[i] = l;
        outR[i] = r;
    }
}

void TR808Processor::getStateInformation(juce::MemoryBlock &destData)
{
    juce::ValueTree state("TR808State");

    for (int i = 0; i < NUM_DRUM_VOICES; i++)
    {
        juce::ValueTree voiceTree("Voice");
        voiceTree.setProperty("index", i, nullptr);
        voiceTree.setProperty("pitch", voiceParams_[i].pitch, nullptr);
        voiceTree.setProperty("decay", voiceParams_[i].decay, nullptr);
        voiceTree.setProperty("tone", voiceParams_[i].tone, nullptr);
        voiceTree.setProperty("drive", voiceParams_[i].drive, nullptr);
        state.addChild(voiceTree, -1, nullptr);
    }

    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void TR808Processor::setStateInformation(const void *data, int sizeInBytes)
{
    auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));

    if (state.isValid() && state.hasType("TR808State"))
    {
        for (int c = 0; c < state.getNumChildren(); c++)
        {
            auto voiceTree = state.getChild(c);
            int idx = voiceTree.getProperty("index", -1);
            if (idx >= 0 && idx < NUM_DRUM_VOICES)
            {
                voiceParams_[idx].pitch = voiceTree.getProperty("pitch", 0.5f);
                voiceParams_[idx].decay = voiceTree.getProperty("decay", 0.5f);
                voiceParams_[idx].tone = voiceTree.getProperty("tone", 0.5f);
                voiceParams_[idx].drive = voiceTree.getProperty("drive", 0.2f);
            }
        }
        applyParams();
    }
}

} // namespace SurgeBox
