#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace magica {

class TimelineFiller;

class LeftPanel : public juce::Component {
public:
    LeftPanel();
    ~LeftPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Timeline filler positioning
    void setTimelineFillerPosition(int y, int height);

private:
    std::unique_ptr<TimelineFiller> timelineFiller;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftPanel)
};

} // namespace magica 