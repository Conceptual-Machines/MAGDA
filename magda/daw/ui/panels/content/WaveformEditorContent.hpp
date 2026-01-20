#pragma once

#include "PanelContent.hpp"
#include "core/ClipManager.hpp"

namespace magda::daw::ui {

/**
 * @brief Waveform editor for audio clips
 *
 * Displays audio waveform for editing:
 * - Waveform visualization
 * - Time axis along the top
 * - Trim handles for adjusting clip boundaries
 */
class WaveformEditorContent : public PanelContent, public magda::ClipManagerListener {
  public:
    WaveformEditorContent();
    ~WaveformEditorContent() override;

    PanelContentType getContentType() const override {
        return PanelContentType::WaveformEditor;
    }

    PanelContentInfo getContentInfo() const override {
        return {PanelContentType::WaveformEditor, "Waveform", "Audio waveform editor", "Waveform"};
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
    static constexpr int HEADER_HEIGHT = 24;
    static constexpr int SIDE_MARGIN = 20;

    // Zoom
    double horizontalZoom_ = 100.0;  // pixels per second

    // Painting helpers
    void paintHeader(juce::Graphics& g, juce::Rectangle<int> area);
    void paintWaveform(juce::Graphics& g, juce::Rectangle<int> area, const magda::ClipInfo& clip);
    void paintNoClipMessage(juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformEditorContent)
};

}  // namespace magda::daw::ui
