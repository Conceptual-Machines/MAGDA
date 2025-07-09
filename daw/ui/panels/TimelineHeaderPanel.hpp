#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace magica {

class TimelineComponent;

class TimelineHeaderPanel : public juce::Component {
public:
    enum TimeDisplayMode {
        Time,      // MM:SS format
        BarsBeats  // Bars:Beats format
    };

    TimelineHeaderPanel();
    ~TimelineHeaderPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Layout alignment with main window panels
    void setLayoutSizes(int leftWidth, int rightWidth);

    // Time display mode
    void setTimeDisplayMode(TimeDisplayMode mode);
    TimeDisplayMode getTimeDisplayMode() const { return timeDisplayMode; }

    // Callback for when time display mode changes
    std::function<void(TimeDisplayMode)> onTimeDisplayModeChanged;
    
    // Timeline component access
    TimelineComponent* getTimelineComponent() const { return timeline.get(); }

private:
    std::unique_ptr<juce::ToggleButton> timeDisplayToggle;
    std::unique_ptr<TimelineComponent> timeline;
    
    TimeDisplayMode timeDisplayMode = Time;
    
    // Layout sizes to align with main window panels
    int leftPanelWidth = 250;
    int rightPanelWidth = 300;
    
    // Helper methods
    void updateTimeDisplayToggle();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineHeaderPanel)
};

} // namespace magica 