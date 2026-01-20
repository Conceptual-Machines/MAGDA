#include "LeftPanel.hpp"

#include "state/PanelController.hpp"

namespace magda {

LeftPanel::LeftPanel() : TabbedPanel(daw::ui::PanelLocation::Left) {
    setName("Left Panel");
}

void LeftPanel::setCollapsed(bool collapsed) {
    daw::ui::PanelController::getInstance().setCollapsed(daw::ui::PanelLocation::Left, collapsed);
}

juce::Rectangle<int> LeftPanel::getCollapseButtonBounds() {
    if (isCollapsed()) {
        return juce::Rectangle<int>(2, getHeight() / 2 - 10, 20, 20);
    } else {
        return juce::Rectangle<int>(getWidth() - 24, 4, 20, 20);
    }
}

}  // namespace magda
