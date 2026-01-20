#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace magda {

/**
 * @brief Unique identifier for tracks
 */
using TrackId = int;
constexpr TrackId INVALID_TRACK_ID = -1;

/**
 * @brief Track types
 */
enum class TrackType {
    Audio,       // Audio clips, recording
    Instrument,  // MIDI → plugin → audio
    MIDI,        // MIDI only, routes to other tracks
    Group,       // Contains child tracks, routing hub
    Aux,         // Receives from sends
    Master       // Final output
};

/**
 * @brief Get display name for track type
 */
inline const char* getTrackTypeName(TrackType type) {
    switch (type) {
        case TrackType::Audio:
            return "Audio";
        case TrackType::Instrument:
            return "Instrument";
        case TrackType::MIDI:
            return "MIDI";
        case TrackType::Group:
            return "Group";
        case TrackType::Aux:
            return "Aux";
        case TrackType::Master:
            return "Master";
    }
    return "Unknown";
}

/**
 * @brief Check if track type can have children
 */
inline bool canHaveChildren(TrackType type) {
    return type == TrackType::Group;
}

/**
 * @brief Check if track type can contain MIDI
 */
inline bool canContainMIDI(TrackType type) {
    return type == TrackType::Instrument || type == TrackType::MIDI || type == TrackType::Group;
}

/**
 * @brief Check if track type can contain audio
 */
inline bool canContainAudio(TrackType type) {
    return type == TrackType::Audio;
}

}  // namespace magda
