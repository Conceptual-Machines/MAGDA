#include "TrackContentPaddingComponent.hpp"

#include <cmath>

#include "TrackContentPanel.hpp"

namespace magda {

TrackContentPaddingComponent::TrackContentPaddingComponent(TrackContentPanel* parent)
    : parent_(parent) {
    setName("TrackContentPaddingComponent");

    // Allow painting outside bounds so labels can extend into content area's LEFT_PADDING
    setPaintingIsUnclipped(true);

    AutomationManager::getInstance().addListener(this);
}

TrackContentPaddingComponent::~TrackContentPaddingComponent() {
    AutomationManager::getInstance().removeListener(this);
}

void TrackContentPaddingComponent::paint(juce::Graphics& g) {
    // Background
    g.fillAll(juce::Colour(0xFF1A1A1A));

    // Right edge separator
    g.setColour(juce::Colour(0xFF333333));
    g.drawVerticalLine(getWidth() - 1, 0.0f, static_cast<float>(getHeight()));

    // Paint scale labels for each automation lane
    for (const auto& layout : laneLayouts_) {
        paintAutomationScale(g, layout);
    }
}

void TrackContentPaddingComponent::automationLanesChanged() {
    rebuildLayoutInfo();
    repaint();
}

void TrackContentPaddingComponent::automationLanePropertyChanged(AutomationLaneId /*laneId*/) {
    rebuildLayoutInfo();
    repaint();
}

void TrackContentPaddingComponent::layoutChanged() {
    rebuildLayoutInfo();
    repaint();
}

void TrackContentPaddingComponent::rebuildLayoutInfo() {
    laneLayouts_.clear();

    if (!parent_)
        return;

    auto& automationManager = AutomationManager::getInstance();

    // Get visible track IDs from parent
    const auto& trackIds = parent_->getVisibleTrackIds();

    // Iterate through visible tracks and their automation lanes
    int numTracks = parent_->getNumTracks();
    for (int trackIndex = 0; trackIndex < numTracks; ++trackIndex) {
        // Get track Y position and height
        int trackY = parent_->getTrackYPosition(trackIndex);
        int trackHeight = parent_->getTrackHeight(trackIndex);

        // Get track ID
        if (trackIndex >= static_cast<int>(trackIds.size()))
            continue;

        TrackId trackId = trackIds[trackIndex];

        // Get visible automation lanes for this track
        auto laneIds = automationManager.getLanesForTrack(trackId);
        int automationY = trackY + trackHeight;  // Start below track content

        for (auto laneId : laneIds) {
            const auto* lane = automationManager.getLane(laneId);
            if (!lane || !lane->visible)
                continue;

            // Automation lane layout constants (must match AutomationLaneComponent)
            constexpr int HEADER_HEIGHT = 20;
            constexpr int RESIZE_HANDLE_HEIGHT = 5;

            int laneHeight = lane->height;
            int contentY = automationY + HEADER_HEIGHT;
            int contentHeight = laneHeight - HEADER_HEIGHT - RESIZE_HANDLE_HEIGHT;

            if (contentHeight > 0) {
                AutomationLaneLayout layout;
                layout.laneId = laneId;
                layout.y = contentY;
                layout.height = contentHeight;
                layout.targetType = lane->target.type;
                laneLayouts_.push_back(layout);
            }

            automationY += laneHeight;
        }
    }
}

void TrackContentPaddingComponent::paintAutomationScale(juce::Graphics& g,
                                                        const AutomationLaneLayout& layout) {
    if (layout.height <= 0)
        return;

    // Draw scale labels
    g.setColour(juce::Colour(0xFF888888));
    g.setFont(9.0f);

    // Draw labels at key positions: 100%, 75%, 50%, 25%, 0%
    const std::array<double, 5> values = {1.0, 0.75, 0.5, 0.25, 0.0};

    // Label width extends past component bounds into content area's LEFT_PADDING
    constexpr int LABEL_OVERHANG = 8;  // How far labels extend into content area

    for (double normalizedValue : values) {
        int y = valueToPixel(normalizedValue, layout.y, layout.height);
        juce::String label = formatValue(normalizedValue, layout.targetType);

        // Draw label - extends past right edge into content's LEFT_PADDING
        auto labelBounds = juce::Rectangle<int>(2, y - 5, getWidth() + LABEL_OVERHANG - 4, 10);

        // Constrain to lane area vertically
        if (labelBounds.getY() < layout.y) {
            labelBounds.setY(layout.y);
        }
        if (labelBounds.getBottom() > layout.y + layout.height) {
            labelBounds.setY(layout.y + layout.height - 10);
        }

        g.drawText(label, labelBounds, juce::Justification::centredRight);

        // Draw small tick mark extending into content area
        g.drawHorizontalLine(y, static_cast<float>(getWidth() - 4),
                             static_cast<float>(getWidth() + LABEL_OVERHANG));
    }

    // Draw separator line at bottom of lane area
    g.setColour(juce::Colour(0xFF333333));
    g.drawHorizontalLine(layout.y + layout.height, 0.0f, static_cast<float>(getWidth()));
}

int TrackContentPaddingComponent::valueToPixel(double value, int laneY, int laneHeight) const {
    return laneY + static_cast<int>((1.0 - value) * laneHeight);
}

juce::String TrackContentPaddingComponent::formatValue(double normalizedValue,
                                                       AutomationTargetType targetType) const {
    switch (targetType) {
        case AutomationTargetType::TrackVolume: {
            // Volume: 0.8 = 0dB, 0 = -inf
            if (normalizedValue <= 0.001) {
                return "-inf";
            }
            // Convert to dB: assuming 0.8 = 0dB (unity gain)
            double dB = 20.0 * std::log10(normalizedValue / 0.8);
            if (dB > 0) {
                return "+" + juce::String(static_cast<int>(std::round(dB)));
            }
            return juce::String(static_cast<int>(std::round(dB)));
        }

        case AutomationTargetType::TrackPan: {
            // Pan: 0 = full left, 0.5 = center, 1 = full right
            if (normalizedValue < 0.48) {
                int percent = static_cast<int>((0.5 - normalizedValue) * 200);
                return juce::String(percent) + "L";
            } else if (normalizedValue > 0.52) {
                int percent = static_cast<int>((normalizedValue - 0.5) * 200);
                return juce::String(percent) + "R";
            }
            return "C";
        }

        default:
            // Generic 0-100%
            return juce::String(static_cast<int>(normalizedValue * 100)) + "%";
    }
}

}  // namespace magda
