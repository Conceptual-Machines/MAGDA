#include "VelocityLaneComponent.hpp"

#include "../../themes/DarkTheme.hpp"
#include "core/ClipInfo.hpp"
#include "core/ClipManager.hpp"

namespace magda {

VelocityLaneComponent::VelocityLaneComponent() {
    setName("VelocityLane");
    setOpaque(true);  // Ensure proper repainting during drag
}

void VelocityLaneComponent::setClip(ClipId clipId) {
    if (clipId_ != clipId) {
        clipId_ = clipId;
        repaint();
    }
}

void VelocityLaneComponent::setPixelsPerBeat(double ppb) {
    if (pixelsPerBeat_ != ppb) {
        pixelsPerBeat_ = ppb;
        repaint();
    }
}

void VelocityLaneComponent::setScrollOffset(int offsetX) {
    if (scrollOffsetX_ != offsetX) {
        scrollOffsetX_ = offsetX;
        repaint();
    }
}

void VelocityLaneComponent::setLeftPadding(int padding) {
    if (leftPadding_ != padding) {
        leftPadding_ = padding;
        repaint();
    }
}

void VelocityLaneComponent::setRelativeMode(bool relative) {
    if (relativeMode_ != relative) {
        relativeMode_ = relative;
        repaint();
    }
}

void VelocityLaneComponent::setClipStartBeats(double startBeats) {
    if (clipStartBeats_ != startBeats) {
        clipStartBeats_ = startBeats;
        repaint();
    }
}

void VelocityLaneComponent::refreshNotes() {
    repaint();
}

void VelocityLaneComponent::setNotePreviewPosition(size_t noteIndex, double previewBeat,
                                                   bool isDragging) {
    if (isDragging) {
        notePreviewPositions_[noteIndex] = previewBeat;
    } else {
        notePreviewPositions_.erase(noteIndex);
    }
    repaint();
}

int VelocityLaneComponent::beatToPixel(double beat) const {
    // In absolute mode, the beat is already absolute
    // In relative mode, we draw starting from 0
    return static_cast<int>(beat * pixelsPerBeat_) + leftPadding_ - scrollOffsetX_;
}

double VelocityLaneComponent::pixelToBeat(int x) const {
    return (x + scrollOffsetX_ - leftPadding_) / pixelsPerBeat_;
}

int VelocityLaneComponent::velocityToY(int velocity) const {
    // Velocity 127 = top, velocity 0 = bottom
    // Leave a small margin at top and bottom
    int margin = 2;
    int usableHeight = getHeight() - (margin * 2);
    int y = margin + usableHeight - (velocity * usableHeight / 127);
    return y;
}

int VelocityLaneComponent::yToVelocity(int y) const {
    int margin = 2;
    int usableHeight = getHeight() - (margin * 2);
    int velocity = 127 - ((y - margin) * 127 / usableHeight);
    return juce::jlimit(0, 127, velocity);
}

size_t VelocityLaneComponent::findNoteAtX(int x) const {
    const auto* clip = ClipManager::getInstance().getClip(clipId_);
    if (!clip || clip->type != ClipType::MIDI) {
        return SIZE_MAX;
    }

    double clickBeat = pixelToBeat(x);

    // Search for a note that contains this beat
    for (size_t i = 0; i < clip->midiNotes.size(); ++i) {
        const auto& note = clip->midiNotes[i];

        // In absolute mode, offset by clip start
        double noteStart = relativeMode_ ? note.startBeat : (clipStartBeats_ + note.startBeat);
        double noteEnd = noteStart + note.lengthBeats;

        if (clickBeat >= noteStart && clickBeat < noteEnd) {
            return i;
        }
    }

    return SIZE_MAX;
}

juce::Colour VelocityLaneComponent::getClipColour() const {
    const auto* clip = ClipManager::getInstance().getClip(clipId_);
    return clip ? clip->colour : DarkTheme::getAccentColour();
}

void VelocityLaneComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();

    // Background
    g.setColour(DarkTheme::getColour(DarkTheme::BACKGROUND_ALT));
    g.fillRect(bounds);

    // Draw horizontal grid lines at 25%, 50%, 75%, 100%
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER).withAlpha(0.5f));
    int margin = 2;
    int usableHeight = getHeight() - (margin * 2);

    for (int pct : {25, 50, 75, 100}) {
        int y = margin + usableHeight - (pct * usableHeight / 100);
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(bounds.getWidth()));
    }

    // Get clip data
    const auto* clip = ClipManager::getInstance().getClip(clipId_);
    if (!clip || clip->type != ClipType::MIDI) {
        return;
    }

    juce::Colour noteColour = clip->colour;
    int minBarWidth = 4;

    // Draw velocity bars for each note
    for (size_t i = 0; i < clip->midiNotes.size(); ++i) {
        const auto& note = clip->midiNotes[i];

        // Calculate x position - use preview position if available
        double noteStart = note.startBeat;
        auto previewIt = notePreviewPositions_.find(i);
        if (previewIt != notePreviewPositions_.end()) {
            noteStart = previewIt->second;
        }
        if (!relativeMode_) {
            noteStart += clipStartBeats_;
        }
        int x = beatToPixel(noteStart);
        int barWidth = juce::jmax(minBarWidth, static_cast<int>(note.lengthBeats * pixelsPerBeat_));

        // Skip if out of view
        if (x + barWidth < 0 || x > bounds.getWidth()) {
            continue;
        }

        // Use drag velocity if this is the note being dragged
        int velocity = note.velocity;
        if (isDragging_ && i == draggingNoteIndex_) {
            velocity = currentDragVelocity_;
        }

        // Calculate bar height from velocity
        int barHeight = velocity * usableHeight / 127;
        int barY = margin + usableHeight - barHeight;

        // Draw the bar
        auto barBounds = juce::Rectangle<int>(x, barY, barWidth - 1, barHeight);

        // Fill
        g.setColour(noteColour.withAlpha(0.8f));
        g.fillRect(barBounds);

        // Border
        g.setColour(noteColour.brighter(0.2f));
        g.drawRect(barBounds, 1);

        // Highlight if being dragged
        if (isDragging_ && i == draggingNoteIndex_) {
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.fillRect(barBounds);
        }
    }

    // Draw top border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(bounds.getWidth()));
}

void VelocityLaneComponent::mouseDown(const juce::MouseEvent& e) {
    size_t noteIndex = findNoteAtX(e.x);

    if (noteIndex != SIZE_MAX) {
        const auto* clip = ClipManager::getInstance().getClip(clipId_);
        if (clip && noteIndex < clip->midiNotes.size()) {
            draggingNoteIndex_ = noteIndex;
            dragStartVelocity_ = clip->midiNotes[noteIndex].velocity;
            currentDragVelocity_ = yToVelocity(e.y);
            isDragging_ = true;
            repaint();
        }
    }
}

void VelocityLaneComponent::mouseDrag(const juce::MouseEvent& e) {
    if (isDragging_ && draggingNoteIndex_ != SIZE_MAX) {
        int newVelocity = yToVelocity(e.y);
        if (newVelocity != currentDragVelocity_) {
            currentDragVelocity_ = newVelocity;
            repaint();
        }
    }
}

void VelocityLaneComponent::mouseUp(const juce::MouseEvent& e) {
    if (isDragging_ && draggingNoteIndex_ != SIZE_MAX) {
        int finalVelocity = yToVelocity(e.y);

        // Only commit if velocity actually changed
        if (finalVelocity != dragStartVelocity_ && onVelocityChanged) {
            onVelocityChanged(clipId_, draggingNoteIndex_, finalVelocity);
        }

        draggingNoteIndex_ = SIZE_MAX;
        isDragging_ = false;
        repaint();
    }
}

}  // namespace magda
