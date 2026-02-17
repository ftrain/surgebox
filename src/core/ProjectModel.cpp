/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "ProjectModel.h"

namespace SurgeBox
{

ProjectModel::ProjectModel(juce::UndoManager *undoManager)
    : undoManager_(undoManager)
{
    tree_ = juce::ValueTree(ProjectIDs::Project);

    // Set defaults
    tree_.setProperty(ProjectIDs::tempo, 120.0, nullptr);
    tree_.setProperty(ProjectIDs::masterVolume, 0.8f, nullptr);

    // Create voice sub-trees
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        juce::ValueTree voiceTree(ProjectIDs::Voice);
        voiceTree.setProperty(ProjectIDs::voiceVolume, 1.0f, nullptr);
        voiceTree.setProperty(ProjectIDs::voicePan, 0.0f, nullptr);
        voiceTree.setProperty(ProjectIDs::voiceSendA, 0.0f, nullptr);
        voiceTree.setProperty(ProjectIDs::voiceSendB, 0.0f, nullptr);
        voiceTree.setProperty(ProjectIDs::voiceMute, false, nullptr);
        voiceTree.setProperty(ProjectIDs::voiceSolo, false, nullptr);
        voiceTree.setProperty(ProjectIDs::voiceName, juce::String("Voice ") + juce::String(i + 1), nullptr);
        tree_.appendChild(voiceTree, nullptr);
    }

    tree_.addListener(this);
}

ProjectModel::~ProjectModel()
{
    tree_.removeListener(this);
}

void ProjectModel::setProject(GrooveboxProject *project)
{
    project_ = project;
    if (project_)
        syncFromProject();
}

// --- Tempo ---

double ProjectModel::getTempo() const
{
    return tree_.getProperty(ProjectIDs::tempo, 120.0);
}

void ProjectModel::setTempo(double bpm)
{
    tree_.setProperty(ProjectIDs::tempo, bpm, undoManager_);
}

// --- Master Volume ---

float ProjectModel::getMasterVolume() const
{
    return tree_.getProperty(ProjectIDs::masterVolume, 0.8f);
}

void ProjectModel::setMasterVolume(float vol)
{
    tree_.setProperty(ProjectIDs::masterVolume, vol, undoManager_);
}

// --- Per-voice properties ---

juce::ValueTree ProjectModel::getVoiceTree(int voice) const
{
    if (voice < 0 || voice >= NUM_VOICES)
        return {};
    return tree_.getChild(voice);
}

float ProjectModel::getVoiceVolume(int voice) const
{
    auto v = getVoiceTree(voice);
    return v.isValid() ? static_cast<float>(v.getProperty(ProjectIDs::voiceVolume, 1.0f)) : 1.0f;
}

void ProjectModel::setVoiceVolume(int voice, float vol)
{
    auto v = getVoiceTree(voice);
    if (v.isValid())
        v.setProperty(ProjectIDs::voiceVolume, vol, undoManager_);
}

float ProjectModel::getVoicePan(int voice) const
{
    auto v = getVoiceTree(voice);
    return v.isValid() ? static_cast<float>(v.getProperty(ProjectIDs::voicePan, 0.0f)) : 0.0f;
}

void ProjectModel::setVoicePan(int voice, float pan)
{
    auto v = getVoiceTree(voice);
    if (v.isValid())
        v.setProperty(ProjectIDs::voicePan, pan, undoManager_);
}

float ProjectModel::getVoiceSendA(int voice) const
{
    auto v = getVoiceTree(voice);
    return v.isValid() ? static_cast<float>(v.getProperty(ProjectIDs::voiceSendA, 0.0f)) : 0.0f;
}

void ProjectModel::setVoiceSendA(int voice, float level)
{
    auto v = getVoiceTree(voice);
    if (v.isValid())
        v.setProperty(ProjectIDs::voiceSendA, level, undoManager_);
}

float ProjectModel::getVoiceSendB(int voice) const
{
    auto v = getVoiceTree(voice);
    return v.isValid() ? static_cast<float>(v.getProperty(ProjectIDs::voiceSendB, 0.0f)) : 0.0f;
}

void ProjectModel::setVoiceSendB(int voice, float level)
{
    auto v = getVoiceTree(voice);
    if (v.isValid())
        v.setProperty(ProjectIDs::voiceSendB, level, undoManager_);
}

bool ProjectModel::getVoiceMute(int voice) const
{
    auto v = getVoiceTree(voice);
    return v.isValid() ? static_cast<bool>(v.getProperty(ProjectIDs::voiceMute, false)) : false;
}

void ProjectModel::setVoiceMute(int voice, bool mute)
{
    auto v = getVoiceTree(voice);
    if (v.isValid())
        v.setProperty(ProjectIDs::voiceMute, mute, undoManager_);
}

bool ProjectModel::getVoiceSolo(int voice) const
{
    auto v = getVoiceTree(voice);
    return v.isValid() ? static_cast<bool>(v.getProperty(ProjectIDs::voiceSolo, false)) : false;
}

void ProjectModel::setVoiceSolo(int voice, bool solo)
{
    auto v = getVoiceTree(voice);
    if (v.isValid())
        v.setProperty(ProjectIDs::voiceSolo, solo, undoManager_);
}

juce::String ProjectModel::getVoiceName(int voice) const
{
    auto v = getVoiceTree(voice);
    return v.isValid() ? v.getProperty(ProjectIDs::voiceName).toString() : juce::String();
}

void ProjectModel::setVoiceName(int voice, const juce::String &name)
{
    auto v = getVoiceTree(voice);
    if (v.isValid())
        v.setProperty(ProjectIDs::voiceName, name, undoManager_);
}

// --- Sync ---

void ProjectModel::syncFromProject()
{
    if (!project_)
        return;

    // Temporarily remove listener to avoid feedback loop
    tree_.removeListener(this);

    tree_.setProperty(ProjectIDs::tempo, project_->tempo, nullptr);
    tree_.setProperty(ProjectIDs::masterVolume, project_->masterVolume, nullptr);

    for (int i = 0; i < NUM_VOICES; ++i)
    {
        auto v = tree_.getChild(i);
        if (v.isValid())
        {
            v.setProperty(ProjectIDs::voiceVolume, project_->voices[i].volume, nullptr);
            v.setProperty(ProjectIDs::voicePan, project_->voices[i].pan, nullptr);
            v.setProperty(ProjectIDs::voiceSendA, project_->voices[i].sendA, nullptr);
            v.setProperty(ProjectIDs::voiceSendB, project_->voices[i].sendB, nullptr);
            v.setProperty(ProjectIDs::voiceMute, project_->voices[i].mute, nullptr);
            v.setProperty(ProjectIDs::voiceSolo, project_->voices[i].solo, nullptr);
            v.setProperty(ProjectIDs::voiceName, juce::String(project_->voices[i].name), nullptr);
        }
    }

    tree_.addListener(this);
}

void ProjectModel::syncToProject(juce::ValueTree &tree, const juce::Identifier &property)
{
    if (!project_)
        return;

    // Project-level properties
    if (tree == tree_)
    {
        if (property == ProjectIDs::tempo)
            project_->tempo = static_cast<double>(tree.getProperty(ProjectIDs::tempo));
        else if (property == ProjectIDs::masterVolume)
            project_->masterVolume = static_cast<float>(tree.getProperty(ProjectIDs::masterVolume));
        return;
    }

    // Voice-level properties
    int voiceIndex = tree_.indexOf(tree);
    if (voiceIndex < 0 || voiceIndex >= NUM_VOICES)
        return;

    auto &voice = project_->voices[voiceIndex];

    if (property == ProjectIDs::voiceVolume)
        voice.volume = static_cast<float>(tree.getProperty(ProjectIDs::voiceVolume));
    else if (property == ProjectIDs::voicePan)
        voice.pan = static_cast<float>(tree.getProperty(ProjectIDs::voicePan));
    else if (property == ProjectIDs::voiceSendA)
        voice.sendA = static_cast<float>(tree.getProperty(ProjectIDs::voiceSendA));
    else if (property == ProjectIDs::voiceSendB)
        voice.sendB = static_cast<float>(tree.getProperty(ProjectIDs::voiceSendB));
    else if (property == ProjectIDs::voiceMute)
        voice.mute = static_cast<bool>(tree.getProperty(ProjectIDs::voiceMute));
    else if (property == ProjectIDs::voiceSolo)
        voice.solo = static_cast<bool>(tree.getProperty(ProjectIDs::voiceSolo));
    else if (property == ProjectIDs::voiceName)
        voice.name = tree.getProperty(ProjectIDs::voiceName).toString().toStdString();
}

void ProjectModel::valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property)
{
    syncToProject(tree, property);

    if (onProjectChanged)
        onProjectChanged();
}

} // namespace SurgeBox
