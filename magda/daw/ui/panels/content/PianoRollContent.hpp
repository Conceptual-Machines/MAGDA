#pragma once

#include "PanelContent.hpp"
#include "core/ClipManager.hpp"

namespace magda::daw::ui {

/**
 * @brief Piano roll editor for MIDI clips
 *
 * Displays MIDI notes in a piano roll grid layout:
 * - Keyboard on the left showing note names
 * - Note rectangles in the grid representing MIDI notes
 * - Time axis along the top
 */
class PianoRollContent : public PanelContent, public magda::ClipManagerListener {
  public:
    PianoRollContent();
    ~PianoRollContent() override;

    PanelContentType getContentType() const override {
        return PanelContentType::PianoRoll;
    }

    PanelContentInfo getContentInfo() const override {
        return {PanelContentType::PianoRoll, "Piano Roll", "MIDI note editor", "PianoRoll"};
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void onActivated() override;
    void onDeactivated() override;

    // ClipManagerListener
    void clipsChanged() override;
    void clipPropertyChanged(magda::ClipId clipId) override;
    void clipSelectionChanged(magda::ClipId clipId) override;

    // Set the clip to edit
    void setClip(magda::ClipId clipId);
    magda::ClipId getEditingClipId() const {
        return editingClipId_;
    }

  private:
    magda::ClipId editingClipId_ = magda::INVALID_CLIP_ID;

    // Layout constants
    static constexpr int KEYBOARD_WIDTH = 60;
    static constexpr int NOTE_HEIGHT = 12;
    static constexpr int HEADER_HEIGHT = 24;
    static constexpr int MIN_NOTE = 21;   // A0
    static constexpr int MAX_NOTE = 108;  // C8

    // Zoom
    double horizontalZoom_ = 50.0;  // pixels per beat

    // Components
    std::unique_ptr<juce::Viewport> viewport_;

    // Painting helpers
    void paintKeyboard(juce::Graphics& g, juce::Rectangle<int> area);
    void paintNoteGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void paintNotes(juce::Graphics& g, juce::Rectangle<int> area, const magda::ClipInfo& clip);
    void paintHeader(juce::Graphics& g, juce::Rectangle<int> area);

    bool isBlackKey(int noteNumber) const;
    juce::String getNoteName(int noteNumber) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollContent)
};

}  // namespace magda::daw::ui
