#pragma once

#include <juce_events/juce_events.h>

#include "ModInfo.hpp"

namespace magda {

/**
 * @brief Engine for calculating LFO modulation values
 *
 * Singleton that runs at 60 FPS to update all LFO phase and output values.
 * Updates phase based on rate, then generates waveform output.
 */
class ModulatorEngine : public juce::Timer {
  public:
    static ModulatorEngine& getInstance() {
        static ModulatorEngine instance;
        return instance;
    }

    ~ModulatorEngine() override {
        stopTimer();
    }

    // Delete copy/move
    ModulatorEngine(const ModulatorEngine&) = delete;
    ModulatorEngine& operator=(const ModulatorEngine&) = delete;
    ModulatorEngine(ModulatorEngine&&) = delete;
    ModulatorEngine& operator=(ModulatorEngine&&) = delete;

    /**
     * @brief Generate waveform value for given phase
     * @param waveform The waveform type
     * @param phase Current phase (0.0 to 1.0)
     * @return Output value (0.0 to 1.0)
     */
    static float generateWaveform(LFOWaveform waveform, float phase) {
        constexpr float PI = 3.14159265359f;

        switch (waveform) {
            case LFOWaveform::Sine:
                return (std::sin(2.0f * PI * phase) + 1.0f) * 0.5f;

            case LFOWaveform::Triangle:
                return (phase < 0.5f) ? phase * 2.0f : 2.0f - phase * 2.0f;

            case LFOWaveform::Square:
                return phase < 0.5f ? 1.0f : 0.0f;

            case LFOWaveform::Saw:
                return phase;

            case LFOWaveform::ReverseSaw:
                return 1.0f - phase;

            default:
                return 0.5f;
        }
    }

  private:
    ModulatorEngine() = default;

    void timerCallback() override {
        // Calculate delta time (approximately 1/60 second at 60 FPS)
        double deltaTime = getTimerInterval() / 1000.0;

        // Update all mods through TrackManager
        updateAllMods(deltaTime);
    }

    void updateAllMods(double deltaTime);
};

}  // namespace magda
