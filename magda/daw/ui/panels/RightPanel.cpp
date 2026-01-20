#include "RightPanel.hpp"

#include "state/PanelController.hpp"

namespace magda {

RightPanel::RightPanel() : TabbedPanel(daw::ui::PanelLocation::Right) {
    setName("Right Panel");
}

void RightPanel::setCollapsed(bool collapsed) {
    daw::ui::PanelController::getInstance().setCollapsed(daw::ui::PanelLocation::Right, collapsed);
}

juce::Rectangle<int> RightPanel::getCollapseButtonBounds() {
    if (isCollapsed()) {
        return juce::Rectangle<int>(2, getHeight() / 2 - 10, 20, 20);
    } else {
        return juce::Rectangle<int>(4, 4, 20, 20);
    }
}

}  // namespace magda
