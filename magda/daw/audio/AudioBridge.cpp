#include "AudioBridge.hpp"

#include <iostream>

namespace magda {

AudioBridge::AudioBridge(te::Engine& engine, te::Edit& edit) : engine_(engine), edit_(edit) {
    // Register as TrackManager listener
    TrackManager::getInstance().addListener(this);

    // Start timer for metering updates (30 FPS for smooth UI)
    startTimerHz(30);

    std::cout << "AudioBridge initialized" << std::endl;
}

AudioBridge::~AudioBridge() {
    stopTimer();
    TrackManager::getInstance().removeListener(this);

    // Clear all mappings
    {
        juce::ScopedLock lock(mappingLock_);
        trackMapping_.clear();
        deviceToPlugin_.clear();
        pluginToDevice_.clear();
        trackMeasurers_.clear();
    }

    std::cout << "AudioBridge destroyed" << std::endl;
}

// =============================================================================
// TrackManagerListener implementation
// =============================================================================

void AudioBridge::tracksChanged() {
    // Tracks were added/removed/reordered - sync all
    syncAll();
}

void AudioBridge::trackPropertyChanged(int trackId) {
    // Track property changed (mute, solo, etc.) - may need to sync
    auto* track = getAudioTrack(trackId);
    if (track) {
        auto* trackInfo = TrackManager::getInstance().getTrack(trackId);
        if (trackInfo) {
            track->setMute(trackInfo->muted);
            track->setSolo(trackInfo->soloed);
        }
    }
}

void AudioBridge::trackDevicesChanged(TrackId trackId) {
    // Devices on a track changed - resync that track's plugins
    syncTrackPlugins(trackId);
}

// =============================================================================
// Plugin Loading
// =============================================================================

te::Plugin::Ptr AudioBridge::loadBuiltInPlugin(TrackId trackId, const juce::String& type) {
    auto* track = getAudioTrack(trackId);
    if (!track) {
        // Create track if it doesn't exist
        auto* trackInfo = TrackManager::getInstance().getTrack(trackId);
        juce::String name = trackInfo ? trackInfo->name : "Track";
        track = createAudioTrack(trackId, name);
    }

    if (!track)
        return nullptr;

    te::Plugin::Ptr plugin;

    if (type.equalsIgnoreCase("tone") || type.equalsIgnoreCase("tonegenerator")) {
        plugin = createToneGenerator(track);
    } else if (type.equalsIgnoreCase("volume") || type.equalsIgnoreCase("volumeandpan")) {
        plugin = createVolumeAndPan(track);
    } else if (type.equalsIgnoreCase("meter") || type.equalsIgnoreCase("levelmeter")) {
        plugin = createLevelMeter(track);
    } else if (type.equalsIgnoreCase("delay")) {
        plugin = edit_.getPluginCache().createNewPlugin(te::DelayPlugin::xmlTypeName, {});
        if (plugin)
            track->pluginList.insertPlugin(plugin, -1, nullptr);
    } else if (type.equalsIgnoreCase("reverb")) {
        plugin = edit_.getPluginCache().createNewPlugin(te::ReverbPlugin::xmlTypeName, {});
        if (plugin)
            track->pluginList.insertPlugin(plugin, -1, nullptr);
    } else if (type.equalsIgnoreCase("eq") || type.equalsIgnoreCase("equaliser")) {
        plugin = edit_.getPluginCache().createNewPlugin(te::EqualiserPlugin::xmlTypeName, {});
        if (plugin)
            track->pluginList.insertPlugin(plugin, -1, nullptr);
    } else if (type.equalsIgnoreCase("compressor")) {
        plugin = edit_.getPluginCache().createNewPlugin(te::CompressorPlugin::xmlTypeName, {});
        if (plugin)
            track->pluginList.insertPlugin(plugin, -1, nullptr);
    } else if (type.equalsIgnoreCase("chorus")) {
        plugin = edit_.getPluginCache().createNewPlugin(te::ChorusPlugin::xmlTypeName, {});
        if (plugin)
            track->pluginList.insertPlugin(plugin, -1, nullptr);
    } else if (type.equalsIgnoreCase("phaser")) {
        plugin = edit_.getPluginCache().createNewPlugin(te::PhaserPlugin::xmlTypeName, {});
        if (plugin)
            track->pluginList.insertPlugin(plugin, -1, nullptr);
    }

    if (plugin) {
        std::cout << "Loaded built-in plugin: " << type << " on track " << trackId << std::endl;
    } else {
        std::cerr << "Failed to load built-in plugin: " << type << std::endl;
    }

    return plugin;
}

te::Plugin::Ptr AudioBridge::loadExternalPlugin(TrackId trackId,
                                                const juce::PluginDescription& description) {
    auto* track = getAudioTrack(trackId);
    if (!track) {
        auto* trackInfo = TrackManager::getInstance().getTrack(trackId);
        juce::String name = trackInfo ? trackInfo->name : "Track";
        track = createAudioTrack(trackId, name);
    }

    if (!track)
        return nullptr;

    // Create external plugin using the description
    auto plugin =
        edit_.getPluginCache().createNewPlugin(te::ExternalPlugin::xmlTypeName, description);

    if (plugin) {
        track->pluginList.insertPlugin(plugin, -1, nullptr);
        std::cout << "Loaded external plugin: " << description.name << " on track " << trackId
                  << std::endl;
    } else {
        std::cerr << "Failed to load external plugin: " << description.name << std::endl;
    }

    return plugin;
}

te::Plugin::Ptr AudioBridge::addLevelMeterToTrack(TrackId trackId) {
    return loadBuiltInPlugin(trackId, "levelmeter");
}

// =============================================================================
// Track Mapping
// =============================================================================

te::AudioTrack* AudioBridge::getAudioTrack(TrackId trackId) {
    juce::ScopedLock lock(mappingLock_);
    auto it = trackMapping_.find(trackId);
    return it != trackMapping_.end() ? it->second : nullptr;
}

te::Plugin::Ptr AudioBridge::getPlugin(DeviceId deviceId) {
    juce::ScopedLock lock(mappingLock_);
    auto it = deviceToPlugin_.find(deviceId);
    return it != deviceToPlugin_.end() ? it->second : nullptr;
}

te::AudioTrack* AudioBridge::createAudioTrack(TrackId trackId, const juce::String& name) {
    // Check if track already exists
    {
        juce::ScopedLock lock(mappingLock_);
        auto it = trackMapping_.find(trackId);
        if (it != trackMapping_.end() && it->second != nullptr) {
            return it->second;
        }
    }

    // Insert new track at the end
    auto insertPoint = te::TrackInsertPoint(nullptr, nullptr);
    auto trackPtr = edit_.insertNewAudioTrack(insertPoint, nullptr);

    te::AudioTrack* track = trackPtr.get();
    if (track) {
        track->setName(name);

        // Add a LevelMeterPlugin for metering
        auto meterPlugin = createLevelMeter(track);
        if (meterPlugin) {
            std::cout << "Added LevelMeterPlugin to track " << trackId << std::endl;
        }

        juce::ScopedLock lock(mappingLock_);
        trackMapping_[trackId] = track;

        std::cout << "Created Tracktion AudioTrack for MAGDA track " << trackId << ": " << name
                  << std::endl;
    }

    return track;
}

void AudioBridge::removeAudioTrack(TrackId trackId) {
    te::AudioTrack* track = nullptr;

    {
        juce::ScopedLock lock(mappingLock_);
        auto it = trackMapping_.find(trackId);
        if (it != trackMapping_.end()) {
            track = it->second;
            trackMapping_.erase(it);
        }

        // Remove any associated measurers
        trackMeasurers_.erase(trackId);
    }

    if (track) {
        edit_.deleteTrack(track);
        std::cout << "Removed Tracktion AudioTrack for MAGDA track " << trackId << std::endl;
    }
}

// =============================================================================
// Parameter Queue
// =============================================================================

bool AudioBridge::pushParameterChange(DeviceId deviceId, int paramIndex, float value) {
    ParameterChange change;
    change.deviceId = deviceId;
    change.paramIndex = paramIndex;
    change.value = value;
    change.source = ParameterChange::Source::User;
    return parameterQueue_.push(change);
}

// =============================================================================
// Synchronization
// =============================================================================

void AudioBridge::syncAll() {
    auto& tm = TrackManager::getInstance();
    const auto& tracks = tm.getTracks();

    std::cout << "AudioBridge: syncing " << tracks.size() << " tracks" << std::endl;

    for (const auto& track : tracks) {
        ensureTrackMapping(track.id);
        syncTrackPlugins(track.id);
    }
}

void AudioBridge::syncTrackPlugins(TrackId trackId) {
    auto* trackInfo = TrackManager::getInstance().getTrack(trackId);
    if (!trackInfo)
        return;

    auto* teTrack = getAudioTrack(trackId);
    if (!teTrack) {
        teTrack = createAudioTrack(trackId, trackInfo->name);
    }

    if (!teTrack)
        return;

    // For Phase 1, we'll sync top-level devices on the track
    // (Full nested rack support comes in Phase 3)

    // Get current MAGDA devices
    std::vector<DeviceId> magdaDevices;
    for (const auto& element : trackInfo->chainElements) {
        if (std::holds_alternative<DeviceInfo>(element)) {
            magdaDevices.push_back(std::get<DeviceInfo>(element).id);
        }
    }

    // Remove TE plugins that no longer exist in MAGDA
    {
        juce::ScopedLock lock(mappingLock_);
        std::vector<DeviceId> toRemove;
        for (const auto& [deviceId, plugin] : deviceToPlugin_) {
            auto pluginIt = pluginToDevice_.find(plugin.get());
            if (pluginIt != pluginToDevice_.end()) {
                // Check if this plugin belongs to this track
                auto* owner = plugin->getOwnerTrack();
                if (owner == teTrack) {
                    // Check if device still exists in MAGDA
                    bool found = std::find(magdaDevices.begin(), magdaDevices.end(), deviceId) !=
                                 magdaDevices.end();
                    if (!found) {
                        toRemove.push_back(deviceId);
                    }
                }
            }
        }

        for (auto deviceId : toRemove) {
            auto it = deviceToPlugin_.find(deviceId);
            if (it != deviceToPlugin_.end()) {
                auto plugin = it->second;
                pluginToDevice_.erase(plugin.get());
                deviceToPlugin_.erase(it);
                plugin->deleteFromParent();
            }
        }
    }

    // Add new plugins for MAGDA devices that don't have TE counterparts
    for (const auto& element : trackInfo->chainElements) {
        if (std::holds_alternative<DeviceInfo>(element)) {
            const auto& device = std::get<DeviceInfo>(element);

            juce::ScopedLock lock(mappingLock_);
            if (deviceToPlugin_.find(device.id) == deviceToPlugin_.end()) {
                // Load this device as a plugin
                auto plugin = loadDeviceAsPlugin(trackId, device);
                if (plugin) {
                    deviceToPlugin_[device.id] = plugin;
                    pluginToDevice_[plugin.get()] = device.id;
                }
            }
        }
    }
}

void AudioBridge::ensureTrackMapping(TrackId trackId) {
    if (!getAudioTrack(trackId)) {
        auto* trackInfo = TrackManager::getInstance().getTrack(trackId);
        if (trackInfo) {
            createAudioTrack(trackId, trackInfo->name);
        }
    }
}

// =============================================================================
// Audio Callback Support
// =============================================================================

void AudioBridge::processParameterChanges() {
    ParameterChange change;
    while (parameterQueue_.pop(change)) {
        auto plugin = getPlugin(change.deviceId);
        if (plugin) {
            auto params = plugin->getAutomatableParameters();
            if (change.paramIndex >= 0 && change.paramIndex < static_cast<int>(params.size())) {
                params[static_cast<size_t>(change.paramIndex)]->setParameter(
                    change.value, juce::sendNotificationSync);
            }
        }
    }
}

void AudioBridge::updateMetering() {
    // This would be called from the audio thread
    // For now, we use the timer callback for metering
}

void AudioBridge::timerCallback() {
    // Update metering from level measurers
    juce::ScopedLock lock(mappingLock_);

    for (const auto& [trackId, track] : trackMapping_) {
        if (!track)
            continue;

        // Get the track's level measurer via the LevelMeterPlugin
        auto* levelMeterPlugin = track->getLevelMeterPlugin();
        if (!levelMeterPlugin)
            continue;

        auto& measurer = levelMeterPlugin->measurer;

        MeterData data;

        // Read peak levels from level cache
        auto [levelL, levelR] = measurer.getLevelCache();

        // Convert from dB to linear
        data.peakL = juce::Decibels::decibelsToGain(levelL);
        data.peakR = juce::Decibels::decibelsToGain(levelR);

        // Check for clipping
        data.clipped = data.peakL > 1.0f || data.peakR > 1.0f;

        // RMS would require accumulation over time - simplified for now
        data.rmsL = data.peakL * 0.7f;  // Rough approximation
        data.rmsR = data.peakR * 0.7f;

        meteringBuffer_.pushLevels(trackId, data);
    }
}

// =============================================================================
// Plugin Creation Helpers
// =============================================================================

te::Plugin::Ptr AudioBridge::createToneGenerator(te::AudioTrack* track) {
    if (!track)
        return nullptr;

    // Create tone generator plugin via PluginCache using xmlTypeName
    auto plugin = edit_.getPluginCache().createNewPlugin(te::ToneGeneratorPlugin::xmlTypeName, {});
    if (plugin) {
        track->pluginList.insertPlugin(plugin, -1, nullptr);
    }
    return plugin;
}

te::Plugin::Ptr AudioBridge::createVolumeAndPan(te::AudioTrack* track) {
    if (!track)
        return nullptr;

    // VolumeAndPanPlugin has create() that returns ValueTree
    auto plugin = edit_.getPluginCache().createNewPlugin(te::VolumeAndPanPlugin::create());
    if (plugin) {
        track->pluginList.insertPlugin(plugin, -1, nullptr);
    }
    return plugin;
}

te::Plugin::Ptr AudioBridge::createLevelMeter(te::AudioTrack* track) {
    if (!track)
        return nullptr;

    // LevelMeterPlugin has create() that returns ValueTree
    auto plugin = edit_.getPluginCache().createNewPlugin(te::LevelMeterPlugin::create());
    if (plugin) {
        track->pluginList.insertPlugin(plugin, -1, nullptr);
    }
    return plugin;
}

te::Plugin::Ptr AudioBridge::loadDeviceAsPlugin(TrackId trackId, const DeviceInfo& device) {
    auto* track = getAudioTrack(trackId);
    if (!track)
        return nullptr;

    te::Plugin::Ptr plugin;

    if (device.format == PluginFormat::Internal) {
        // Map internal device types to Tracktion plugins
        if (device.pluginId.containsIgnoreCase("tone")) {
            plugin = createToneGenerator(track);
        } else if (device.pluginId.containsIgnoreCase("volume")) {
            plugin = createVolumeAndPan(track);
        } else if (device.pluginId.containsIgnoreCase("meter")) {
            plugin = createLevelMeter(track);
        } else if (device.pluginId.containsIgnoreCase("delay")) {
            plugin = edit_.getPluginCache().createNewPlugin(te::DelayPlugin::xmlTypeName, {});
            if (plugin)
                track->pluginList.insertPlugin(plugin, -1, nullptr);
        } else if (device.pluginId.containsIgnoreCase("reverb")) {
            plugin = edit_.getPluginCache().createNewPlugin(te::ReverbPlugin::xmlTypeName, {});
            if (plugin)
                track->pluginList.insertPlugin(plugin, -1, nullptr);
        } else if (device.pluginId.containsIgnoreCase("eq")) {
            plugin = edit_.getPluginCache().createNewPlugin(te::EqualiserPlugin::xmlTypeName, {});
            if (plugin)
                track->pluginList.insertPlugin(plugin, -1, nullptr);
        } else if (device.pluginId.containsIgnoreCase("compressor")) {
            plugin = edit_.getPluginCache().createNewPlugin(te::CompressorPlugin::xmlTypeName, {});
            if (plugin)
                track->pluginList.insertPlugin(plugin, -1, nullptr);
        }
    } else {
        // External plugin - need to find matching description
        // This will be fully implemented in Phase 2
        std::cout << "External plugin loading deferred to Phase 2: " << device.name << std::endl;
    }

    if (plugin) {
        // Apply device state
        plugin->setEnabled(!device.bypassed);
        std::cout << "Loaded device " << device.id << " (" << device.name << ") as plugin"
                  << std::endl;
    }

    return plugin;
}

}  // namespace magda
