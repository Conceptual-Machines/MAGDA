#pragma once

#include <juce_events/juce_events.h>

namespace magda {

class AudioEngine;
class TimelineController;

/**
 * @brief Timer that polls the audio engine for playhead position updates
 *
 * This class periodically polls the AudioEngine for the current
 * playback position and dispatches SetPlaybackPositionEvent to the
 * TimelineController, which then notifies all listeners.
 */
class PlaybackPositionTimer : private juce::Timer {
  public:
    PlaybackPositionTimer(AudioEngine& engine, TimelineController& timeline);
    ~PlaybackPositionTimer() override;

    void start();
    void stop();
    bool isRunning() const;

  private:
    void timerCallback() override;

    AudioEngine& engine_;
    TimelineController& timeline_;

    static constexpr int UPDATE_INTERVAL_MS = 30;  // ~33fps for smooth playhead
};

}  // namespace magda
