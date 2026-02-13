/**
 * FastMath.h - Optimized math utilities for real-time audio DSP
 *
 * These functions trade some accuracy for speed, suitable for:
 * - Mobile devices where CPU is limited
 * - Per-sample calculations in tight loops
 * - Anywhere std::pow/exp is too slow
 *
 * Accuracy: ~0.1-0.5% error, inaudible for most audio applications
 */

#pragma once

#include <cstdint>
#include <algorithm>

namespace anysynth {

// ============================================================================
// Fast Power Functions
// ============================================================================

/**
 * Fast approximation of 2^x using bit manipulation
 * Accurate to ~0.1% for x in [-16, 16]
 *
 * Usage: frequency calculations, exponential envelopes
 */
inline float fastPow2(float x) {
    x = std::max(-16.0f, std::min(16.0f, x));

    int i = static_cast<int>(x);
    if (x < 0) i--;
    float f = x - static_cast<float>(i);

    // Polynomial approximation for 2^f where f is in [0, 1]
    float p = 1.0f + f * (0.6931472f + f * (0.2402265f + f * 0.0550893f));

    // Combine with integer power using bit manipulation
    union { float f; int32_t i; } u;
    u.i = (i + 127) << 23;
    return u.f * p;
}

/**
 * Fast MIDI note to frequency conversion
 * Uses fastPow2 instead of std::pow
 *
 * Usage: voice.osc.setFrequency(fastNoteToFreq(midiNote + pitchMod))
 */
inline float fastNoteToFreq(float note) {
    return 440.0f * fastPow2((note - 69.0f) * 0.0833333f);  // 1/12
}

/**
 * Fast dB to linear gain conversion
 *
 * Usage: float gain = fastDbToLinear(volumeDb)
 */
inline float fastDbToLinear(float db) {
    return fastPow2(db * 0.166096f);  // log2(10)/20
}

/**
 * Fast linear to dB conversion
 *
 * Usage: float db = fastLinearToDb(gain)
 */
inline float fastLinearToDb(float linear) {
    if (linear <= 0.0f) return -100.0f;
    // log2(x) * 20 / log2(10) = log2(x) * 6.0206
    union { float f; int32_t i; } u;
    u.f = linear;
    float log2 = static_cast<float>((u.i >> 23) - 127) +
                 static_cast<float>(u.i & 0x7FFFFF) / 8388608.0f;
    return log2 * 6.0206f;
}

// ============================================================================
// Denormal Prevention
// ============================================================================

/**
 * Flush denormal numbers to zero
 * Prevents CPU spikes during decay tails in filters/envelopes
 *
 * Usage: output = flushDenormal(filter.process(input))
 */
inline float flushDenormal(float x) {
    return x + 1e-18f - 1e-18f;
}

/**
 * Anti-denormal constant - add to filter feedback paths
 *
 * Usage: filterOut = filter.process(input + ANTI_DENORMAL)
 */
constexpr float ANTI_DENORMAL = 1e-20f;

/**
 * Check if a value is denormal (for debugging)
 */
inline bool isDenormal(float x) {
    union { float f; int32_t i; } u;
    u.f = x;
    int32_t exp = (u.i >> 23) & 0xFF;
    return exp == 0 && (u.i & 0x7FFFFF) != 0;
}

// ============================================================================
// Interpolation
// ============================================================================

/**
 * Linear interpolation
 */
inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

/**
 * Cubic interpolation for smoother parameter changes
 */
inline float smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

/**
 * One-pole smoothing filter for parameter changes
 * coeff should be pre-calculated: coeff = 1 - exp(-1 / (smoothTimeMs * sampleRate / 1000))
 */
inline float smooth(float current, float target, float coeff) {
    return current + coeff * (target - current);
}

// ============================================================================
// Clamping & Saturation
// ============================================================================

/**
 * Fast soft clipping (tanh approximation)
 * Warmer than hard clipping, good for overdrive
 */
inline float softClip(float x) {
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

/**
 * Hard clip to [-1, 1]
 */
inline float hardClip(float x) {
    return std::max(-1.0f, std::min(1.0f, x));
}

} // namespace anysynth
