#include "BottomPanel.hpp"

#include "PanelTabBar.hpp"
#include "state/PanelController.hpp"

namespace magica {

BottomPanel::BottomPanel() : TabbedPanel(daw::ui::PanelLocation::Bottom) {
    setName("Bottom Panel");
}

void BottomPanel::setCollapsed(bool collapsed) {
    daw::ui::PanelController::getInstance().setCollapsed(daw::ui::PanelLocation::Bottom, collapsed);
}

juce::Rectangle<int> BottomPanel::getCollapseButtonBounds() {
    if (isCollapsed()) {
        return juce::Rectangle<int>(getWidth() / 2 - 10, 2, 20, 20);
    } else {
        return juce::Rectangle<int>(getWidth() - 24, 4, 20, 20);
    }
}

juce::Rectangle<int> BottomPanel::getTabBarBounds() {
    // Tab bar on the left side for bottom panel
    auto bounds = getLocalBounds();
    return bounds.removeFromLeft(200).withTrimmedTop(1);
}

juce::Rectangle<int> BottomPanel::getContentBounds() {
    auto bounds = getLocalBounds();
    // Content to the right of tab bar
    return bounds.withTrimmedLeft(200).withTrimmedTop(1);
}

}  // namespace magica
