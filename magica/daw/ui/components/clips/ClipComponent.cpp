#include "ClipComponent.hpp"

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"
#include "../tracks/TrackContentPanel.hpp"
#include "core/SelectionManager.hpp"

namespace magica {

ClipComponent::ClipComponent(ClipId clipId, TrackContentPanel* parent)
    : clipId_(clipId), parentPanel_(parent) {
    setName("ClipComponent");

    // Register as ClipManager listener
    ClipManager::getInstance().addListener(this);

    // Check if this clip is currently selected
    isSelected_ = ClipManager::getInstance().getSelectedClip() == clipId_;
}

ClipComponent::~ClipComponent() {
    ClipManager::getInstance().removeListener(this);
}

void ClipComponent::paint(juce::Graphics& g) {
    const auto* clip = getClipInfo();
    if (!clip) {
        return;
    }

    auto bounds = getLocalBounds();

    // Draw based on clip type
    if (clip->type == ClipType::Audio) {
        paintAudioClip(g, *clip, bounds);
    } else {
        paintMidiClip(g, *clip, bounds);
    }

    // Draw header (name, loop indicator)
    paintClipHeader(g, *clip, bounds);

    // Draw resize handles if selected
    if (isSelected_) {
        paintResizeHandles(g, bounds);
    }

    // Selection border
    if (isSelected_) {
        g.setColour(juce::Colours::white);
        g.drawRect(bounds, 2);
    }
}

void ClipComponent::paintAudioClip(juce::Graphics& g, const ClipInfo& clip,
                                   juce::Rectangle<int> bounds) {
    // Background - slightly darker than clip colour
    auto bgColour = clip.colour.darker(0.3f);
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds.toFloat(), CORNER_RADIUS);

    // Waveform placeholder - draw simplified representation
    auto waveformArea = bounds.reduced(2, HEADER_HEIGHT + 2);
    g.setColour(clip.colour.brighter(0.2f));

    // Draw a simple sine wave representation
    juce::Path waveform;
    waveform.startNewSubPath(waveformArea.getX(), waveformArea.getCentreY());

    float amplitude = waveformArea.getHeight() * 0.3f;
    for (int x = 0; x < waveformArea.getWidth(); x += 3) {
        float phase = static_cast<float>(x) / 20.0f;
        float y = waveformArea.getCentreY() + std::sin(phase) * amplitude;
        waveform.lineTo(waveformArea.getX() + x, y);
    }

    g.strokePath(waveform, juce::PathStrokeType(1.5f));

    // Border
    g.setColour(clip.colour);
    g.drawRoundedRectangle(bounds.toFloat(), CORNER_RADIUS, 1.0f);
}

void ClipComponent::paintMidiClip(juce::Graphics& g, const ClipInfo& clip,
                                  juce::Rectangle<int> bounds) {
    // Background
    auto bgColour = clip.colour.darker(0.3f);
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds.toFloat(), CORNER_RADIUS);

    // MIDI note representation area
    auto noteArea = bounds.reduced(2, HEADER_HEIGHT + 2);

    // Draw MIDI notes if we have them
    if (!clip.midiNotes.empty() && noteArea.getHeight() > 5) {
        g.setColour(clip.colour.brighter(0.3f));

        // Find note range
        int minNote = 127, maxNote = 0;
        double maxBeat = 0;
        for (const auto& note : clip.midiNotes) {
            minNote = juce::jmin(minNote, note.noteNumber);
            maxNote = juce::jmax(maxNote, note.noteNumber);
            maxBeat = juce::jmax(maxBeat, note.startBeat + note.lengthBeats);
        }

        int noteRange = juce::jmax(1, maxNote - minNote);
        double beatRange = juce::jmax(1.0, maxBeat);

        // Draw each note as a small rectangle
        for (const auto& note : clip.midiNotes) {
            float noteY = noteArea.getY() +
                          (maxNote - note.noteNumber) * noteArea.getHeight() / (noteRange + 1);
            float noteHeight =
                juce::jmax(2.0f, static_cast<float>(noteArea.getHeight()) / (noteRange + 1) - 1.0f);
            float noteX = noteArea.getX() +
                          static_cast<float>(note.startBeat / beatRange) * noteArea.getWidth();
            float noteWidth = juce::jmax(2.0f, static_cast<float>(note.lengthBeats / beatRange) *
                                                   noteArea.getWidth());

            g.fillRoundedRectangle(noteX, noteY, noteWidth, noteHeight, 1.0f);
        }
    } else {
        // Draw placeholder pattern for empty MIDI clip
        g.setColour(clip.colour.withAlpha(0.3f));
        for (int i = 0; i < 4; i++) {
            int y = noteArea.getY() + i * (noteArea.getHeight() / 4);
            g.drawHorizontalLine(y, noteArea.getX(), noteArea.getRight());
        }
    }

    // Border
    g.setColour(clip.colour);
    g.drawRoundedRectangle(bounds.toFloat(), CORNER_RADIUS, 1.0f);
}

void ClipComponent::paintClipHeader(juce::Graphics& g, const ClipInfo& clip,
                                    juce::Rectangle<int> bounds) {
    auto headerArea = bounds.removeFromTop(HEADER_HEIGHT);

    // Header background
    g.setColour(clip.colour);
    g.fillRoundedRectangle(headerArea.toFloat().withBottom(headerArea.getBottom() + 2),
                           CORNER_RADIUS);

    // Clip name
    if (bounds.getWidth() > MIN_WIDTH_FOR_NAME) {
        g.setColour(DarkTheme::getColour(DarkTheme::BACKGROUND));
        g.setFont(FontManager::getInstance().getUIFont(10.0f));
        g.drawText(clip.name, headerArea.reduced(4, 0), juce::Justification::centredLeft, true);
    }

    // Loop indicator
    if (clip.internalLoopEnabled) {
        auto loopArea = headerArea.removeFromRight(14).reduced(2);
        g.setColour(DarkTheme::getColour(DarkTheme::BACKGROUND));
        g.drawText("L", loopArea, juce::Justification::centred, false);
    }
}

void ClipComponent::paintResizeHandles(juce::Graphics& g, juce::Rectangle<int> bounds) {
    auto handleColour = juce::Colours::white.withAlpha(0.5f);

    // Left handle
    auto leftHandle = bounds.removeFromLeft(RESIZE_HANDLE_WIDTH);
    if (hoverLeftEdge_) {
        g.setColour(handleColour);
        g.fillRect(leftHandle);
    }

    // Right handle
    auto rightHandle = bounds.removeFromRight(RESIZE_HANDLE_WIDTH);
    if (hoverRightEdge_) {
        g.setColour(handleColour);
        g.fillRect(rightHandle);
    }
}

void ClipComponent::resized() {
    // Nothing to do - clip bounds are set by parent
}

// ============================================================================
// Mouse Handling
// ============================================================================

void ClipComponent::mouseDown(const juce::MouseEvent& e) {
    const auto* clip = getClipInfo();
    if (!clip) {
        return;
    }

    // Select this clip
    setSelected(true);
    SelectionManager::getInstance().selectClip(clipId_);

    if (onClipSelected) {
        onClipSelected(clipId_);
    }

    // Store drag start info
    dragStartPos_ = e.getPosition();
    dragStartBoundsPos_ = getBounds().getPosition();
    dragStartTime_ = clip->startTime;
    dragStartLength_ = clip->length;
    dragStartTrackId_ = clip->trackId;

    // Initialize preview state
    previewStartTime_ = clip->startTime;
    previewLength_ = clip->length;
    isDragging_ = false;

    // Determine drag mode based on click position
    if (isOnLeftEdge(e.x)) {
        dragMode_ = DragMode::ResizeLeft;
    } else if (isOnRightEdge(e.x)) {
        dragMode_ = DragMode::ResizeRight;
    } else {
        dragMode_ = DragMode::Move;
    }
}

void ClipComponent::mouseDrag(const juce::MouseEvent& e) {
    if (dragMode_ == DragMode::None || !parentPanel_) {
        return;
    }

    const auto* clip = getClipInfo();
    if (!clip) {
        return;
    }

    isDragging_ = true;

    // Convert pixel delta to time delta
    double pixelsPerSecond = parentPanel_->getZoom();
    if (pixelsPerSecond <= 0) {
        return;
    }

    int deltaX = e.x - dragStartPos_.x;
    double deltaTime = deltaX / pixelsPerSecond;

    switch (dragMode_) {
        case DragMode::Move: {
            // Calculate raw position
            double rawStartTime = juce::jmax(0.0, dragStartTime_ + deltaTime);
            previewStartTime_ = rawStartTime;

            // Calculate visual position - use magnetic snapping
            int newX = dragStartBoundsPos_.x + deltaX;

            // Check for magnetic snap if snap callback is available
            if (snapTimeToGrid) {
                double snappedTime = snapTimeToGrid(rawStartTime);
                int snappedX = static_cast<int>(snappedTime * pixelsPerSecond);
                int rawX = static_cast<int>(rawStartTime * pixelsPerSecond);

                // If within threshold, snap visually
                if (std::abs(snappedX - rawX) <= SNAP_THRESHOLD_PIXELS) {
                    newX = snappedX;
                    previewStartTime_ = snappedTime;  // Update preview to snapped position
                }
            }

            newX = juce::jmax(0, newX);
            setBounds(newX, getY(), getWidth(), getHeight());

            // Check for vertical track change (this still needs immediate feedback)
            auto screenPos = e.getScreenPosition();
            auto parentPos = parentPanel_->getScreenBounds().getPosition();
            int localY = screenPos.y - parentPos.y;
            int trackIndex = parentPanel_->getTrackIndexAtY(localY);

            if (trackIndex >= 0) {
                auto visibleTracks = TrackManager::getInstance().getVisibleTracks(
                    ViewModeController::getInstance().getViewMode());

                if (trackIndex < static_cast<int>(visibleTracks.size())) {
                    TrackId newTrackId = visibleTracks[trackIndex];
                    if (newTrackId != dragStartTrackId_ && onClipMovedToTrack) {
                        onClipMovedToTrack(clipId_, newTrackId);
                        dragStartTrackId_ = newTrackId;
                    }
                }
            }
            break;
        }

        case DragMode::ResizeLeft: {
            // Calculate raw values
            double rawStartTime = juce::jmax(0.0, dragStartTime_ + deltaTime);
            double rawLength = juce::jmax(0.1, dragStartLength_ - deltaTime);

            previewStartTime_ = rawStartTime;
            previewLength_ = rawLength;

            int newX = dragStartBoundsPos_.x + deltaX;

            // Magnetic snap for left edge
            if (snapTimeToGrid) {
                double snappedTime = snapTimeToGrid(rawStartTime);
                int snappedX = static_cast<int>(snappedTime * pixelsPerSecond);
                int rawX = static_cast<int>(rawStartTime * pixelsPerSecond);

                if (std::abs(snappedX - rawX) <= SNAP_THRESHOLD_PIXELS) {
                    newX = snappedX;
                    previewStartTime_ = snappedTime;
                    previewLength_ = dragStartLength_ - (snappedTime - dragStartTime_);
                }
            }

            int newWidth = static_cast<int>(previewLength_ * pixelsPerSecond);
            newX = juce::jmax(0, newX);
            newWidth = juce::jmax(10, newWidth);
            setBounds(newX, getY(), newWidth, getHeight());
            break;
        }

        case DragMode::ResizeRight: {
            // Calculate raw length
            double rawLength = juce::jmax(0.1, dragStartLength_ + deltaTime);
            previewLength_ = rawLength;

            double rawEndTime = dragStartTime_ + rawLength;
            int newWidth = static_cast<int>(rawLength * pixelsPerSecond);

            // Magnetic snap for right edge (end time)
            if (snapTimeToGrid) {
                double snappedEndTime = snapTimeToGrid(rawEndTime);
                int snappedWidth =
                    static_cast<int>((snappedEndTime - dragStartTime_) * pixelsPerSecond);
                int rawWidth = newWidth;

                if (std::abs(snappedWidth - rawWidth) <= SNAP_THRESHOLD_PIXELS) {
                    newWidth = snappedWidth;
                    previewLength_ = snappedEndTime - dragStartTime_;
                }
            }

            newWidth = juce::jmax(10, newWidth);
            setBounds(getX(), getY(), newWidth, getHeight());
            break;
        }

        default:
            break;
    }
}

void ClipComponent::mouseUp(const juce::MouseEvent& /*e*/) {
    if (isDragging_ && dragMode_ != DragMode::None) {
        // Now apply snapping and commit to ClipManager
        switch (dragMode_) {
            case DragMode::Move: {
                double finalStartTime = previewStartTime_;
                if (snapTimeToGrid) {
                    finalStartTime = snapTimeToGrid(finalStartTime);
                }
                finalStartTime = juce::jmax(0.0, finalStartTime);

                if (onClipMoved) {
                    onClipMoved(clipId_, finalStartTime);
                }
                break;
            }

            case DragMode::ResizeLeft: {
                double finalStartTime = previewStartTime_;
                double finalLength = previewLength_;

                if (snapTimeToGrid) {
                    finalStartTime = snapTimeToGrid(finalStartTime);
                    finalLength = dragStartLength_ - (finalStartTime - dragStartTime_);
                }

                finalStartTime = juce::jmax(0.0, finalStartTime);
                finalLength = juce::jmax(0.1, finalLength);

                if (onClipResized) {
                    onClipResized(clipId_, finalLength, true);
                }
                if (onClipMoved) {
                    onClipMoved(clipId_, finalStartTime);
                }
                break;
            }

            case DragMode::ResizeRight: {
                double finalLength = previewLength_;

                if (snapTimeToGrid) {
                    double endTime = snapTimeToGrid(dragStartTime_ + finalLength);
                    finalLength = endTime - dragStartTime_;
                }

                finalLength = juce::jmax(0.1, finalLength);

                if (onClipResized) {
                    onClipResized(clipId_, finalLength, false);
                }
                break;
            }

            default:
                break;
        }
    }

    dragMode_ = DragMode::None;
    isDragging_ = false;
}

void ClipComponent::mouseMove(const juce::MouseEvent& e) {
    bool wasHoverLeft = hoverLeftEdge_;
    bool wasHoverRight = hoverRightEdge_;

    hoverLeftEdge_ = isOnLeftEdge(e.x);
    hoverRightEdge_ = isOnRightEdge(e.x);

    if (hoverLeftEdge_ != wasHoverLeft || hoverRightEdge_ != wasHoverRight) {
        updateCursor();
        repaint();
    }
}

void ClipComponent::mouseExit(const juce::MouseEvent& /*e*/) {
    hoverLeftEdge_ = false;
    hoverRightEdge_ = false;
    updateCursor();
    repaint();
}

void ClipComponent::mouseDoubleClick(const juce::MouseEvent& /*e*/) {
    if (onClipDoubleClicked) {
        onClipDoubleClicked(clipId_);
    }
}

// ============================================================================
// ClipManagerListener
// ============================================================================

void ClipComponent::clipsChanged() {
    // Clip may have been deleted
    const auto* clip = getClipInfo();
    if (!clip) {
        // This clip was deleted - parent should remove this component
        return;
    }
    repaint();
}

void ClipComponent::clipPropertyChanged(ClipId clipId) {
    if (clipId == clipId_) {
        repaint();
    }
}

void ClipComponent::clipSelectionChanged(ClipId clipId) {
    bool wasSelected = isSelected_;
    isSelected_ = (clipId == clipId_);

    if (wasSelected != isSelected_) {
        repaint();
    }
}

// ============================================================================
// Selection
// ============================================================================

void ClipComponent::setSelected(bool selected) {
    if (isSelected_ != selected) {
        isSelected_ = selected;
        repaint();
    }
}

// ============================================================================
// Helpers
// ============================================================================

bool ClipComponent::isOnLeftEdge(int x) const {
    return x < RESIZE_HANDLE_WIDTH;
}

bool ClipComponent::isOnRightEdge(int x) const {
    return x > getWidth() - RESIZE_HANDLE_WIDTH;
}

void ClipComponent::updateCursor() {
    if (hoverLeftEdge_ || hoverRightEdge_) {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    } else {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
}

const ClipInfo* ClipComponent::getClipInfo() const {
    return ClipManager::getInstance().getClip(clipId_);
}

}  // namespace magica
