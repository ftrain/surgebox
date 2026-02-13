/**
 * DrumVoices.h - TR-808/909 style analog drum synthesis
 *
 * Classic drum voices using simple analog-modeling techniques:
 * - Sine oscillators with pitch envelopes for tonal drums
 * - Filtered noise for snares and cymbals
 * - Metallic ring modulation for hi-hats
 *
 * All voices are self-contained and stateless after trigger.
 */

#pragma once

#include <cmath>
#include <algorithm>
#include "FastMath.h"

namespace anysynth {

// ============================================================================
// Drum Envelopes
// ============================================================================

/**
 * Simple exponential decay envelope for drums
 * Instant attack, exponential decay to zero
 */
class DrumEnvelope {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updateCoeff();
    }

    /**
     * Set decay time in seconds
     */
    void setDecay(float seconds) {
        decayTime_ = std::max(0.001f, seconds);
        updateCoeff();
    }

    /**
     * Trigger envelope from peak value (typically 1.0 or velocity)
     */
    void trigger(float level = 1.0f) {
        value_ = level;
        active_ = true;
    }

    /**
     * Force envelope to zero (for choke groups)
     */
    void choke() {
        value_ = 0.0f;
        active_ = false;
    }

    /**
     * Process one sample
     */
    float process() {
        if (!active_) return 0.0f;

        float out = value_;
        value_ *= decayCoeff_;

        // Stop when below threshold
        if (value_ < 1e-6f) {
            value_ = 0.0f;
            active_ = false;
        }

        return out;
    }

    bool isActive() const { return active_; }
    float getValue() const { return value_; }

private:
    void updateCoeff() {
        // Coefficient for exponential decay
        // After decayTime seconds, envelope should be at ~0.001 (-60dB)
        if (sampleRate_ > 0 && decayTime_ > 0) {
            decayCoeff_ = std::pow(0.001f, 1.0f / (decayTime_ * sampleRate_));
        }
    }

    float sampleRate_ = 44100.0f;
    float decayTime_ = 0.5f;
    float decayCoeff_ = 0.9999f;
    float value_ = 0.0f;
    bool active_ = false;
};

/**
 * Fast pitch envelope for kick transients
 * Sweeps from start pitch to end pitch exponentially
 */
class PitchEnvelope {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updateCoeff();
    }

    /**
     * Set pitch sweep parameters
     * @param startHz Starting frequency in Hz
     * @param endHz Ending frequency in Hz
     * @param sweepTime Time to reach end pitch in seconds
     */
    void setParams(float startHz, float endHz, float sweepTime) {
        startFreq_ = startHz;
        endFreq_ = endHz;
        sweepTime_ = std::max(0.001f, sweepTime);
        updateCoeff();
    }

    void trigger() {
        currentFreq_ = startFreq_;
        active_ = true;
    }

    /**
     * Get current frequency (call once per sample)
     */
    float getFrequency() {
        if (!active_) return endFreq_;

        float freq = currentFreq_;

        // Exponential approach to end frequency
        currentFreq_ = endFreq_ + (currentFreq_ - endFreq_) * decayCoeff_;

        // Stop when close to target
        if (std::abs(currentFreq_ - endFreq_) < 0.1f) {
            currentFreq_ = endFreq_;
            active_ = false;
        }

        return freq;
    }

    bool isActive() const { return active_; }

private:
    void updateCoeff() {
        if (sampleRate_ > 0 && sweepTime_ > 0) {
            // Reach 99% of target in sweepTime
            decayCoeff_ = std::pow(0.01f, 1.0f / (sweepTime_ * sampleRate_));
        }
    }

    float sampleRate_ = 44100.0f;
    float startFreq_ = 150.0f;
    float endFreq_ = 50.0f;
    float sweepTime_ = 0.05f;
    float decayCoeff_ = 0.999f;
    float currentFreq_ = 50.0f;
    bool active_ = false;
};

// ============================================================================
// Analog Kick Drum
// ============================================================================

/**
 * TR-808 style analog kick drum
 *
 * Synthesis: Sine oscillator with pitch envelope + soft saturation
 * Controls: Pitch, Decay, Tone (pitch sweep amount), Drive
 */
class AnalogKick {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        ampEnv_.setSampleRate(sampleRate);
        pitchEnv_.setSampleRate(sampleRate);
        updateParams();
    }

    // Parameters (0-1 normalized)
    void setPitch(float normalized) {
        basePitch_ = normalized;
        updateParams();
    }

    void setDecay(float normalized) {
        decay_ = normalized;
        updateParams();
    }

    void setTone(float normalized) {
        tone_ = normalized;
        updateParams();
    }

    void setDrive(float normalized) {
        drive_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void trigger(float velocity) {
        velocity_ = velocity;
        phase_ = 0.0f;
        ampEnv_.trigger(velocity);
        pitchEnv_.trigger();
    }

    void choke() {
        ampEnv_.choke();
    }

    /**
     * Process one sample, output to left and right
     */
    void process(float& left, float& right) {
        if (!ampEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        // Get current pitch from envelope
        float freq = pitchEnv_.getFrequency();

        // Phase accumulation
        float phaseInc = freq / sampleRate_;
        phase_ += phaseInc;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        // Sine oscillator
        float osc = std::sin(phase_ * 2.0f * M_PI);

        // Soft saturation for analog warmth
        if (drive_ > 0.0f) {
            float driveAmount = 1.0f + drive_ * 4.0f;
            osc = std::tanh(osc * driveAmount) / std::tanh(driveAmount);
        }

        // Apply amplitude envelope
        float env = ampEnv_.process();
        float out = osc * env;

        // Kick is mono, output to both channels
        left = right = out;
    }

    bool isActive() const { return ampEnv_.isActive(); }

private:
    void updateParams() {
        // Base frequency: 30-80 Hz based on pitch
        float baseFreq = 30.0f + basePitch_ * 50.0f;

        // Pitch envelope: tone controls sweep amount
        float startFreq = baseFreq + tone_ * 150.0f;  // Start up to 150Hz higher
        float sweepTime = 0.02f + tone_ * 0.08f;       // 20-100ms sweep
        pitchEnv_.setParams(startFreq, baseFreq, sweepTime);

        // Amplitude decay: 100ms to 2s
        float decayTime = 0.1f + decay_ * 1.9f;
        ampEnv_.setDecay(decayTime);
    }

    float sampleRate_ = 44100.0f;
    float basePitch_ = 0.3f;
    float decay_ = 0.5f;
    float tone_ = 0.5f;
    float drive_ = 0.2f;
    float velocity_ = 1.0f;
    float phase_ = 0.0f;

    DrumEnvelope ampEnv_;
    PitchEnvelope pitchEnv_;
};

// ============================================================================
// Analog Snare Drum
// ============================================================================

/**
 * TR-808 style analog snare drum
 *
 * Synthesis: Two tuned sine oscillators + bandpass filtered noise
 * Controls: Pitch, Decay, Snappy (noise amount), Tone (noise brightness)
 */
class AnalogSnare {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        ampEnv_.setSampleRate(sampleRate);
        noiseEnv_.setSampleRate(sampleRate);
        pitchEnv_.setSampleRate(sampleRate);
        updateParams();
        updateNoiseFilter();  // Initialize noise filter coefficient
    }

    void setPitch(float normalized) {
        pitch_ = normalized;
        updateParams();
    }

    void setDecay(float normalized) {
        decay_ = normalized;
        updateParams();
    }

    void setSnappy(float normalized) {
        snappy_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void setTone(float normalized) {
        // Controls highpass cutoff for noise (brighter = higher)
        noiseTone_ = normalized;
        updateNoiseFilter();
    }

    void trigger(float velocity) {
        velocity_ = velocity;
        phase1_ = 0.0f;
        phase2_ = 0.0f;
        ampEnv_.trigger(velocity);
        noiseEnv_.trigger(velocity);
        pitchEnv_.trigger();
        rngState_ = 0x12345678 ^ static_cast<uint32_t>(velocity * 12345.0f);
    }

    void choke() {
        ampEnv_.choke();
        noiseEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!ampEnv_.isActive() && !noiseEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        // === Tone component (two sines) ===
        float freq = pitchEnv_.getFrequency();
        float freq1 = freq;
        float freq2 = freq * 1.5f;  // Perfect fifth up

        phase1_ += freq1 / sampleRate_;
        phase2_ += freq2 / sampleRate_;
        if (phase1_ >= 1.0f) phase1_ -= 1.0f;
        if (phase2_ >= 1.0f) phase2_ -= 1.0f;

        float tone = std::sin(phase1_ * 2.0f * M_PI) * 0.7f +
                     std::sin(phase2_ * 2.0f * M_PI) * 0.3f;
        float toneEnv = ampEnv_.process();

        // === Noise component ===
        float noise = generateNoise();
        noise = processNoiseFilter(noise);
        float noiseEnvVal = noiseEnv_.process();

        // Mix tone and noise
        float toneOut = tone * toneEnv * (1.0f - snappy_ * 0.5f);
        float noiseOut = noise * noiseEnvVal * snappy_;
        float out = toneOut + noiseOut;

        left = right = out;
    }

    bool isActive() const { return ampEnv_.isActive() || noiseEnv_.isActive(); }

private:
    void updateParams() {
        // Base frequency: 100-250 Hz
        float baseFreq = 100.0f + pitch_ * 150.0f;
        float startFreq = baseFreq + 80.0f;  // Start 80Hz higher
        pitchEnv_.setParams(startFreq, baseFreq, 0.03f);

        // Tone decay: 50-300ms
        float toneDecay = 0.05f + decay_ * 0.25f;
        ampEnv_.setDecay(toneDecay);

        // Noise decay: slightly longer than tone
        float noiseDecay = 0.08f + decay_ * 0.4f;
        noiseEnv_.setDecay(noiseDecay);
    }

    void updateNoiseFilter() {
        // Simple one-pole highpass coefficient
        float cutoff = 2000.0f + noiseTone_ * 6000.0f;  // 2-8 kHz
        float rc = 1.0f / (2.0f * M_PI * cutoff);
        float dt = 1.0f / sampleRate_;
        hpCoeff_ = rc / (rc + dt);
    }

    float generateNoise() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float>(static_cast<int32_t>(rngState_)) * (1.0f / 2147483648.0f);
    }

    float processNoiseFilter(float input) {
        // Simple highpass filter
        float output = hpCoeff_ * (hpPrev_ + input - hpInput_);
        hpInput_ = input;
        hpPrev_ = output;
        return output;
    }

    float sampleRate_ = 44100.0f;
    float pitch_ = 0.4f;
    float decay_ = 0.5f;
    float snappy_ = 0.5f;
    float noiseTone_ = 0.5f;
    float velocity_ = 1.0f;

    float phase1_ = 0.0f;
    float phase2_ = 0.0f;

    DrumEnvelope ampEnv_;
    DrumEnvelope noiseEnv_;
    PitchEnvelope pitchEnv_;

    // Noise generation
    uint32_t rngState_ = 0x12345678;

    // Highpass filter state
    float hpCoeff_ = 0.9f;
    float hpInput_ = 0.0f;
    float hpPrev_ = 0.0f;
};

// ============================================================================
// Analog Hi-Hat
// ============================================================================

/**
 * TR-808 style analog hi-hat
 *
 * Synthesis: 6 square waves at metallic ratios + bandpass filter
 * Two modes: Closed (short decay) and Open (long decay)
 *
 * The key to hi-hat sound is the inharmonic frequency ratios creating
 * a noisy, metallic texture, then bandpass filtered.
 */
class AnalogHiHat {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        ampEnv_.setSampleRate(sampleRate);
        updateParams();
        updateOscFrequencies();
        updateFilter();
    }

    void setPitch(float normalized) {
        pitch_ = normalized;
        updateOscFrequencies();
    }

    void setDecay(float normalized) {
        decay_ = normalized;
        updateParams();
    }

    void setTone(float normalized) {
        tone_ = normalized;
        updateFilter();
    }

    void setOpen(bool open) {
        isOpen_ = open;
        updateParams();
    }

    void trigger(float velocity) {
        velocity_ = velocity;
        ampEnv_.trigger(velocity);
        // Reset phases with slight randomization for natural sound
        rngState_ = 0x12345678 ^ static_cast<uint32_t>(velocity * 54321.0f);
        for (int i = 0; i < 6; ++i) {
            phases_[i] = generateRandom() * 0.5f;
        }
    }

    void choke() {
        ampEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!ampEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        // Sum 6 square waves at metallic frequency ratios
        float sum = 0.0f;
        for (int i = 0; i < 6; ++i) {
            phases_[i] += phaseIncs_[i];
            if (phases_[i] >= 1.0f) phases_[i] -= 1.0f;

            float sq = (phases_[i] < 0.5f) ? 1.0f : -1.0f;
            sum += sq;
        }
        sum *= (1.0f / 6.0f);

        // Add some noise for extra sizzle
        float noise = generateNoise() * 0.3f;
        sum += noise;

        // Bandpass filter: highpass then lowpass
        float hp = hpCoeff_ * (hpPrev_ + sum - hpInput_);
        hpInput_ = sum;
        hpPrev_ = hp;

        lpState_ += lpCoeff_ * (hp - lpState_);
        float filtered = lpState_;

        // Apply envelope
        float env = ampEnv_.process();
        float out = filtered * env * 0.8f;

        left = right = out;
    }

    bool isActive() const { return ampEnv_.isActive(); }

private:
    void updateParams() {
        // Closed: 30-150ms, Open: 200-800ms
        float decayTime;
        if (isOpen_) {
            decayTime = 0.2f + decay_ * 0.6f;
        } else {
            decayTime = 0.03f + decay_ * 0.12f;
        }
        ampEnv_.setDecay(decayTime);
    }

    void updateOscFrequencies() {
        // Higher base frequencies for metallic hi-hat sound
        // TR-808 hi-hat oscillators are around 200-800 Hz fundamentals
        // but the metallic character comes from the mix of inharmonic ratios
        float base = 300.0f + pitch_ * 500.0f;  // 300-800 Hz

        // Non-harmonic ratios create metallic timbre
        const float ratios[6] = {1.0f, 1.34f, 1.47f, 1.66f, 1.81f, 2.0f};

        for (int i = 0; i < 6; ++i) {
            float freq = base * ratios[i];
            phaseIncs_[i] = freq / sampleRate_;
        }
    }

    void updateFilter() {
        // Highpass: 500-2000 Hz (lets the metallic harmonics through)
        float hpCutoff = 500.0f + tone_ * 1500.0f;
        float rc = 1.0f / (2.0f * M_PI * hpCutoff);
        float dt = 1.0f / sampleRate_;
        hpCoeff_ = rc / (rc + dt);

        // Lowpass: 8-16 kHz (removes harsh ultra-highs)
        float lpCutoff = 8000.0f + tone_ * 8000.0f;
        float lpRc = 1.0f / (2.0f * M_PI * lpCutoff);
        lpCoeff_ = dt / (lpRc + dt);
    }

    float generateRandom() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float>(rngState_) * (1.0f / 4294967296.0f);
    }

    float generateNoise() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float>(static_cast<int32_t>(rngState_)) * (1.0f / 2147483648.0f);
    }

    float sampleRate_ = 44100.0f;
    float pitch_ = 0.5f;
    float decay_ = 0.5f;
    float tone_ = 0.5f;
    float velocity_ = 1.0f;
    bool isOpen_ = false;

    float phases_[6] = {0};
    float phaseIncs_[6] = {0};

    DrumEnvelope ampEnv_;

    // Filter state
    float hpCoeff_ = 0.9f;
    float hpInput_ = 0.0f;
    float hpPrev_ = 0.0f;
    float lpCoeff_ = 0.3f;
    float lpState_ = 0.0f;

    uint32_t rngState_ = 0x12345678;
};

// ============================================================================
// Analog Clap
// ============================================================================

/**
 * TR-808 style hand clap
 *
 * Synthesis: Multiple filtered noise bursts with staggered timing
 */
class AnalogClap {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        ampEnv_.setSampleRate(sampleRate);
        updateParams();
        updateFilter();  // Initialize filter coefficients
    }

    void setDecay(float normalized) {
        decay_ = normalized;
        updateParams();
    }

    void setTone(float normalized) {
        tone_ = normalized;
        updateFilter();
    }

    void setSpread(float normalized) {
        // Controls timing spread of multiple hits
        spread_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void trigger(float velocity) {
        velocity_ = velocity;
        sampleCount_ = 0;
        burstIndex_ = 0;
        ampEnv_.trigger(velocity);
        rngState_ = 0x12345678 ^ static_cast<uint32_t>(velocity * 98765.0f);
    }

    void choke() {
        ampEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!ampEnv_.isActive() && burstIndex_ >= kNumBursts) {
            left = right = 0.0f;
            return;
        }

        // Multiple short bursts (4 bursts over ~30ms)
        float burstEnv = 0.0f;
        int samplesPerBurst = static_cast<int>(sampleRate_ * 0.007f * (1.0f + spread_));

        for (int b = 0; b < kNumBursts; ++b) {
            int burstStart = b * samplesPerBurst;
            int burstEnd = burstStart + static_cast<int>(sampleRate_ * 0.003f);

            if (sampleCount_ >= burstStart && sampleCount_ < burstEnd) {
                // Burst is active
                float t = static_cast<float>(sampleCount_ - burstStart) /
                         static_cast<float>(burstEnd - burstStart);
                float env = 1.0f - t;  // Simple decay per burst
                burstEnv = std::max(burstEnv, env);
            }
        }

        // After all bursts, use main envelope for tail
        if (sampleCount_ > kNumBursts * samplesPerBurst) {
            burstEnv = ampEnv_.process();
        }

        // Generate and filter noise
        float noise = generateNoise();
        float filtered = processFilter(noise);

        float out = filtered * burstEnv * velocity_;

        left = right = out;
        sampleCount_++;
    }

    bool isActive() const {
        return ampEnv_.isActive() || sampleCount_ < static_cast<int>(sampleRate_ * 0.1f);
    }

private:
    static constexpr int kNumBursts = 4;

    void updateParams() {
        // Tail decay: 100-400ms
        float decayTime = 0.1f + decay_ * 0.3f;
        ampEnv_.setDecay(decayTime);
    }

    void updateFilter() {
        // Bandpass around 1-2 kHz
        float cutoff = 800.0f + tone_ * 1500.0f;
        float rc = 1.0f / (2.0f * M_PI * cutoff);
        float dt = 1.0f / sampleRate_;
        hpCoeff_ = rc / (rc + dt);

        float lpCutoff = 2000.0f + tone_ * 2000.0f;
        float lpRc = 1.0f / (2.0f * M_PI * lpCutoff);
        lpCoeff_ = dt / (lpRc + dt);
    }

    float generateNoise() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float>(static_cast<int32_t>(rngState_)) * (1.0f / 2147483648.0f);
    }

    float processFilter(float input) {
        // Highpass
        float hp = hpCoeff_ * (hpPrev_ + input - hpInput_);
        hpInput_ = input;
        hpPrev_ = hp;

        // Lowpass
        lpState_ += lpCoeff_ * (hp - lpState_);

        return lpState_;
    }

    float sampleRate_ = 44100.0f;
    float decay_ = 0.5f;
    float tone_ = 0.5f;
    float spread_ = 0.5f;
    float velocity_ = 1.0f;

    int sampleCount_ = 0;
    int burstIndex_ = 0;

    DrumEnvelope ampEnv_;

    // Filter state
    float hpCoeff_ = 0.9f;
    float hpInput_ = 0.0f;
    float hpPrev_ = 0.0f;
    float lpCoeff_ = 0.3f;
    float lpState_ = 0.0f;

    uint32_t rngState_ = 0x12345678;
};

// ============================================================================
// Analog Tom
// ============================================================================

/**
 * TR-808 style tom drum
 *
 * Synthesis: Sine oscillator with pitch envelope (similar to kick but higher)
 */
class AnalogTom {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        ampEnv_.setSampleRate(sampleRate);
        pitchEnv_.setSampleRate(sampleRate);
        updateParams();
    }

    void setPitch(float normalized) {
        pitch_ = normalized;
        updateParams();
    }

    void setDecay(float normalized) {
        decay_ = normalized;
        updateParams();
    }

    void setTone(float normalized) {
        tone_ = normalized;
        updateParams();
    }

    void trigger(float velocity) {
        velocity_ = velocity;
        phase_ = 0.0f;
        ampEnv_.trigger(velocity);
        pitchEnv_.trigger();
    }

    void choke() {
        ampEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!ampEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        float freq = pitchEnv_.getFrequency();
        float phaseInc = freq / sampleRate_;
        phase_ += phaseInc;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        float osc = std::sin(phase_ * 2.0f * M_PI);
        float env = ampEnv_.process();
        float out = osc * env;

        left = right = out;
    }

    bool isActive() const { return ampEnv_.isActive(); }

private:
    void updateParams() {
        // Base frequency: 80-300 Hz (low to high tom)
        float baseFreq = 80.0f + pitch_ * 220.0f;
        float startFreq = baseFreq + tone_ * 100.0f;
        pitchEnv_.setParams(startFreq, baseFreq, 0.04f);

        // Decay: 100-600ms
        float decayTime = 0.1f + decay_ * 0.5f;
        ampEnv_.setDecay(decayTime);
    }

    float sampleRate_ = 44100.0f;
    float pitch_ = 0.5f;
    float decay_ = 0.5f;
    float tone_ = 0.5f;
    float velocity_ = 1.0f;
    float phase_ = 0.0f;

    DrumEnvelope ampEnv_;
    PitchEnvelope pitchEnv_;
};

// ============================================================================
// Analog Cymbal (Crash/Ride)
// ============================================================================

/**
 * TR-808/909 style cymbal
 *
 * Synthesis: Multiple metallic oscillators + noise with long decay
 */
class AnalogCymbal {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        ampEnv_.setSampleRate(sampleRate);
        updateParams();
        updateOscFrequencies();
        updateFilter();
    }

    void setPitch(float normalized) {
        pitch_ = normalized;
        updateOscFrequencies();
    }

    void setDecay(float normalized) {
        decay_ = normalized;
        updateParams();
    }

    void setTone(float normalized) {
        tone_ = normalized;
        updateFilter();
    }

    /**
     * Mix between metallic oscillators and noise shimmer
     * 0 = mostly metallic, 1 = mostly noise
     */
    void setMix(float normalized) {
        mix_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void trigger(float velocity) {
        velocity_ = velocity;
        ampEnv_.trigger(velocity);
        rngState_ = 0x12345678 ^ static_cast<uint32_t>(velocity * 11111.0f);
        for (int i = 0; i < 6; ++i) {
            phases_[i] = generateRandom();
        }
    }

    void choke() {
        ampEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!ampEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        // Metallic oscillators - higher frequencies than hi-hat
        float metal = 0.0f;
        for (int i = 0; i < 6; ++i) {
            phases_[i] += phaseIncs_[i];
            if (phases_[i] >= 1.0f) phases_[i] -= 1.0f;
            float sq = (phases_[i] < 0.5f) ? 1.0f : -1.0f;
            metal += sq;
        }
        metal *= (1.0f / 6.0f);

        // Noise for shimmer
        float noise = generateNoise();
        float noiseAmount = 0.2f + mix_ * 0.5f;
        float sum = metal * (1.0f - noiseAmount * 0.3f) + noise * noiseAmount;

        // Bandpass filter for cymbal character
        float hp = hpCoeff_ * (hpPrev_ + sum - hpInput_);
        hpInput_ = sum;
        hpPrev_ = hp;

        lpState_ += lpCoeff_ * (hp - lpState_);
        float filtered = lpState_;

        float env = ampEnv_.process();
        float out = filtered * env * 0.6f;

        left = right = out;
    }

    bool isActive() const { return ampEnv_.isActive(); }

private:
    void updateParams() {
        // Long decay: 500ms - 4s
        float decayTime = 0.5f + decay_ * 3.5f;
        ampEnv_.setDecay(decayTime);
    }

    void updateOscFrequencies() {
        // Cymbal uses higher frequencies than hi-hat
        float base = 400.0f + pitch_ * 600.0f;  // 400-1000 Hz
        const float ratios[6] = {1.0f, 1.19f, 1.41f, 1.56f, 1.73f, 2.0f};

        for (int i = 0; i < 6; ++i) {
            phaseIncs_[i] = (base * ratios[i]) / sampleRate_;
        }
    }

    void updateFilter() {
        // Highpass: 300-1500 Hz (allow fundamentals through)
        float hpCutoff = 300.0f + tone_ * 1200.0f;
        float rc = 1.0f / (2.0f * M_PI * hpCutoff);
        float dt = 1.0f / sampleRate_;
        hpCoeff_ = rc / (rc + dt);

        // Lowpass: 6-14 kHz
        float lpCutoff = 6000.0f + tone_ * 8000.0f;
        float lpRc = 1.0f / (2.0f * M_PI * lpCutoff);
        lpCoeff_ = dt / (lpRc + dt);
    }

    float generateNoise() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float>(static_cast<int32_t>(rngState_)) * (1.0f / 2147483648.0f);
    }

    float generateRandom() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float>(rngState_) * (1.0f / 4294967296.0f);
    }

    float sampleRate_ = 44100.0f;
    float pitch_ = 0.5f;
    float decay_ = 0.5f;
    float tone_ = 0.5f;
    float mix_ = 0.5f;
    float velocity_ = 1.0f;

    float phases_[6] = {0};
    float phaseIncs_[6] = {0};

    DrumEnvelope ampEnv_;

    // Bandpass filter state
    float hpCoeff_ = 0.9f;
    float hpInput_ = 0.0f;
    float hpPrev_ = 0.0f;
    float lpCoeff_ = 0.3f;
    float lpState_ = 0.0f;

    uint32_t rngState_ = 0x12345678;
};

// ============================================================================
// Cowbell
// ============================================================================

/**
 * TR-808 cowbell
 *
 * Two square waves at ~540Hz and ~800Hz with metallic timbre
 */
class AnalogCowbell {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        ampEnv_.setSampleRate(sampleRate);
        updateOscFrequencies();
        ampEnv_.setDecay(0.3f);  // 300ms default decay
    }

    void setPitch(float normalized) {
        pitch_ = normalized;
        updateOscFrequencies();
    }

    void setDecay(float normalized) {
        // 100ms to 800ms
        float decayTime = 0.1f + normalized * 0.7f;
        ampEnv_.setDecay(decayTime);
    }

    void trigger(float velocity) {
        phase1_ = 0.0f;
        phase2_ = 0.0f;
        ampEnv_.trigger(velocity);
    }

    void choke() {
        ampEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!ampEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        // Two square waves - the core cowbell sound
        phase1_ += phaseInc1_;
        phase2_ += phaseInc2_;
        if (phase1_ >= 1.0f) phase1_ -= 1.0f;
        if (phase2_ >= 1.0f) phase2_ -= 1.0f;

        // Square waves
        float sq1 = (phase1_ < 0.5f) ? 1.0f : -1.0f;
        float sq2 = (phase2_ < 0.5f) ? 1.0f : -1.0f;

        // Mix and apply envelope directly - no filtering
        float env = ampEnv_.process();
        float out = (sq1 + sq2) * 0.25f * env;

        left = right = out;
    }

    bool isActive() const { return ampEnv_.isActive(); }

private:
    void updateOscFrequencies() {
        // Cowbell frequencies: ~540 Hz and ~800 Hz
        float base = 500.0f + pitch_ * 100.0f;
        phaseInc1_ = base / sampleRate_;
        phaseInc2_ = (base * 1.5f) / sampleRate_;
    }

    float sampleRate_ = 44100.0f;
    float pitch_ = 0.5f;
    float phase1_ = 0.0f;
    float phase2_ = 0.0f;
    float phaseInc1_ = 0.012f;  // ~540Hz at 44100
    float phaseInc2_ = 0.018f;  // ~800Hz at 44100
    DrumEnvelope ampEnv_;
};

// ============================================================================
// Rimshot
// ============================================================================

/**
 * TR-808 rimshot
 *
 * Synthesis: Short pitched click + filtered noise burst
 */
class AnalogRimshot {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        clickEnv_.setSampleRate(sampleRate);
        noiseEnv_.setSampleRate(sampleRate);
        updateParams();
    }

    void setPitch(float normalized) {
        pitch_ = normalized;
        updateParams();
    }

    void setDecay(float normalized) {
        decay_ = normalized;
        updateParams();
    }

    void trigger(float velocity) {
        velocity_ = velocity;
        phase_ = 0.0f;
        clickEnv_.trigger(velocity);
        noiseEnv_.trigger(velocity * 0.5f);
        rngState_ = 0x12345678;
    }

    void choke() {
        clickEnv_.choke();
        noiseEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!clickEnv_.isActive() && !noiseEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        // Click - short high-pitched tone
        phase_ += phaseInc_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
        float click = std::sin(phase_ * 2.0f * M_PI);
        float clickEnvVal = clickEnv_.process();

        // Noise burst
        float noise = generateNoise();
        noise = processFilter(noise);
        float noiseEnvVal = noiseEnv_.process();

        float out = click * clickEnvVal * 0.7f + noise * noiseEnvVal * 0.3f;

        left = right = out;
    }

    bool isActive() const { return clickEnv_.isActive() || noiseEnv_.isActive(); }

private:
    void updateParams() {
        // High click frequency: 800-1500 Hz
        float freq = 800.0f + pitch_ * 700.0f;
        phaseInc_ = freq / sampleRate_;

        // Very short decays
        clickEnv_.setDecay(0.005f + decay_ * 0.02f);
        noiseEnv_.setDecay(0.01f + decay_ * 0.03f);

        // Update filter
        float cutoff = freq * 2.0f;
        float rc = 1.0f / (2.0f * M_PI * cutoff);
        float dt = 1.0f / sampleRate_;
        hpCoeff_ = rc / (rc + dt);
    }

    float generateNoise() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float>(static_cast<int32_t>(rngState_)) * (1.0f / 2147483648.0f);
    }

    float processFilter(float input) {
        float output = hpCoeff_ * (hpPrev_ + input - hpInput_);
        hpInput_ = input;
        hpPrev_ = output;
        return output;
    }

    float sampleRate_ = 44100.0f;
    float pitch_ = 0.5f;
    float decay_ = 0.5f;
    float velocity_ = 1.0f;
    float phase_ = 0.0f;
    float phaseInc_ = 0.0f;

    DrumEnvelope clickEnv_;
    DrumEnvelope noiseEnv_;

    float hpCoeff_ = 0.9f;
    float hpInput_ = 0.0f;
    float hpPrev_ = 0.0f;

    uint32_t rngState_ = 0x12345678;
};

// ============================================================================
// Claves
// ============================================================================

/**
 * TR-808 claves
 *
 * Synthesis: Short high-pitched resonant sine wave (~1667 Hz per spec)
 * Very short decay, minimal noise - pure "wood block" click
 */
class AnalogClaves {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        ampEnv_.setSampleRate(sampleRate);
        updateParams();
    }

    void setPitch(float normalized) {
        pitch_ = normalized;
        updateParams();
    }

    void setDecay(float normalized) {
        decay_ = normalized;
        updateParams();
    }

    void trigger(float velocity) {
        velocity_ = velocity;
        phase_ = 0.0f;
        ampEnv_.trigger(velocity);
    }

    void choke() {
        ampEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!ampEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        // Pure resonant sine for wooden click
        phase_ += phaseInc_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        // Sine with slight harmonic content via soft clipping
        float osc = std::sin(phase_ * 2.0f * M_PI);
        osc = osc + 0.1f * std::sin(phase_ * 4.0f * M_PI);  // Add 2nd harmonic

        float env = ampEnv_.process();
        float out = osc * env * 0.8f;

        left = right = out;
    }

    bool isActive() const { return ampEnv_.isActive(); }

private:
    void updateParams() {
        // Clave frequency: ~500-2000 Hz (spec says ~1667 Hz for RS/CL circuit)
        float freq = 400.0f + pitch_ * 1600.0f;
        phaseInc_ = freq / sampleRate_;

        // Very short decay: 10-50ms
        float decayTime = 0.01f + decay_ * 0.04f;
        ampEnv_.setDecay(decayTime);
    }

    float sampleRate_ = 44100.0f;
    float pitch_ = 0.7f;  // Default ~1500 Hz
    float decay_ = 0.5f;
    float velocity_ = 1.0f;
    float phase_ = 0.0f;
    float phaseInc_ = 0.0f;

    DrumEnvelope ampEnv_;
};

// ============================================================================
// Maracas
// ============================================================================

/**
 * TR-808 maracas
 *
 * Synthesis: High-pass filtered white noise burst
 * Per spec: HPF ~5kHz, 25-35ms decay
 */
class AnalogMaracas {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        ampEnv_.setSampleRate(sampleRate);
        updateFilter();
        updateParams();
    }

    void setTone(float normalized) {
        tone_ = normalized;
        updateFilter();
    }

    void setDecay(float normalized) {
        decay_ = normalized;
        updateParams();
    }

    void trigger(float velocity) {
        velocity_ = velocity;
        ampEnv_.trigger(velocity);
        rngState_ = 0x12345678 ^ static_cast<uint32_t>(velocity * 77777.0f);
    }

    void choke() {
        ampEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!ampEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        // White noise
        float noise = generateNoise();

        // High-pass filter for bright shaker sound
        float filtered = processFilter(noise);

        float env = ampEnv_.process();
        float out = filtered * env;

        left = right = out;
    }

    bool isActive() const { return ampEnv_.isActive(); }

private:
    void updateParams() {
        // Short decay: 15-50ms (spec says 25-35ms)
        float decayTime = 0.015f + decay_ * 0.035f;
        ampEnv_.setDecay(decayTime);
    }

    void updateFilter() {
        // High-pass cutoff: 3-8 kHz (spec says ~5kHz)
        float cutoff = 3000.0f + tone_ * 5000.0f;
        float rc = 1.0f / (2.0f * M_PI * cutoff);
        float dt = 1.0f / sampleRate_;
        hpCoeff_ = rc / (rc + dt);
    }

    float generateNoise() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float>(static_cast<int32_t>(rngState_)) * (1.0f / 2147483648.0f);
    }

    float processFilter(float input) {
        float output = hpCoeff_ * (hpPrev_ + input - hpInput_);
        hpInput_ = input;
        hpPrev_ = output;
        return output;
    }

    float sampleRate_ = 44100.0f;
    float tone_ = 0.5f;
    float decay_ = 0.5f;
    float velocity_ = 1.0f;

    DrumEnvelope ampEnv_;

    // Highpass filter state
    float hpCoeff_ = 0.95f;
    float hpInput_ = 0.0f;
    float hpPrev_ = 0.0f;

    uint32_t rngState_ = 0x12345678;
};

// ============================================================================
// DFAM-Style Drum Voice
// ============================================================================

/**
 * Moog DFAM-inspired analog drum voice
 *
 * Full semi-modular percussion synthesizer architecture:
 * - 2 VCOs (triangle/square) with FM and hard sync
 * - White noise source
 * - 3-channel mixer
 * - Moog-style ladder filter (LP/HP)
 * - 3 independent decay envelopes (pitch, filter, VCA)
 * - Bipolar envelope modulation
 *
 * This is the most complex drum voice, offering deep sound design capabilities.
 * Each of the 4 drum voices (BD, SD, HH, RS) will be pre-configured versions
 * of this engine.
 */
class DFAMVoice {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        vcoEnv_.setSampleRate(sampleRate);
        vcfEnv_.setSampleRate(sampleRate);
        vcaEnv_.setSampleRate(sampleRate);
        updateAllParams();
    }

    // ===== VCO 1 Parameters =====
    void setVCO1Freq(float normalized) {
        vco1Freq_ = normalized;
        updateVCO1Frequency();
    }

    void setVCO1Wave(float normalized) {
        vco1Wave_ = std::clamp(normalized, 0.0f, 1.0f);  // 0=tri, 1=square
    }

    void setVCO1Level(float normalized) {
        vco1Level_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void setVCO1EGAmount(float normalized) {
        // Bipolar: -1 to +1 (center = 0)
        vco1EGAmount_ = (normalized - 0.5f) * 2.0f;
    }

    // ===== VCO 2 Parameters =====
    void setVCO2Freq(float normalized) {
        vco2Freq_ = normalized;
        updateVCO2Frequency();
    }

    void setVCO2Wave(float normalized) {
        vco2Wave_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void setVCO2Level(float normalized) {
        vco2Level_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void setVCO2EGAmount(float normalized) {
        vco2EGAmount_ = (normalized - 0.5f) * 2.0f;
    }

    void setFMAmount(float normalized) {
        fmAmount_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void setHardSync(float normalized) {
        hardSync_ = normalized > 0.5f;
    }

    // ===== Noise/Mixer Parameters =====
    void setNoiseLevel(float normalized) {
        noiseLevel_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    // ===== Filter Parameters =====
    void setCutoff(float normalized) {
        cutoff_ = normalized;
        updateFilter();
    }

    void setResonance(float normalized) {
        resonance_ = std::clamp(normalized, 0.0f, 1.0f);
    }

    void setVCFEGAmount(float normalized) {
        vcfEGAmount_ = (normalized - 0.5f) * 2.0f;  // Bipolar
    }

    void setFilterMode(float normalized) {
        filterMode_ = normalized > 0.5f;  // false=LP, true=HP
    }

    // ===== Envelope Parameters =====
    void setVCODecay(float normalized) {
        vcoDecay_ = normalized;
        updateVCOEnvelope();
    }

    void setVCFDecay(float normalized) {
        vcfDecay_ = normalized;
        updateVCFEnvelope();
    }

    void setVCADecay(float normalized) {
        vcaDecay_ = normalized;
        updateVCAEnvelope();
    }

    void setVCAAttack(float normalized) {
        // Fast (1ms) or Slow (100ms)
        vcaAttackSlow_ = normalized > 0.5f;
        updateVCAEnvelope();
    }

    // ===== Trigger =====
    void trigger(float velocity) {
        velocity_ = velocity;

        // Trigger all three envelopes with velocity scaling
        vcoEnv_.trigger(velocity);
        vcfEnv_.trigger(velocity);
        vcaEnv_.trigger(velocity);

        // Reset oscillator phases
        vco1Phase_ = 0.0f;
        vco2Phase_ = 0.0f;

        // Seed noise generator
        rngState_ = 0x12345678 ^ static_cast<uint32_t>(velocity * 54321.0f);
    }

    void choke() {
        vcoEnv_.choke();
        vcfEnv_.choke();
        vcaEnv_.choke();
    }

    void process(float& left, float& right) {
        if (!vcaEnv_.isActive()) {
            left = right = 0.0f;
            return;
        }

        // ===== Process Envelopes =====
        float vcoEnvValue = vcoEnv_.process();
        float vcfEnvValue = vcfEnv_.process();
        float vcaEnvValue = vcaEnv_.process();

        // ===== VCO 1 with Pitch Envelope =====
        float vco1FreqMod = vco1BaseFreq_;
        if (vco1EGAmount_ != 0.0f) {
            // Bipolar pitch modulation (up to ±5 octaves)
            float pitchMod = vco1EGAmount_ * vcoEnvValue * 5.0f;  // octaves
            vco1FreqMod = vco1BaseFreq_ * std::pow(2.0f, pitchMod);
        }

        float vco1Inc = vco1FreqMod / sampleRate_;
        vco1Phase_ += vco1Inc;
        if (vco1Phase_ >= 1.0f) {
            vco1Phase_ -= 1.0f;
            // Hard sync: reset VCO 2 phase when VCO 1 completes cycle
            if (hardSync_) {
                vco2Phase_ = 0.0f;
            }
        }

        // VCO 1 waveform (triangle/square blend)
        float vco1Out;
        float tri1 = 4.0f * std::abs(vco1Phase_ - 0.5f) - 1.0f;
        float sq1 = (vco1Phase_ < 0.5f) ? 1.0f : -1.0f;
        vco1Out = tri1 * (1.0f - vco1Wave_) + sq1 * vco1Wave_;

        // ===== VCO 2 with Pitch Envelope + FM =====
        float vco2FreqMod = vco2BaseFreq_;
        if (vco2EGAmount_ != 0.0f) {
            float pitchMod = vco2EGAmount_ * vcoEnvValue * 5.0f;
            vco2FreqMod = vco2BaseFreq_ * std::pow(2.0f, pitchMod);
        }

        // Linear FM from VCO 1
        float fmMod = vco1Out * fmAmount_ * vco2FreqMod * 0.5f;
        vco2FreqMod += fmMod;

        float vco2Inc = vco2FreqMod / sampleRate_;
        vco2Phase_ += vco2Inc;
        if (vco2Phase_ >= 1.0f) vco2Phase_ -= 1.0f;

        // VCO 2 waveform
        float vco2Out;
        float tri2 = 4.0f * std::abs(vco2Phase_ - 0.5f) - 1.0f;
        float sq2 = (vco2Phase_ < 0.5f) ? 1.0f : -1.0f;
        vco2Out = tri2 * (1.0f - vco2Wave_) + sq2 * vco2Wave_;

        // ===== White Noise =====
        float noise = generateNoise();

        // ===== 3-Channel Mixer =====
        float mixed = vco1Out * vco1Level_ +
                      vco2Out * vco2Level_ +
                      noise * noiseLevel_;

        // ===== VCF with Envelope Modulation =====
        float cutoffMod = baseCutoffHz_;
        if (vcfEGAmount_ != 0.0f) {
            // Bipolar cutoff modulation (±4 octaves)
            float cutoffModOct = vcfEGAmount_ * vcfEnvValue * 4.0f;
            cutoffMod = baseCutoffHz_ * std::pow(2.0f, cutoffModOct);
            cutoffMod = std::clamp(cutoffMod, 20.0f, 20000.0f);
        }

        // Simple ladder filter approximation
        float filtered = processLadderFilter(mixed, cutoffMod);

        // ===== VCA with Envelope =====
        float out = filtered * vcaEnvValue;

        left = right = out * 0.7f;
    }

    bool isActive() const {
        return vcaEnv_.isActive();
    }

private:
    void updateAllParams() {
        updateVCO1Frequency();
        updateVCO2Frequency();
        updateFilter();
        updateVCOEnvelope();
        updateVCFEnvelope();
        updateVCAEnvelope();
    }

    void updateVCO1Frequency() {
        // 10 octaves: ~20Hz to ~20kHz (but typically used in bass range)
        vco1BaseFreq_ = 20.0f * std::pow(2.0f, vco1Freq_ * 10.0f);
    }

    void updateVCO2Frequency() {
        vco2BaseFreq_ = 20.0f * std::pow(2.0f, vco2Freq_ * 10.0f);
    }

    void updateFilter() {
        // Cutoff: 20Hz to 20kHz
        baseCutoffHz_ = 20.0f * std::pow(1000.0f, cutoff_);
    }

    void updateVCOEnvelope() {
        // VCO pitch envelope: 1ms attack, 1ms to several seconds decay
        float decay = 0.001f + vcoDecay_ * vcoDecay_ * vcoDecay_ * 5.0f;  // Cubic curve
        vcoEnv_.setDecay(decay);
    }

    void updateVCFEnvelope() {
        // VCF envelope: 1ms attack, 10ms to 10s decay
        float decay = 0.01f + vcfDecay_ * vcfDecay_ * vcfDecay_ * 10.0f;
        vcfEnv_.setDecay(decay);
    }

    void updateVCAEnvelope() {
        // VCA envelope: Fast (1ms) or Slow (100ms) attack
        // Decay: ~1ms to several seconds
        float decay = 0.001f + vcaDecay_ * vcaDecay_ * vcaDecay_ * 3.0f;
        vcaEnv_.setDecay(decay);
    }

    float generateNoise() {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float>(static_cast<int32_t>(rngState_)) * (1.0f / 2147483648.0f);
    }

    // Simplified 4-pole ladder filter (Moog-style)
    float processLadderFilter(float input, float cutoffHz) {
        // Update filter coefficients
        float wc = 2.0f * M_PI * cutoffHz / sampleRate_;
        wc = std::clamp(wc, 0.0f, static_cast<float>(M_PI * 0.45f));
        float g = std::tan(wc * 0.5f);
        float G = g / (1.0f + g);

        // Feedback with resonance
        float feedback = resonance_ * 4.0f;  // 0-4 range
        float S = G * G * G * G;
        float inputFB = input - feedback * ladderState_[3];

        // 4 cascaded one-pole lowpass stages
        for (int i = 0; i < 4; ++i) {
            float stageInput = (i == 0) ? inputFB : ladderState_[i - 1];
            ladderState_[i] = G * stageInput + (1.0f - G) * ladderState_[i];
        }

        // LP or HP output
        if (filterMode_) {
            // Highpass = input - lowpass
            return input - ladderState_[3];
        } else {
            // Lowpass
            return ladderState_[3];
        }
    }

    float sampleRate_ = 44100.0f;
    float velocity_ = 1.0f;

    // VCO 1
    float vco1Freq_ = 0.5f;           // Normalized frequency control
    float vco1BaseFreq_ = 100.0f;      // Hz
    float vco1Wave_ = 0.0f;            // 0=triangle, 1=square
    float vco1Level_ = 0.7f;
    float vco1EGAmount_ = 0.0f;        // Bipolar -1 to +1
    float vco1Phase_ = 0.0f;

    // VCO 2
    float vco2Freq_ = 0.5f;
    float vco2BaseFreq_ = 100.0f;
    float vco2Wave_ = 0.0f;
    float vco2Level_ = 0.0f;
    float vco2EGAmount_ = 0.0f;
    float vco2Phase_ = 0.0f;

    // FM & Sync
    float fmAmount_ = 0.0f;
    bool hardSync_ = false;

    // Noise
    float noiseLevel_ = 0.0f;
    uint32_t rngState_ = 0x12345678;

    // Filter
    float cutoff_ = 0.7f;
    float baseCutoffHz_ = 2000.0f;
    float resonance_ = 0.2f;
    float vcfEGAmount_ = 0.0f;         // Bipolar
    bool filterMode_ = false;          // false=LP, true=HP
    float ladderState_[4] = {0};       // 4-pole ladder stages

    // Envelopes
    float vcoDecay_ = 0.3f;
    float vcfDecay_ = 0.3f;
    float vcaDecay_ = 0.3f;
    bool vcaAttackSlow_ = false;

    DrumEnvelope vcoEnv_;
    DrumEnvelope vcfEnv_;
    DrumEnvelope vcaEnv_;
};

} // namespace anysynth
