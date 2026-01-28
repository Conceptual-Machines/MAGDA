#pragma once

#include <memory>

#include "PanelContent.hpp"
#include "core/ClipManager.hpp"
#include "ui/components/timeline/TimeRuler.hpp"
#include "ui/components/waveform/WaveformGridComponent.hpp"

namespace magda::daw::ui {

/**
 * @brief Waveform editor for audio clips
 *
 * Container that manages:
 * - ScrollNotifyingViewport (scrolling)
 * - WaveformGridComponent (scrollable waveform content)
 * - TimeRuler (synchronized with scroll)
 * - ABS/REL mode toggle
 * - Zoom controls
 *
 * Architecture based on PianoRollContent pattern.
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

    // Mouse wheel for zoom
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;

    // ClipManagerListener
    void clipsChanged() override;
    void clipPropertyChanged(magda::ClipId clipId) override;
    void clipSelectionChanged(magda::ClipId clipId) override;

    // Set the clip to edit
    void setClip(magda::ClipId clipId);
    magda::ClipId getEditingClipId() const {
        return editingClipId_;
    }

    // Timeline mode
    void setRelativeTimeMode(bool relative);
    bool isRelativeTimeMode() const {
        return relativeTimeMode_;
    }

  private:
    magda::ClipId editingClipId_ = magda::INVALID_CLIP_ID;

    // Timeline mode
    bool relativeTimeMode_ = false;  // false = absolute (timeline), true = relative (clip)

    // Zoom
    double horizontalZoom_ = 100.0;  // pixels per second
    static constexpr double MIN_ZOOM = 20.0;
    static constexpr double MAX_ZOOM = 500.0;

    // Layout constants
    static constexpr int TIME_RULER_HEIGHT = 30;
    static constexpr int TOOLBAR_HEIGHT = 30;
    static constexpr int GRID_LEFT_PADDING = 10;

    // Components (created in constructor)
    class ScrollNotifyingViewport;  // Forward declaration
    std::unique_ptr<ScrollNotifyingViewport> viewport_;
    std::unique_ptr<WaveformGridComponent> gridComponent_;
    std::unique_ptr<magda::TimeRuler> timeRuler_;
    std::unique_ptr<juce::TextButton> timeModeButton_;

    // Look and feel
    class ButtonLookAndFeel;
    std::unique_ptr<ButtonLookAndFeel> buttonLookAndFeel_;

    // Update grid size when clip or zoom changes
    void updateGridSize();

    // Scroll to show clip start
    void scrollToClipStart();

    // Anchor-point zoom
    void performAnchorPointZoom(double zoomFactor, int anchorX);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformEditorContent)
};

}  // namespace magda::daw::ui
