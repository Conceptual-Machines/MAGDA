#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <memory>

#include "../components/common/SvgButton.hpp"
#include "content/PanelContent.hpp"
#include "state/PanelState.hpp"

namespace magica::daw::ui {

/**
 * @brief Tab bar component for TabbedPanel
 *
 * Displays horizontal row of icon buttons for switching between panel content.
 * Follows the FooterBar pattern for button styling and layout.
 * Sits at the bottom of the panel (footer position).
 */
class PanelTabBar : public juce::Component {
  public:
    static constexpr int MAX_TABS = 4;
    static constexpr int BUTTON_SIZE = 24;
    static constexpr int BUTTON_SPACING = 8;
    static constexpr int BAR_HEIGHT = 32;

    PanelTabBar();
    ~PanelTabBar() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * @brief Set the tabs to display
     */
    void setTabs(const std::vector<PanelContentType>& tabs);

    /**
     * @brief Set the active tab index
     */
    void setActiveTab(int index);

    /**
     * @brief Get the current active tab index
     */
    int getActiveTab() const {
        return activeTabIndex_;
    }

    /**
     * @brief Callback when a tab is clicked
     * Parameter is the tab index
     */
    std::function<void(int)> onTabClicked;

    /**
     * @brief Callback when a tab is right-clicked (for context menu)
     * Parameters are tab index and mouse position
     */
    std::function<void(int, juce::Point<int>)> onTabRightClicked;

  private:
    std::array<std::unique_ptr<SvgButton>, MAX_TABS> tabButtons_;
    std::vector<PanelContentType> currentTabs_;
    int activeTabIndex_ = 0;

    void setupButton(size_t index, PanelContentType type);
    void updateButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelTabBar)
};

}  // namespace magica::daw::ui
