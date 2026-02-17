/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "GrooveboxProject.h"

namespace SurgeBox
{

// ValueTree identifiers for project-level state
namespace ProjectIDs
{
inline const juce::Identifier Project{"Project"};
inline const juce::Identifier Voice{"Voice"};
inline const juce::Identifier tempo{"tempo"};
inline const juce::Identifier masterVolume{"masterVolume"};
inline const juce::Identifier voiceVolume{"voiceVolume"};
inline const juce::Identifier voicePan{"voicePan"};
inline const juce::Identifier voiceSendA{"voiceSendA"};
inline const juce::Identifier voiceSendB{"voiceSendB"};
inline const juce::Identifier voiceMute{"voiceMute"};
inline const juce::Identifier voiceSolo{"voiceSolo"};
inline const juce::Identifier voiceName{"voiceName"};
} // namespace ProjectIDs

/**
 * ProjectModel wraps non-pattern project state in a ValueTree for undo/redo support.
 * Mirrors PatternModel's approach: all changes go through the UndoManager,
 * and an onProjectChanged callback auto-syncs values to GrooveboxProject
 * so the audio thread reads from the same place it always has.
 */
class ProjectModel : public juce::ValueTree::Listener
{
  public:
    explicit ProjectModel(juce::UndoManager *undoManager);
    ~ProjectModel() override;

    void setProject(GrooveboxProject *project);

    // Tempo
    double getTempo() const;
    void setTempo(double bpm);

    // Master volume
    float getMasterVolume() const;
    void setMasterVolume(float vol);

    // Per-voice properties
    float getVoiceVolume(int voice) const;
    void setVoiceVolume(int voice, float vol);

    float getVoicePan(int voice) const;
    void setVoicePan(int voice, float pan);

    float getVoiceSendA(int voice) const;
    void setVoiceSendA(int voice, float level);

    float getVoiceSendB(int voice) const;
    void setVoiceSendB(int voice, float level);

    bool getVoiceMute(int voice) const;
    void setVoiceMute(int voice, bool mute);

    bool getVoiceSolo(int voice) const;
    void setVoiceSolo(int voice, bool solo);

    juce::String getVoiceName(int voice) const;
    void setVoiceName(int voice, const juce::String &name);

    // Load current project state into the ValueTree (e.g. after project load)
    void syncFromProject();

    // Direct ValueTree access
    juce::ValueTree &getValueTree() { return tree_; }

    // Callback for UI updates
    std::function<void()> onProjectChanged;

  protected:
    void valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}

  private:
    juce::ValueTree getVoiceTree(int voice) const;

    juce::ValueTree tree_;
    juce::UndoManager *undoManager_{nullptr};
    GrooveboxProject *project_{nullptr};

    // Sync a single property change to the project
    void syncToProject(juce::ValueTree &tree, const juce::Identifier &property);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectModel)
};

} // namespace SurgeBox
