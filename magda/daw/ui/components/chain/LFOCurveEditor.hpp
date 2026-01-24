#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

#include "core/ModInfo.hpp"
#include "ui/components/common/curve/CurveEditorBase.hpp"

namespace magda {

/**
 * @brief Curve editor for LFO waveform editing
 *
 * Extends CurveEditorBase with LFO-specific functionality:
 * - Phase-based X coordinate (0 to 1)
 * - Seamless looping (last point connects to first)
 * - Integration with ModInfo for waveform storage
 * - Animated phase indicator showing current LFO position
 *
 * Used in the modulator editor panel for custom LFO shapes.
 */
class LFOCurveEditor : public CurveEditorBase, private juce::Timer {
  public:
    LFOCurveEditor();
    ~LFOCurveEditor() override;

    // Set the mod info to edit
    void setModInfo(ModInfo* mod);
    ModInfo* getModInfo() const {
        return modInfo_;
    }

    // CurveEditorBase coordinate interface
    double getPixelsPerX() const override;
    double pixelToX(int px) const override;
    int xToPixel(double x) const override;

    // LFO loops seamlessly
    bool shouldLoop() const override {
        return true;
    }

    // CurveEditorBase data access
    const std::vector<CurvePoint>& getPoints() const override;

    // Callback when waveform changes
    std::function<void()> onWaveformChanged;

  protected:
    // CurveEditorBase data mutation callbacks
    void onPointAdded(double x, double y, CurveType curveType) override;
    void onPointMoved(uint32_t pointId, double newX, double newY) override;
    void onPointDeleted(uint32_t pointId) override;
    void onPointSelected(uint32_t pointId) override;
    void onTensionChanged(uint32_t pointId, double tension) override;
    void onHandlesChanged(uint32_t pointId, const CurveHandleData& inHandle,
                          const CurveHandleData& outHandle) override;

    void paintGrid(juce::Graphics& g) override;

    // Paint override to add phase indicator
    void paint(juce::Graphics& g) override;

  private:
    void timerCallback() override;
    ModInfo* modInfo_ = nullptr;

    // Local curve points for custom waveform
    mutable std::vector<CurvePoint> points_;
    uint32_t nextPointId_ = 1;

    // Selected point (local selection, not using SelectionManager)
    uint32_t selectedPointId_ = INVALID_CURVE_POINT_ID;

    void notifyWaveformChanged();
};

}  // namespace magda
