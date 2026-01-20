#pragma once

#include <functional>

#include "TabbedPanel.hpp"

namespace magda {

/**
 * @brief Right sidebar panel with tabbed content
 *
 * Default tabs: Inspector
 */
class RightPanel : public daw::ui::TabbedPanel {
  public:
    RightPanel();
    ~RightPanel() override = default;

    // Legacy API for compatibility
    void setCollapsed(bool collapsed);

  protected:
    juce::Rectangle<int> getCollapseButtonBounds() override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightPanel)
};

}  // namespace magda
