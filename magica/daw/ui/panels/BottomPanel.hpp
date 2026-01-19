#pragma once

#include <functional>

#include "TabbedPanel.hpp"

namespace magica {

/**
 * @brief Bottom panel with tabbed content
 *
 * Default tabs: AI Chat Console, Scripting Console
 */
class BottomPanel : public daw::ui::TabbedPanel {
  public:
    BottomPanel();
    ~BottomPanel() override = default;

    // Legacy API for compatibility
    void setCollapsed(bool collapsed);

  protected:
    juce::Rectangle<int> getCollapseButtonBounds() override;
    juce::Rectangle<int> getTabBarBounds() override;
    juce::Rectangle<int> getContentBounds() override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomPanel)
};

}  // namespace magica
