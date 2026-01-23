#pragma once

#include <juce_events/juce_events.h>

#include "ModInfo.hpp"

namespace magda {

// Forward declarations
class TrackManager;
struct RackInfo;

/**
 * @brief Simple LFO generator for mock modulation
 *
 * Updates LFO phases and generates waveform output values.
 * Runs on a timer to simulate real-time modulation.
 */
class ModulatorEngine : public juce::Timer {
  public:
    static ModulatorEngine& getInstance() {
        static ModulatorEngine instance;
        return instance;
    }

    void start() {
        if (!isTimerRunning()) {
            startTimer(16);  // ~60 FPS
        }
    }

    void stop() {
        stopTimer();
    }

    /**
     * @brief Update a mod's phase and output value
     * @param mod The mod to update
     * @param deltaTime Time elapsed since last update (seconds)
     */
    static void updateMod(ModInfo& mod, double deltaTime) {
        if (mod.type != ModType::LFO) {
            return;  // Only handle LFOs for now
        }

        // Update phase
        mod.phase += static_cast<float>(mod.rate * deltaTime);
        while (mod.phase >= 1.0f) {
            mod.phase -= 1.0f;  // Wrap phase to [0, 1)
        }

        // Generate waveform output
        mod.value = generateWaveform(mod.waveform, mod.phase);
    }

    /**
     * @brief Generate waveform value for a given phase
     * @param waveform The waveform shape
     * @param phase Current phase (0.0 to 1.0)
     * @return Output value (0.0 to 1.0)
     */
    static float generateWaveform(LFOWaveform waveform, float phase) {
        constexpr float PI = 3.14159265359f;

        switch (waveform) {
            case LFOWaveform::Sine: {
                // sin(2π * phase) mapped to [0, 1]
                float sinValue = juce::MathConstants<float>::pi * 2.0f * phase;
                return (std::sin(sinValue) + 1.0f) * 0.5f;
            }

            case LFOWaveform::Triangle:
                // Triangle wave: ramps up 0→1 then down 1→0
                if (phase < 0.5f) {
                    return phase * 2.0f;  // 0 to 1
                } else {
                    return 2.0f - (phase * 2.0f);  // 1 to 0
                }

            case LFOWaveform::Square:
                // Square wave: 1.0 for first half, 0.0 for second half
                return phase < 0.5f ? 1.0f : 0.0f;

            case LFOWaveform::Saw:
                // Sawtooth: linear ramp from 0 to 1
                return phase;

            case LFOWaveform::ReverseSaw:
                // Reverse sawtooth: linear ramp from 1 to 0
                return 1.0f - phase;

            default:
                return 0.5f;
        }
    }

    // Update all mods via TrackManager
    void updateAllMods(double deltaTime);

  private:
    ModulatorEngine() = default;
    ~ModulatorEngine() = default;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorEngine)
};

}  // namespace magda
