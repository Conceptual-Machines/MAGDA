#include "MidiBridge.hpp"

#include "AudioBridge.hpp"

namespace magda {

MidiBridge::MidiBridge(te::Engine& engine) : engine_(engine) {
    DBG("MidiBridge initialized");
}

void MidiBridge::setAudioBridge(AudioBridge* audioBridge) {
    audioBridge_ = audioBridge;
}

MidiBridge::~MidiBridge() {
    // IMPORTANT: Release MIDI inputs carefully to avoid CoreMIDI crashes
    // Don't hold lock while destroying MIDI inputs (can cause deadlock)
    std::unordered_map<juce::String, std::unique_ptr<juce::MidiInput>> inputsToDestroy;

    {
        juce::ScopedLock lock(routingLock_);
        inputsToDestroy = std::move(activeMidiInputs_);
        activeMidiInputs_.clear();
        trackMidiInputs_.clear();
        monitoredTracks_.clear();
    }

    // Destroy MIDI inputs outside of lock
    for (auto& [deviceId, midiInput] : inputsToDestroy) {
        if (midiInput) {
            midiInput->stop();
        }
    }
    inputsToDestroy.clear();
}

std::vector<MidiDeviceInfo> MidiBridge::getAvailableMidiInputs() const {
    std::vector<MidiDeviceInfo> devices;

    // Use JUCE's MidiInput::getAvailableDevices() instead of Tracktion's device manager
    // This works immediately without waiting for async scan
    auto midiInputs = juce::MidiInput::getAvailableDevices();

    for (const auto& device : midiInputs) {
        MidiDeviceInfo info;
        info.id = device.identifier;
        info.name = device.name;
        info.isEnabled = false;  // Input enable state handled separately
        info.isAvailable = true;

        devices.push_back(info);
    }

    DBG("Found " << devices.size() << " MIDI input devices");
    return devices;
}

std::vector<MidiDeviceInfo> MidiBridge::getAvailableMidiOutputs() const {
    std::vector<MidiDeviceInfo> devices;

    auto midiOutputs = juce::MidiOutput::getAvailableDevices();

    for (const auto& device : midiOutputs) {
        MidiDeviceInfo info;
        info.id = device.identifier;
        info.name = device.name;
        info.isEnabled = false;  // Output enable state not tracked same way
        info.isAvailable = true;

        devices.push_back(info);
    }

    return devices;
}

void MidiBridge::enableMidiInput(const juce::String& deviceId) {
    juce::ScopedLock lock(routingLock_);

    // Check if already listening
    if (activeMidiInputs_.find(deviceId) != activeMidiInputs_.end()) {
        return;  // Already active
    }

    // Open MIDI input and start listening
    auto availableDevices = juce::MidiInput::getAvailableDevices();

    for (const auto& deviceInfo : availableDevices) {
        if (deviceInfo.identifier == deviceId) {
            auto midiInput = juce::MidiInput::openDevice(deviceInfo.identifier, this);
            if (midiInput) {
                midiInput->start();
                activeMidiInputs_[deviceId] = std::move(midiInput);
                DBG("Started MIDI activity monitoring for: " << deviceInfo.name);
            }
            break;
        }
    }
}

void MidiBridge::disableMidiInput(const juce::String& deviceId) {
    // Stop our MIDI input listener
    juce::ScopedLock lock(routingLock_);
    auto it = activeMidiInputs_.find(deviceId);
    if (it != activeMidiInputs_.end()) {
        if (it->second) {
            it->second->stop();
        }
        activeMidiInputs_.erase(it);
    }
}

bool MidiBridge::isMidiInputEnabled(const juce::String& deviceId) const {
    auto device = engine_.getDeviceManager().findMidiInputDeviceForID(deviceId);
    return device ? device->isEnabled() : false;
}

void MidiBridge::setTrackMidiInput(TrackId trackId, const juce::String& midiDeviceId) {
    juce::ScopedLock lock(routingLock_);

    if (midiDeviceId.isEmpty()) {
        // Clear routing
        trackMidiInputs_.erase(trackId);
    } else {
        trackMidiInputs_[trackId] = midiDeviceId;

        // Auto-enable the device if not already enabled
        if (midiDeviceId == "all") {
            // Special case: enable ALL MIDI input devices
            auto availableDevices = juce::MidiInput::getAvailableDevices();
            for (const auto& deviceInfo : availableDevices) {
                enableMidiInput(deviceInfo.identifier);
            }
        } else {
            // Single device
            enableMidiInput(midiDeviceId);
        }
    }
}

juce::String MidiBridge::getTrackMidiInput(TrackId trackId) const {
    juce::ScopedLock lock(routingLock_);

    auto it = trackMidiInputs_.find(trackId);
    if (it != trackMidiInputs_.end()) {
        return it->second;
    }
    return {};  // Empty string = no input
}

void MidiBridge::clearTrackMidiInput(TrackId trackId) {
    setTrackMidiInput(trackId, {});  // This will trigger the callback
}

void MidiBridge::handleIncomingMidiMessage(juce::MidiInput* source,
                                           const juce::MidiMessage& message) {
    if (!source || !audioBridge_) {
        return;
    }

    // ONLY trigger activity indicator for note on messages
    // Ignore MIDI clock, CC, etc. to avoid constant "on" state
    if (!message.isNoteOn()) {
        return;
    }

    // Get the device ID for this input
    juce::String sourceDeviceId = source->getIdentifier();

    // Find all tracks routing this MIDI input
    juce::ScopedLock lock(routingLock_);

    for (const auto& [trackId, deviceId] : trackMidiInputs_) {
        // Check if this track is routing this device (or "all" inputs)
        if (deviceId == sourceDeviceId || deviceId == "all") {
            // Check if monitoring is enabled for this track
            if (monitoredTracks_.find(trackId) != monitoredTracks_.end()) {
                // Trigger MIDI activity indicator
                audioBridge_->triggerMidiActivity(trackId);

                // Call callbacks if set (for note/CC monitoring)
                if (message.isNoteOn() || message.isNoteOff()) {
                    if (onNoteEvent) {
                        MidiNoteEvent noteEvent;
                        noteEvent.noteNumber = message.getNoteNumber();
                        noteEvent.velocity = message.getVelocity();
                        noteEvent.isNoteOn = message.isNoteOn();
                        onNoteEvent(trackId, noteEvent);
                    }
                } else if (message.isController()) {
                    if (onCCEvent) {
                        MidiCCEvent ccEvent;
                        ccEvent.controller = message.getControllerNumber();
                        ccEvent.value = message.getControllerValue();
                        onCCEvent(trackId, ccEvent);
                    }
                }
            }
        }
    }
}

void MidiBridge::startMonitoring(TrackId trackId) {
    juce::ScopedLock lock(routingLock_);
    monitoredTracks_.insert(trackId);
}

void MidiBridge::stopMonitoring(TrackId trackId) {
    juce::ScopedLock lock(routingLock_);
    monitoredTracks_.erase(trackId);
    DBG("Stopped MIDI monitoring for track " << trackId);
}

bool MidiBridge::isMonitoring(TrackId trackId) const {
    juce::ScopedLock lock(routingLock_);
    return monitoredTracks_.find(trackId) != monitoredTracks_.end();
}

}  // namespace magda
