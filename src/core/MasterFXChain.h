/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#pragma once

#include "GrooveboxProject.h"
#include "SurgeStorage.h"
#include "dsp/Effect.h"
#include <array>
#include <memory>

namespace SurgeBox
{

class MasterFXChain
{
  public:
    MasterFXChain();
    ~MasterFXChain();

    // Initialize with a SurgeStorage borrowed from a Surge voice.
    // Must be called before process(). Can be called again if storage changes.
    void initialize(SurgeStorage *storage, double sampleRate);

    // Shutdown - releases all effects
    void shutdown();

    // Process audio in-place (called from audio thread between mixVoices and master volume)
    void process(float *outputL, float *outputR, int numSamples);

    // Load effect types and parameters from project state
    void loadFromProject(const GrooveboxProject &project);

    // Save current effect state back to project
    void saveToProject(GrooveboxProject &project) const;

    // Change the effect type for a slot (recreates the effect)
    void setEffectType(int slot, int fxType);

    // Get the current effect type for a slot
    int getEffectType(int slot) const;

    // Set a parameter value (0.0 - 1.0)
    void setParameter(int slot, int paramIndex, float value);

    // Get a parameter value
    float getParameter(int slot, int paramIndex) const;

    // Enable/disable a slot
    void setSlotEnabled(int slot, bool enabled);
    bool isSlotEnabled(int slot) const;

    // Check if initialized
    bool isInitialized() const { return storage_ != nullptr; }

  private:
    void recreateEffect(int slot);

    SurgeStorage *storage_{nullptr};

    struct FXSlot
    {
        std::unique_ptr<FxStorage> fxStorage;
        pdata paramData[n_fx_params];
        std::unique_ptr<Effect> effect;
        int currentType{0}; // fxt_off
        bool enabled{true};
    };

    std::array<FXSlot, NUM_GLOBAL_FX> slots_;

    // Surge effects process BLOCK_SIZE samples at a time.
    // We need intermediate buffers for block-based processing.
    alignas(16) float fxBufferL_[4096];
    alignas(16) float fxBufferR_[4096];
};

} // namespace SurgeBox
