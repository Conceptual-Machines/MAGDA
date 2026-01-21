#include "RightPanel.hpp"

#include "state/PanelController.hpp"

namespace magda {

RightPanel::RightPanel() : TabbedPanel(daw::ui::PanelLocation::Left) {
    setName("Right Panel");
}

void RightPanel::setCollapsed(bool collapsed) {
    daw::ui::PanelController::getInstance().setCollapsed(daw::ui::PanelLocation::Left, collapsed);
}

juce::Rectangle<int> RightPanel::getCollapseButtonBounds() {
    if (isCollapsed()) {
        return juce::Rectangle<int>(2, getHeight() / 2 - 10, 20, 20);
    } else {
        return juce::Rectangle<int>(getWidth() - 24, 4, 20, 20);
    }
}

}  // namespace magda
