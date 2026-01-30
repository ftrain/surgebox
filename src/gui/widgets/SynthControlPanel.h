/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class SurgeSynthesizer;

namespace SurgeBox
{

class SurgeBoxEngine;

// Comprehensive synth control panel for one voice
class SynthControlPanel : public juce::Component,
                          public juce::Slider::Listener,
                          public juce::ComboBox::Listener
{
  public:
    SynthControlPanel();
    ~SynthControlPanel() override = default;

    void setEngine(SurgeBoxEngine *engine);
    void setVoice(int voice);
    void updateFromSynth();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void sliderValueChanged(juce::Slider *slider) override;
    void comboBoxChanged(juce::ComboBox *comboBox) override;

  private:
    void createSlider(juce::Slider &slider, const juce::String &suffix, double min, double max,
                      double defaultVal);
    void createComboBox(juce::ComboBox &box, const juce::StringArray &items);
    void updateSynthParameter(juce::Slider *slider);

    SurgeBoxEngine *engine_{nullptr};
    int voiceIndex_{0};
    SurgeSynthesizer *synth_{nullptr};

    // Oscillator section
    juce::ComboBox osc1Type_{"Osc1 Type"};
    juce::Slider osc1Level_{"Osc1 Level"};
    juce::Slider osc1Pitch_{"Osc1 Pitch"};
    juce::ComboBox osc2Type_{"Osc2 Type"};
    juce::Slider osc2Level_{"Osc2 Level"};
    juce::Slider osc2Pitch_{"Osc2 Pitch"};

    // Filter section
    juce::ComboBox filterType_{"Filter Type"};
    juce::Slider filterCutoff_{"Cutoff"};
    juce::Slider filterResonance_{"Reso"};
    juce::Slider filterEnvMod_{"EnvMod"};
    juce::Slider filterKeytrack_{"Keytrack"};

    // Amp Envelope
    juce::Slider aegAttack_{"A"};
    juce::Slider aegDecay_{"D"};
    juce::Slider aegSustain_{"S"};
    juce::Slider aegRelease_{"R"};

    // Filter Envelope
    juce::Slider fegAttack_{"A"};
    juce::Slider fegDecay_{"D"};
    juce::Slider fegSustain_{"S"};
    juce::Slider fegRelease_{"R"};

    // LFO
    juce::Slider lfoRate_{"Rate"};
    juce::ComboBox lfoShape_{"Shape"};

    // Output
    juce::Slider volume_{"Vol"};
    juce::Slider pan_{"Pan"};

    // Labels
    juce::Label voiceLabel_{"", "Voice 1"};
    juce::Label oscLabel_{"", "OSCILLATORS"};
    juce::Label filterLabel_{"", "FILTER"};
    juce::Label aegLabel_{"", "AMP ENV"};
    juce::Label fegLabel_{"", "FILTER ENV"};
    juce::Label lfoLabel_{"", "LFO"};
    juce::Label outputLabel_{"", "OUTPUT"};

    // Colors
    juce::Colour bgColor_{0xff1a1a2e};
    juce::Colour sectionColor_{0xff16213e};
    juce::Colour accentColor_{0xff00d4ff};
    juce::Colour textColor_{0xffe0e0e0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthControlPanel)
};

} // namespace SurgeBox
