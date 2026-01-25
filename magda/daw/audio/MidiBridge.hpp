#pragma once

#include <tracktion_engine/tracktion_engine.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include "../core/MidiTypes.hpp"
#include "../core/TypeIds.hpp"

namespace magda {

namespace te = tracktion;

// Forward declaration
class AudioBridge;

/**
 * @brief Bridges MAGDA's MIDI model to Tracktion Engine's MIDI system
 *
 * Responsibilities:
 * - Enumerate and manage MIDI input devices
 * - Route MIDI inputs to tracks
 * - Monitor MIDI activity for visualization
 * - Thread-safe communication between UI and audio threads
 *
 * Similar to AudioBridge, but for MIDI.
 */
class MidiBridge : public juce::MidiInputCallback {
  public:
    explicit MidiBridge(te::Engine& engine);
    ~MidiBridge() override;

    /**
     * @brief Set AudioBridge reference for triggering MIDI activity
     * Must be called after AudioBridge is created
     */
    void setAudioBridge(AudioBridge* audioBridge);

    // =========================================================================
    // MIDI Device Enumeration
    // =========================================================================

    /**
     * @brief Get all available MIDI input devices
     * @return Vector of device info (id, name, enabled status)
     */
    std::vector<MidiDeviceInfo> getAvailableMidiInputs() const;

    /**
     * @brief Get all available MIDI output devices
     * @return Vector of device info
     */
    std::vector<MidiDeviceInfo> getAvailableMidiOutputs() const;

    // =========================================================================
    // MIDI Device Enable/Disable
    // =========================================================================

    /**
     * @brief Enable a MIDI input device globally
     * @param deviceId Device identifier from MidiDeviceInfo
     */
    void enableMidiInput(const juce::String& deviceId);

    /**
     * @brief Disable a MIDI input device globally
     * @param deviceId Device identifier
     */
    void disableMidiInput(const juce::String& deviceId);

    /**
     * @brief Check if a MIDI input is enabled
     */
    bool isMidiInputEnabled(const juce::String& deviceId) const;

    // =========================================================================
    // Track MIDI Routing
    // =========================================================================

    /**
     * @brief Set MIDI input source for a track
     * @param trackId MAGDA track ID
     * @param midiDeviceId MIDI device ID (empty string = no input)
     */
    void setTrackMidiInput(TrackId trackId, const juce::String& midiDeviceId);

    /**
     * @brief Get current MIDI input source for a track
     * @return Device ID, or empty string if no input
     */
    juce::String getTrackMidiInput(TrackId trackId) const;

    /**
     * @brief Clear MIDI input routing for a track
     */
    void clearTrackMidiInput(TrackId trackId);

    // =========================================================================
    // MIDI Monitoring (for visualization)
    // =========================================================================

    /**
     * @brief Callback when MIDI note event received on a track
     * Parameters: (trackId, noteEvent)
     * Called from audio thread - keep handlers lightweight!
     */
    std::function<void(TrackId, const MidiNoteEvent&)> onNoteEvent;

    /**
     * @brief Callback when MIDI CC event received on a track
     * Parameters: (trackId, ccEvent)
     * Called from audio thread - keep handlers lightweight!
     */
    std::function<void(TrackId, const MidiCCEvent&)> onCCEvent;

    /**
     * @brief Start monitoring MIDI events for a track
     * Enables callbacks for note/CC events
     */
    void startMonitoring(TrackId trackId);

    /**
     * @brief Stop monitoring MIDI events for a track
     */
    void stopMonitoring(TrackId trackId);

    /**
     * @brief Check if monitoring is active for a track
     */
    bool isMonitoring(TrackId trackId) const;

  private:
    // MidiInputCallback implementation
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;

    te::Engine& engine_;

    // AudioBridge reference for triggering MIDI activity (not owned)
    AudioBridge* audioBridge_ = nullptr;

    // Track MIDI input routing (trackId → MIDI device ID)
    std::unordered_map<TrackId, juce::String> trackMidiInputs_;

    // Tracks being monitored for MIDI activity
    std::unordered_set<TrackId> monitoredTracks_;

    // Active MIDI input listeners (deviceId → MidiInput)
    std::unordered_map<juce::String, std::unique_ptr<juce::MidiInput>> activeMidiInputs_;

    // Synchronization for UI thread access
    juce::CriticalSection routingLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiBridge)
};

}  // namespace magda
