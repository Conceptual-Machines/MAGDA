#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace magica {

class BottomPanel : public juce::Component {
public:
    BottomPanel();
    ~BottomPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomPanel)
};

} // namespace magica 