#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

#include "core/AutomationInfo.hpp"
#include "core/AutomationManager.hpp"
#include "core/AutomationTypes.hpp"
#include "core/TypeIds.hpp"

namespace magda {

// Forward declaration
class TrackContentPanel;

/**
 * @brief Left margin component for TrackContentPanel
 *
 * Paints Y-axis scale labels for automation lanes and any other
 * left-margin content. Gets layout info from parent TrackContentPanel.
 */
class TrackContentPaddingComponent : public juce::Component, public AutomationManagerListener {
  public:
    static constexpr int PADDING_WIDTH = 50;

    explicit TrackContentPaddingComponent(TrackContentPanel* parent);
    ~TrackContentPaddingComponent() override;

    void paint(juce::Graphics& g) override;

    // AutomationManagerListener
    void automationLanesChanged() override;
    void automationLanePropertyChanged(AutomationLaneId laneId) override;

    // Called by parent when layout changes
    void layoutChanged();

  private:
    TrackContentPanel* parent_;

    // Info about an automation lane's position for painting
    struct AutomationLaneLayout {
        AutomationLaneId laneId = INVALID_AUTOMATION_LANE_ID;
        int y = 0;       // Y position (below header)
        int height = 0;  // Content height (excluding header/resize)
        AutomationTargetType targetType = AutomationTargetType::DeviceParameter;
    };

    std::vector<AutomationLaneLayout> laneLayouts_;

    // Rebuild layout info from parent
    void rebuildLayoutInfo();

    // Paint scale labels for one automation lane
    void paintAutomationScale(juce::Graphics& g, const AutomationLaneLayout& layout);

    // Format value based on target type
    juce::String formatValue(double normalizedValue, AutomationTargetType targetType) const;

    // Convert normalized value to Y pixel within a lane
    int valueToPixel(double value, int laneY, int laneHeight) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackContentPaddingComponent)
};

}  // namespace magda
