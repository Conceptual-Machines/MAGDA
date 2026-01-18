#pragma once

#include <memory>
#include <vector>

#include "TrackInfo.hpp"

namespace magica {

/**
 * @brief Master channel state
 */
struct MasterChannelState {
    float volume = 1.0f;
    float pan = 0.0f;
    bool muted = false;
    bool soloed = false;
};

/**
 * @brief Listener interface for track changes
 */
class TrackManagerListener {
  public:
    virtual ~TrackManagerListener() = default;

    // Called when tracks are added, removed, or reordered
    virtual void tracksChanged() = 0;

    // Called when a specific track's properties change
    virtual void trackPropertyChanged(int trackId) {
        juce::ignoreUnused(trackId);
    }

    // Called when master channel properties change
    virtual void masterChannelChanged() {}
};

/**
 * @brief Singleton manager for all tracks in the project
 *
 * Provides CRUD operations for tracks and notifies listeners of changes.
 */
class TrackManager {
  public:
    static TrackManager& getInstance();

    // Prevent copying
    TrackManager(const TrackManager&) = delete;
    TrackManager& operator=(const TrackManager&) = delete;

    // Track operations
    int createTrack(const juce::String& name = "");
    void deleteTrack(int trackId);
    void duplicateTrack(int trackId);
    void moveTrack(int trackId, int newIndex);

    // Access
    const std::vector<TrackInfo>& getTracks() const {
        return tracks_;
    }
    TrackInfo* getTrack(int trackId);
    const TrackInfo* getTrack(int trackId) const;
    int getTrackIndex(int trackId) const;
    int getNumTracks() const {
        return static_cast<int>(tracks_.size());
    }

    // Track property setters (notify listeners)
    void setTrackName(int trackId, const juce::String& name);
    void setTrackColour(int trackId, juce::Colour colour);
    void setTrackVolume(int trackId, float volume);
    void setTrackPan(int trackId, float pan);
    void setTrackMuted(int trackId, bool muted);
    void setTrackSoloed(int trackId, bool soloed);
    void setTrackRecordArmed(int trackId, bool armed);

    // Master channel
    const MasterChannelState& getMasterChannel() const {
        return masterChannel_;
    }
    void setMasterVolume(float volume);
    void setMasterPan(float pan);
    void setMasterMuted(bool muted);
    void setMasterSoloed(bool soloed);

    // Listener management
    void addListener(TrackManagerListener* listener);
    void removeListener(TrackManagerListener* listener);

    // Initialize with default tracks
    void createDefaultTracks(int count = 8);
    void clearAllTracks();

  private:
    TrackManager();
    ~TrackManager() = default;

    std::vector<TrackInfo> tracks_;
    std::vector<TrackManagerListener*> listeners_;
    int nextTrackId_ = 1;
    MasterChannelState masterChannel_;

    void notifyTracksChanged();
    void notifyTrackPropertyChanged(int trackId);
    void notifyMasterChannelChanged();

    juce::String generateTrackName() const;
};

}  // namespace magica
