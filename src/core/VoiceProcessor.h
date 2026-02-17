/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include "GrooveboxProject.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

namespace SurgeBox
{

class PlaceholderProcessor;

/**
 * VoiceProcessor is a thin wrapper that consolidates a processor instance
 * with its instrument type. Replaces the parallel arrays in SurgeBoxProcessor.
 */
class VoiceProcessor
{
  public:
    VoiceProcessor() = default;

    VoiceProcessor(std::unique_ptr<juce::AudioProcessor> proc, InstrumentType type)
        : processor_(std::move(proc)), type_(type)
    {
    }

    // Move-only
    VoiceProcessor(VoiceProcessor &&) = default;
    VoiceProcessor &operator=(VoiceProcessor &&) = default;
    VoiceProcessor(const VoiceProcessor &) = delete;
    VoiceProcessor &operator=(const VoiceProcessor &) = delete;

    juce::AudioProcessor *get() const { return processor_.get(); }
    juce::AudioProcessor *operator->() const { return processor_.get(); }
    explicit operator bool() const { return processor_ != nullptr; }

    InstrumentType getType() const { return type_; }
    void setType(InstrumentType type) { type_ = type; }

    juce::String getName() const
    {
        if (!processor_)
            return "Empty";
        return processor_->getName();
    }

    bool isPlaceholder() const;

    std::unique_ptr<juce::AudioProcessor> release()
    {
        type_ = InstrumentType::Unknown;
        return std::move(processor_);
    }

    void reset(std::unique_ptr<juce::AudioProcessor> proc, InstrumentType type)
    {
        processor_ = std::move(proc);
        type_ = type;
    }

  private:
    std::unique_ptr<juce::AudioProcessor> processor_;
    InstrumentType type_{InstrumentType::Unknown};
};

} // namespace SurgeBox
