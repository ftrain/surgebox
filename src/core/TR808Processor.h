/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * TR-808 Drum Machine Processor - JUCE AudioProcessor wrapper around analog drum voices.
 * GM-compatible MIDI mapping for piano roll integration.
 */

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "drums/DrumVoices.h"
#include <array>
#include <mutex>

namespace SurgeBox
{

class TR808Editor;

class TR808Processor : public juce::AudioProcessor
{
  public:
    TR808Processor();
    ~TR808Processor() override = default;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TR-808"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String &) override {}

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    // Drum voice types
    enum DrumVoice
    {
        Kick = 0,
        Snare,
        ClosedHH,
        OpenHH,
        Clap,
        TomHi,
        TomMid,
        TomLow,
        Cymbal,
        Cowbell,
        Rimshot,
        Claves,
        Maracas,
        NUM_DRUM_VOICES
    };

    // GM MIDI note mapping
    static constexpr int midiNoteForVoice(DrumVoice v)
    {
        switch (v)
        {
            case Kick:     return 36;  // C2
            case Snare:    return 38;  // D2
            case ClosedHH: return 42;  // F#2
            case OpenHH:   return 46;  // A#2
            case Clap:     return 39;  // D#2
            case TomHi:    return 48;  // C3
            case TomMid:   return 45;  // A2
            case TomLow:   return 41;  // F2
            case Cymbal:   return 49;  // C#3
            case Cowbell:  return 56;  // G#3
            case Rimshot:  return 37;  // C#2
            case Claves:   return 75;  // D#5
            case Maracas:  return 70;  // A#4
            default:       return -1;
        }
    }

    // Reverse lookup: MIDI note → drum voice index (-1 if unmapped)
    static int drumVoiceForMidiNote(int note);

    // Human-readable name for a drum voice
    static constexpr const char* nameForVoice(DrumVoice v)
    {
        switch (v)
        {
            case Kick:     return "Kick";
            case Snare:    return "Snare";
            case ClosedHH: return "CH";
            case OpenHH:   return "OH";
            case Clap:     return "Clap";
            case TomHi:    return "HiTom";
            case TomMid:   return "MdTom";
            case TomLow:   return "LoTom";
            case Cymbal:   return "Cym";
            case Cowbell:  return "Cow";
            case Rimshot:  return "Rim";
            case Claves:   return "Clav";
            case Maracas:  return "Mar";
            default:       return "";
        }
    }

    // Lookup drum voice name by MIDI note (empty string if unmapped)
    static const char* nameForMidiNote(int note)
    {
        int idx = drumVoiceForMidiNote(note);
        if (idx >= 0 && idx < NUM_DRUM_VOICES)
            return nameForVoice(static_cast<DrumVoice>(idx));
        return "";
    }

    // Per-voice parameter access (0-1 normalized)
    struct VoiceParams
    {
        float pitch{0.5f};
        float decay{0.5f};
        float tone{0.5f};
        float drive{0.2f};  // or snappy, spread, mix depending on voice
    };

    VoiceParams &getVoiceParams(int voiceIndex);
    const VoiceParams &getVoiceParams(int voiceIndex) const;

  private:
    void handleNoteOn(int note, int velocity);
    void handleNoteOff(int note);
    void applyParams();

    double sampleRate_{44100.0};

    // Drum voice instances
    anysynth::AnalogKick kick_;
    anysynth::AnalogSnare snare_;
    anysynth::AnalogHiHat closedHH_;
    anysynth::AnalogHiHat openHH_;
    anysynth::AnalogClap clap_;
    anysynth::AnalogTom tomHi_;
    anysynth::AnalogTom tomMid_;
    anysynth::AnalogTom tomLow_;
    anysynth::AnalogCymbal cymbal_;
    anysynth::AnalogCowbell cowbell_;
    anysynth::AnalogRimshot rimshot_;
    anysynth::AnalogClaves claves_;
    anysynth::AnalogMaracas maracas_;

    std::array<VoiceParams, NUM_DRUM_VOICES> voiceParams_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TR808Processor)
};

} // namespace SurgeBox
