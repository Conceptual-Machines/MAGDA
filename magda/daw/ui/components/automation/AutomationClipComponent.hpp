#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

#include "core/AutomationInfo.hpp"
#include "core/AutomationManager.hpp"
#include "core/SelectionManager.hpp"

namespace magda {

class AutomationLaneComponent;

/**
 * @brief Automation clip for clip-based automation
 *
 * Similar to ClipComponent - supports Move, ResizeLeft, ResizeRight drag modes.
 * Contains a mini curve preview. Double-click opens in detail editor.
 */
class AutomationClipComponent : public juce::Component,
                                public AutomationManagerListener,
                                public SelectionManagerListener {
  public:
    AutomationClipComponent(AutomationClipId clipId, AutomationLaneComponent* parent);
    ~AutomationClipComponent() override;

    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool hitTest(int x, int y) override;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // AutomationManagerListener
    void automationLanesChanged() override {}
    void automationClipsChanged(AutomationLaneId laneId) override;

    // SelectionManagerListener
    void selectionTypeChanged(SelectionType newType) override;
    void automationClipSelectionChanged(const AutomationClipSelection& selection) override;

    // State
    AutomationClipId getClipId() const {
        return clipId_;
    }
    bool isSelected() const {
        return isSelected_;
    }
    void setSelected(bool selected);

    // Configuration
    void setPixelsPerSecond(double pps) {
        pixelsPerSecond_ = pps;
    }
    double getPixelsPerSecond() const {
        return pixelsPerSecond_;
    }

    // Callbacks
    std::function<void(AutomationClipId)> onClipSelected;
    std::function<void(AutomationClipId, double)> onClipMoved;  // clipId, newStartTime
    std::function<void(AutomationClipId, double, bool)>
        onClipResized;  // clipId, newLength, fromStart
    std::function<void(AutomationClipId)> onClipDoubleClicked;
    std::function<double(double)> snapTimeToGrid;

    // Resize edge detection
    static constexpr int RESIZE_EDGE_WIDTH = 6;

  private:
    AutomationClipId clipId_;
    AutomationLaneComponent* parentLane_;
    double pixelsPerSecond_ = 100.0;

    bool isSelected_ = false;
    bool isHovered_ = false;

    enum class DragMode { None, Move, ResizeLeft, ResizeRight };
    DragMode dragMode_ = DragMode::None;
    bool isDragging_ = false;

    juce::Point<int> dragStartPos_;
    double dragStartTime_ = 0.0;
    double dragStartLength_ = 0.0;
    double previewStartTime_ = 0.0;
    double previewLength_ = 0.0;

    // Helpers
    bool isOnLeftEdge(int x) const {
        return x < RESIZE_EDGE_WIDTH;
    }
    bool isOnRightEdge(int x) const {
        return x >= getWidth() - RESIZE_EDGE_WIDTH;
    }
    const AutomationClipInfo* getClipInfo() const;
    void paintMiniCurve(juce::Graphics& g, juce::Rectangle<int> bounds);
    void syncSelectionState();
};

}  // namespace magda
