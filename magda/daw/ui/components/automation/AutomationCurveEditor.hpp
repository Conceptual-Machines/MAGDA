#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

#include "AutomationPointComponent.hpp"
#include "BezierHandleComponent.hpp"
#include "core/AutomationInfo.hpp"
#include "core/AutomationManager.hpp"
#include "core/AutomationTypes.hpp"
#include "core/SelectionManager.hpp"

namespace magda {

/**
 * @brief Curve editing surface for automation data
 *
 * Renders automation curves (linear, bezier, step) and manages
 * AutomationPointComponents. Supports drawing tools: Select, Pencil, Line.
 * Double-click to add point, Delete to remove.
 */
class AutomationCurveEditor : public juce::Component,
                              public AutomationManagerListener,
                              public SelectionManagerListener {
  public:
    AutomationCurveEditor(AutomationLaneId laneId);
    ~AutomationCurveEditor() override;

    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // AutomationManagerListener
    void automationLanesChanged() override;
    void automationLanePropertyChanged(AutomationLaneId laneId) override;
    void automationPointsChanged(AutomationLaneId laneId) override;

    // SelectionManagerListener
    void selectionTypeChanged(SelectionType newType) override;
    void automationPointSelectionChanged(const AutomationPointSelection& selection) override;

    // Configuration
    void setLaneId(AutomationLaneId laneId);
    AutomationLaneId getLaneId() const {
        return laneId_;
    }

    void setDrawMode(AutomationDrawMode mode) {
        drawMode_ = mode;
    }
    AutomationDrawMode getDrawMode() const {
        return drawMode_;
    }

    // Coordinate conversion
    void setPixelsPerSecond(double pps) {
        pixelsPerSecond_ = pps;
    }
    double getPixelsPerSecond() const {
        return pixelsPerSecond_;
    }

    double getPixelsPerValue() const {
        return getHeight() > 0 ? getHeight() : 100.0;
    }

    double pixelToTime(int x) const;
    int timeToPixel(double time) const;
    double pixelToValue(int y) const;
    int valueToPixel(double value) const;

    // Snapping
    std::function<double(double)> snapTimeToGrid;

    // Clip mode (for clip-based automation)
    void setClipId(AutomationClipId clipId) {
        clipId_ = clipId;
    }
    AutomationClipId getClipId() const {
        return clipId_;
    }
    void setClipOffset(double offset) {
        clipOffset_ = offset;
    }

  private:
    AutomationLaneId laneId_;
    AutomationClipId clipId_ = INVALID_AUTOMATION_CLIP_ID;
    double clipOffset_ = 0.0;

    AutomationDrawMode drawMode_ = AutomationDrawMode::Select;
    double pixelsPerSecond_ = 100.0;

    std::vector<std::unique_ptr<AutomationPointComponent>> pointComponents_;
    std::vector<std::unique_ptr<BezierHandleComponent>> handleComponents_;

    // Drawing state
    bool isDrawing_ = false;
    std::vector<juce::Point<int>> drawingPath_;
    juce::Point<int> lineStartPoint_;

    // Rebuild
    void rebuildPointComponents();
    void updatePointPositions();
    void syncSelectionState();

    // Drawing
    void paintCurve(juce::Graphics& g);
    void paintGrid(juce::Graphics& g);
    void paintDrawingPreview(juce::Graphics& g);

    // Point access
    const std::vector<AutomationPoint>& getPoints() const;
    void addPointAt(double time, double value, AutomationCurveType curveType);
    void deleteSelectedPoints();

    // Pencil drawing
    void createPointsFromDrawingPath();
};

}  // namespace magda
