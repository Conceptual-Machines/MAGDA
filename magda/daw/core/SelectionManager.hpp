#pragma once

#include <unordered_set>
#include <vector>

#include "ClipTypes.hpp"
#include "TrackTypes.hpp"

namespace magda {

/**
 * @brief Selection types in the DAW
 */
enum class SelectionType {
    None,       // Nothing selected
    Track,      // Track selected (for mixer/inspector)
    Clip,       // Single clip selected (backward compat)
    MultiClip,  // Multiple clips selected
    TimeRange,  // Time range selected (for operations)
    Note        // MIDI note(s) selected in piano roll
};

/**
 * @brief MIDI note selection data
 */
struct NoteSelection {
    ClipId clipId = INVALID_CLIP_ID;
    std::vector<size_t> noteIndices;  // Indices into clip's midiNotes vector

    bool isValid() const {
        return clipId != INVALID_CLIP_ID && !noteIndices.empty();
    }

    bool isSingleNote() const {
        return noteIndices.size() == 1;
    }

    size_t getCount() const {
        return noteIndices.size();
    }
};

/**
 * @brief Time range selection data
 */
struct TimeRangeSelection {
    double startTime = 0.0;
    double endTime = 0.0;
    std::vector<TrackId> trackIds;  // Which tracks are included

    bool isValid() const {
        return endTime > startTime && !trackIds.empty();
    }

    double getLength() const {
        return endTime - startTime;
    }
};

/**
 * @brief Listener interface for selection changes
 */
class SelectionManagerListener {
  public:
    virtual ~SelectionManagerListener() = default;

    virtual void selectionTypeChanged(SelectionType newType) = 0;
    virtual void trackSelectionChanged([[maybe_unused]] TrackId trackId) {}
    virtual void clipSelectionChanged([[maybe_unused]] ClipId clipId) {}
    virtual void multiClipSelectionChanged(
        [[maybe_unused]] const std::unordered_set<ClipId>& clipIds) {}
    virtual void timeRangeSelectionChanged([[maybe_unused]] const TimeRangeSelection& selection) {}
    virtual void noteSelectionChanged([[maybe_unused]] const NoteSelection& selection) {}
};

/**
 * @brief Singleton manager that coordinates selection state across the DAW
 *
 * Ensures only one type of selection is active at a time (track OR clip OR range)
 * and notifies listeners of changes.
 */
class SelectionManager {
  public:
    static SelectionManager& getInstance();

    // Prevent copying
    SelectionManager(const SelectionManager&) = delete;
    SelectionManager& operator=(const SelectionManager&) = delete;

    // ========================================================================
    // Selection State
    // ========================================================================

    SelectionType getSelectionType() const {
        return selectionType_;
    }

    // ========================================================================
    // Track Selection
    // ========================================================================

    /**
     * @brief Select a track (clears clip and range selection)
     */
    void selectTrack(TrackId trackId);

    /**
     * @brief Get the currently selected track
     * @return INVALID_TRACK_ID if no track selected
     */
    TrackId getSelectedTrack() const {
        return selectedTrackId_;
    }

    // ========================================================================
    // Clip Selection
    // ========================================================================

    /**
     * @brief Select a single clip (clears track and range selection)
     */
    void selectClip(ClipId clipId);

    /**
     * @brief Get the currently selected clip (backward compat)
     * @return INVALID_CLIP_ID if no clip selected or multiple clips selected
     */
    ClipId getSelectedClip() const {
        return selectedClipId_;
    }

    // ========================================================================
    // Multi-Clip Selection
    // ========================================================================

    /**
     * @brief Select multiple clips (clears other selection types)
     */
    void selectClips(const std::unordered_set<ClipId>& clipIds);

    /**
     * @brief Add a clip to the current selection
     * If not already in multi-clip mode, converts current selection to multi-clip
     */
    void addClipToSelection(ClipId clipId);

    /**
     * @brief Remove a clip from the current selection
     */
    void removeClipFromSelection(ClipId clipId);

    /**
     * @brief Toggle a clip's selection state (add if not selected, remove if selected)
     */
    void toggleClipSelection(ClipId clipId);

    /**
     * @brief Extend selection from anchor to target clip (Shift+click behavior)
     * Selects all clips in the rectangular region between anchor and target
     */
    void extendSelectionTo(ClipId targetClipId);

    /**
     * @brief Get the anchor clip (last single-clicked clip)
     */
    ClipId getAnchorClip() const {
        return anchorClipId_;
    }

    /**
     * @brief Get all selected clips
     */
    const std::unordered_set<ClipId>& getSelectedClips() const {
        return selectedClipIds_;
    }

    /**
     * @brief Check if a specific clip is selected
     */
    bool isClipSelected(ClipId clipId) const;

    /**
     * @brief Get the number of selected clips
     */
    size_t getSelectedClipCount() const {
        return selectedClipIds_.size();
    }

    // ========================================================================
    // Time Range Selection
    // ========================================================================

    /**
     * @brief Set a time range selection (clears track and clip selection)
     */
    void selectTimeRange(double startTime, double endTime, const std::vector<TrackId>& trackIds);

    /**
     * @brief Get the current time range selection
     */
    const TimeRangeSelection& getTimeRangeSelection() const {
        return timeRangeSelection_;
    }

    /**
     * @brief Check if there's a valid time range selection
     */
    bool hasTimeRangeSelection() const {
        return selectionType_ == SelectionType::TimeRange && timeRangeSelection_.isValid();
    }

    // ========================================================================
    // Note Selection
    // ========================================================================

    /**
     * @brief Select a single MIDI note (clears other selection types)
     */
    void selectNote(ClipId clipId, size_t noteIndex);

    /**
     * @brief Select multiple MIDI notes in the same clip
     */
    void selectNotes(ClipId clipId, const std::vector<size_t>& noteIndices);

    /**
     * @brief Add a note to the current selection
     */
    void addNoteToSelection(ClipId clipId, size_t noteIndex);

    /**
     * @brief Remove a note from the current selection
     */
    void removeNoteFromSelection(size_t noteIndex);

    /**
     * @brief Toggle a note's selection state
     */
    void toggleNoteSelection(ClipId clipId, size_t noteIndex);

    /**
     * @brief Get the current note selection
     */
    const NoteSelection& getNoteSelection() const {
        return noteSelection_;
    }

    /**
     * @brief Check if a specific note is selected
     */
    bool isNoteSelected(ClipId clipId, size_t noteIndex) const;

    /**
     * @brief Check if there's a valid note selection
     */
    bool hasNoteSelection() const {
        return selectionType_ == SelectionType::Note && noteSelection_.isValid();
    }

    // ========================================================================
    // Clear
    // ========================================================================

    /**
     * @brief Clear all selections
     */
    void clearSelection();

    // ========================================================================
    // Listeners
    // ========================================================================

    void addListener(SelectionManagerListener* listener);
    void removeListener(SelectionManagerListener* listener);

  private:
    SelectionManager();
    ~SelectionManager() = default;

    SelectionType selectionType_ = SelectionType::None;
    TrackId selectedTrackId_ = INVALID_TRACK_ID;
    ClipId selectedClipId_ = INVALID_CLIP_ID;
    ClipId anchorClipId_ = INVALID_CLIP_ID;       // Anchor for Shift+click range selection
    std::unordered_set<ClipId> selectedClipIds_;  // For multi-clip selection
    TimeRangeSelection timeRangeSelection_;
    NoteSelection noteSelection_;

    std::vector<SelectionManagerListener*> listeners_;

    void notifySelectionTypeChanged(SelectionType type);
    void notifyTrackSelectionChanged(TrackId trackId);
    void notifyClipSelectionChanged(ClipId clipId);
    void notifyMultiClipSelectionChanged(const std::unordered_set<ClipId>& clipIds);
    void notifyTimeRangeSelectionChanged(const TimeRangeSelection& selection);
    void notifyNoteSelectionChanged(const NoteSelection& selection);
};

}  // namespace magda
