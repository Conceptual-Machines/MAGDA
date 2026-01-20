#include "PianoRollContent.hpp"

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"

namespace magda::daw::ui {

PianoRollContent::PianoRollContent() {
    setName("PianoRoll");

    // Create viewport for scrolling
    viewport_ = std::make_unique<juce::Viewport>();
    viewport_->setScrollBarsShown(true, true);
    addAndMakeVisible(viewport_.get());

    // Register as ClipManager listener
    magda::ClipManager::getInstance().addListener(this);

    // Check if there's already a selected MIDI clip
    magda::ClipId selectedClip = magda::ClipManager::getInstance().getSelectedClip();
    if (selectedClip != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(selectedClip);
        if (clip && clip->type == magda::ClipType::MIDI) {
            editingClipId_ = selectedClip;
        }
    }
}

PianoRollContent::~PianoRollContent() {
    magda::ClipManager::getInstance().removeListener(this);
}

void PianoRollContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());

    auto bounds = getLocalBounds();

    // Header area (time axis)
    auto headerArea = bounds.removeFromTop(HEADER_HEIGHT);
    paintHeader(g, headerArea);

    // Keyboard area on the left
    auto keyboardArea = bounds.removeFromLeft(KEYBOARD_WIDTH);
    paintKeyboard(g, keyboardArea);

    // Note grid in the remaining space
    paintNoteGrid(g, bounds);

    // Draw notes if we have a clip
    if (editingClipId_ != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(editingClipId_);
        if (clip && clip->type == magda::ClipType::MIDI) {
            paintNotes(g, bounds, *clip);
        }
    }
}

void PianoRollContent::paintHeader(juce::Graphics& g, juce::Rectangle<int> area) {
    g.setColour(DarkTheme::getColour(DarkTheme::SURFACE));
    g.fillRect(area);

    // Draw beat markers
    g.setColour(DarkTheme::getSecondaryTextColour());
    g.setFont(FontManager::getInstance().getUIFont(9.0f));

    const auto* clip = editingClipId_ != magda::INVALID_CLIP_ID
                           ? magda::ClipManager::getInstance().getClip(editingClipId_)
                           : nullptr;

    double lengthBeats = clip ? clip->length * 2.0 : 16.0;  // Approximate beats

    for (int beat = 0; beat <= static_cast<int>(lengthBeats); beat++) {
        int x = KEYBOARD_WIDTH + static_cast<int>(beat * horizontalZoom_);
        if (x < area.getRight()) {
            g.drawVerticalLine(x, area.getY(), area.getBottom());
            g.drawText(juce::String(beat + 1), x + 2, area.getY(), 20, area.getHeight(),
                       juce::Justification::centredLeft, false);
        }
    }

    // Border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(area);
}

void PianoRollContent::paintKeyboard(juce::Graphics& g, juce::Rectangle<int> area) {
    int numNotes = MAX_NOTE - MIN_NOTE + 1;
    int totalHeight = numNotes * NOTE_HEIGHT;

    for (int note = MIN_NOTE; note <= MAX_NOTE; note++) {
        int y = area.getY() + (MAX_NOTE - note) * NOTE_HEIGHT;

        if (y + NOTE_HEIGHT < area.getY() || y > area.getBottom()) {
            continue;
        }

        auto keyArea =
            juce::Rectangle<int>(area.getX(), y, area.getWidth(), NOTE_HEIGHT).reduced(0, 1);

        if (isBlackKey(note)) {
            g.setColour(DarkTheme::getColour(DarkTheme::BACKGROUND));
        } else {
            g.setColour(DarkTheme::getColour(DarkTheme::SURFACE).brighter(0.2f));
        }
        g.fillRect(keyArea);

        // Draw note name for C notes
        if (note % 12 == 0) {
            g.setColour(DarkTheme::getTextColour());
            g.setFont(FontManager::getInstance().getUIFont(9.0f));
            g.drawText(getNoteName(note), keyArea.reduced(4, 0), juce::Justification::centredLeft,
                       false);
        }

        // Key border
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawRect(keyArea);
    }
}

void PianoRollContent::paintNoteGrid(juce::Graphics& g, juce::Rectangle<int> area) {
    // Background
    g.setColour(DarkTheme::getColour(DarkTheme::TRACK_BACKGROUND));
    g.fillRect(area);

    // Horizontal lines for each note
    for (int note = MIN_NOTE; note <= MAX_NOTE; note++) {
        int y = area.getY() + (MAX_NOTE - note) * NOTE_HEIGHT;

        if (y + NOTE_HEIGHT < area.getY() || y > area.getBottom()) {
            continue;
        }

        // Alternate row colors
        if (isBlackKey(note)) {
            g.setColour(DarkTheme::getColour(DarkTheme::BACKGROUND).withAlpha(0.3f));
            g.fillRect(area.getX(), y, area.getWidth(), NOTE_HEIGHT);
        }

        // Grid line
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER).withAlpha(0.3f));
        g.drawHorizontalLine(y, area.getX(), area.getRight());
    }

    // Vertical lines for beats
    const auto* clip = editingClipId_ != magda::INVALID_CLIP_ID
                           ? magda::ClipManager::getInstance().getClip(editingClipId_)
                           : nullptr;

    double lengthBeats = clip ? clip->length * 2.0 : 16.0;

    for (int beat = 0; beat <= static_cast<int>(lengthBeats); beat++) {
        int x = static_cast<int>(beat * horizontalZoom_);
        if (x < area.getWidth()) {
            bool isBar = (beat % 4 == 0);
            g.setColour(DarkTheme::getColour(DarkTheme::BORDER).withAlpha(isBar ? 0.6f : 0.3f));
            g.drawVerticalLine(area.getX() + x, area.getY(), area.getBottom());
        }
    }
}

void PianoRollContent::paintNotes(juce::Graphics& g, juce::Rectangle<int> area,
                                  const magda::ClipInfo& clip) {
    g.setColour(clip.colour);

    for (const auto& note : clip.midiNotes) {
        // Calculate note position
        int y = area.getY() + (MAX_NOTE - note.noteNumber) * NOTE_HEIGHT;
        int x = area.getX() + static_cast<int>(note.startBeat * horizontalZoom_);
        int width = juce::jmax(4, static_cast<int>(note.lengthBeats * horizontalZoom_));

        if (y + NOTE_HEIGHT < area.getY() || y > area.getBottom()) {
            continue;
        }

        auto noteRect = juce::Rectangle<int>(x, y + 1, width, NOTE_HEIGHT - 2);

        // Fill
        g.setColour(clip.colour);
        g.fillRoundedRectangle(noteRect.toFloat(), 2.0f);

        // Border
        g.setColour(clip.colour.brighter(0.3f));
        g.drawRoundedRectangle(noteRect.toFloat(), 2.0f, 1.0f);

        // Velocity indicator (height variation)
        float velocityRatio = note.velocity / 127.0f;
        int velocityHeight = static_cast<int>((NOTE_HEIGHT - 4) * velocityRatio);
        g.setColour(clip.colour.brighter(0.5f));
        g.fillRect(x + 2, y + NOTE_HEIGHT - velocityHeight - 1, 2, velocityHeight);
    }
}

void PianoRollContent::resized() {
    auto bounds = getLocalBounds();

    // Viewport fills everything
    viewport_->setBounds(bounds);
}

void PianoRollContent::onActivated() {
    // Check for selected MIDI clip
    magda::ClipId selectedClip = magda::ClipManager::getInstance().getSelectedClip();
    if (selectedClip != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(selectedClip);
        if (clip && clip->type == magda::ClipType::MIDI) {
            editingClipId_ = selectedClip;
        }
    }
    repaint();
}

void PianoRollContent::onDeactivated() {
    // Nothing to do
}

// ============================================================================
// ClipManagerListener
// ============================================================================

void PianoRollContent::clipsChanged() {
    // Check if our clip was deleted
    if (editingClipId_ != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(editingClipId_);
        if (!clip) {
            editingClipId_ = magda::INVALID_CLIP_ID;
        }
    }
    repaint();
}

void PianoRollContent::clipPropertyChanged(magda::ClipId clipId) {
    if (clipId == editingClipId_) {
        repaint();
    }
}

void PianoRollContent::clipSelectionChanged(magda::ClipId clipId) {
    // Auto-switch to the selected clip if it's a MIDI clip
    if (clipId != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(clipId);
        if (clip && clip->type == magda::ClipType::MIDI) {
            editingClipId_ = clipId;
            repaint();
        }
    }
}

// ============================================================================
// Public Methods
// ============================================================================

void PianoRollContent::setClip(magda::ClipId clipId) {
    if (editingClipId_ != clipId) {
        editingClipId_ = clipId;
        repaint();
    }
}

// ============================================================================
// Helpers
// ============================================================================

bool PianoRollContent::isBlackKey(int noteNumber) const {
    int note = noteNumber % 12;
    return note == 1 || note == 3 || note == 6 || note == 8 || note == 10;
}

juce::String PianoRollContent::getNoteName(int noteNumber) const {
    static const char* noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                      "F#", "G",  "G#", "A",  "A#", "B"};
    int octave = (noteNumber / 12) - 1;
    int note = noteNumber % 12;
    return juce::String(noteNames[note]) + juce::String(octave);
}

}  // namespace magda::daw::ui
