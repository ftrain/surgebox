/*
 * SurgeBox - A groovebox built on Surge XT
 * Copyright 2024, Various Authors
 *
 * Released under the GNU General Public Licence v3 or later (GPL-3.0-or-later).
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "MasterFXChain.h"
#include <cstring>

namespace SurgeBox
{

static const fxslot_positions slotPositions[NUM_GLOBAL_FX] = {
    fxslot_global1, fxslot_global2, fxslot_global3, fxslot_global4};

MasterFXChain::MasterFXChain() = default;

MasterFXChain::~MasterFXChain() { shutdown(); }

void MasterFXChain::initialize(SurgeStorage *storage, double /*sampleRate*/)
{
    storage_ = storage;

    for (int i = 0; i < NUM_GLOBAL_FX; i++)
    {
        auto &slot = slots_[i];

        // Create FxStorage for this slot
        slot.fxStorage = std::make_unique<FxStorage>(slotPositions[i]);

        // Initialize parameter IDs with simple identity mapping
        // so pd_float[i] maps to paramData[i].f
        for (int p = 0; p < n_fx_params; p++)
        {
            slot.fxStorage->p[p].id = p;
            slot.paramData[p].f = 0.0f;
            slot.paramData[p].i = 0;
        }

        // Set initial type to off
        slot.fxStorage->type.val.i = fxt_off;
        slot.currentType = fxt_off;
        slot.enabled = true;
    }
}

void MasterFXChain::shutdown()
{
    for (auto &slot : slots_)
    {
        if (slot.effect)
        {
            slot.effect->suspend();
            slot.effect.reset();
        }
        slot.fxStorage.reset();
        slot.currentType = fxt_off;
    }
    storage_ = nullptr;
}

void MasterFXChain::process(float *outputL, float *outputR, int numSamples)
{
    if (!storage_)
        return;

    // Surge effects process exactly BLOCK_SIZE samples at a time.
    // We process the audio in BLOCK_SIZE chunks.
    int processed = 0;

    while (processed < numSamples)
    {
        int chunk = std::min(BLOCK_SIZE, numSamples - processed);

        // Copy this chunk into the FX buffer
        memcpy(fxBufferL_, outputL + processed, chunk * sizeof(float));
        memcpy(fxBufferR_, outputR + processed, chunk * sizeof(float));

        // Pad with zeros if chunk < BLOCK_SIZE (effects expect full blocks)
        if (chunk < BLOCK_SIZE)
        {
            memset(fxBufferL_ + chunk, 0, (BLOCK_SIZE - chunk) * sizeof(float));
            memset(fxBufferR_ + chunk, 0, (BLOCK_SIZE - chunk) * sizeof(float));
        }

        // Process through each enabled FX slot in series
        for (int i = 0; i < NUM_GLOBAL_FX; i++)
        {
            auto &slot = slots_[i];
            if (slot.effect && slot.enabled && slot.currentType != fxt_off)
            {
                slot.effect->process(fxBufferL_, fxBufferR_);
            }
        }

        // Copy processed audio back (only the valid chunk)
        memcpy(outputL + processed, fxBufferL_, chunk * sizeof(float));
        memcpy(outputR + processed, fxBufferR_, chunk * sizeof(float));

        processed += chunk;
    }
}

void MasterFXChain::loadFromProject(const GrooveboxProject &project)
{
    if (!storage_)
        return;

    for (int i = 0; i < NUM_GLOBAL_FX; i++)
    {
        const auto &gfx = project.globalFX[i];
        auto &slot = slots_[i];

        slot.enabled = gfx.enabled;

        // Set parameter values
        for (int p = 0; p < n_fx_params && p < FX_PARAMS_PER_SLOT; p++)
            slot.paramData[p].f = gfx.params[p];

        // Change effect type if different
        if (gfx.type != slot.currentType)
            setEffectType(i, gfx.type);
    }
}

void MasterFXChain::saveToProject(GrooveboxProject &project) const
{
    for (int i = 0; i < NUM_GLOBAL_FX; i++)
    {
        auto &gfx = project.globalFX[i];
        const auto &slot = slots_[i];

        gfx.type = slot.currentType;
        gfx.enabled = slot.enabled;

        for (int p = 0; p < n_fx_params && p < FX_PARAMS_PER_SLOT; p++)
            gfx.params[p] = slot.paramData[p].f;
    }
}

void MasterFXChain::setEffectType(int slot, int fxType)
{
    if (slot < 0 || slot >= NUM_GLOBAL_FX || !storage_)
        return;

    auto &s = slots_[slot];

    // Destroy existing effect
    if (s.effect)
    {
        s.effect->suspend();
        s.effect.reset();
    }

    s.currentType = fxType;
    s.fxStorage->type.val.i = fxType;

    if (fxType == fxt_off)
        return;

    recreateEffect(slot);
}

int MasterFXChain::getEffectType(int slot) const
{
    if (slot < 0 || slot >= NUM_GLOBAL_FX)
        return fxt_off;
    return slots_[slot].currentType;
}

void MasterFXChain::setParameter(int slot, int paramIndex, float value)
{
    if (slot < 0 || slot >= NUM_GLOBAL_FX)
        return;
    if (paramIndex < 0 || paramIndex >= n_fx_params)
        return;

    slots_[slot].paramData[paramIndex].f = value;
}

float MasterFXChain::getParameter(int slot, int paramIndex) const
{
    if (slot < 0 || slot >= NUM_GLOBAL_FX)
        return 0.0f;
    if (paramIndex < 0 || paramIndex >= n_fx_params)
        return 0.0f;

    return slots_[slot].paramData[paramIndex].f;
}

void MasterFXChain::setSlotEnabled(int slot, bool enabled)
{
    if (slot < 0 || slot >= NUM_GLOBAL_FX)
        return;
    slots_[slot].enabled = enabled;
}

bool MasterFXChain::isSlotEnabled(int slot) const
{
    if (slot < 0 || slot >= NUM_GLOBAL_FX)
        return false;
    return slots_[slot].enabled;
}

void MasterFXChain::recreateEffect(int slot)
{
    auto &s = slots_[slot];

    // Create the effect using Surge's factory
    Effect *fx = spawn_effect(s.currentType, storage_, s.fxStorage.get(), s.paramData);
    if (!fx)
        return;

    s.effect.reset(fx);

    // Initialize parameter types and defaults
    s.effect->init_ctrltypes();
    s.effect->init_default_values();

    // Re-map parameter IDs after init_ctrltypes (which may change them)
    for (int p = 0; p < n_fx_params; p++)
        s.fxStorage->p[p].id = p;

    // Initialize the effect's internal state
    s.effect->init();
}

} // namespace SurgeBox
