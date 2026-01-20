#include "PanelController.hpp"

#include <algorithm>

namespace magda::daw::ui {

PanelController& PanelController::getInstance() {
    static PanelController instance;
    return instance;
}

PanelController::PanelController() : state_(getDefaultPanelStates()) {}

void PanelController::dispatch(const PanelEvent& event) {
    std::visit(
        [this](auto&& e) {
            using T = std::decay_t<decltype(e)>;

            if constexpr (std::is_same_v<T, SetActiveTabEvent>) {
                handleSetActiveTab(e);
            } else if constexpr (std::is_same_v<T, SetActiveTabByTypeEvent>) {
                handleSetActiveTabByType(e);
            } else if constexpr (std::is_same_v<T, TogglePanelCollapsedEvent>) {
                handleToggleCollapsed(e);
            } else if constexpr (std::is_same_v<T, SetPanelCollapsedEvent>) {
                handleSetCollapsed(e);
            } else if constexpr (std::is_same_v<T, SetPanelSizeEvent>) {
                handleSetSize(e);
            } else if constexpr (std::is_same_v<T, AddTabEvent>) {
                handleAddTab(e);
            } else if constexpr (std::is_same_v<T, RemoveTabEvent>) {
                handleRemoveTab(e);
            } else if constexpr (std::is_same_v<T, ReorderTabsEvent>) {
                handleReorderTabs(e);
            } else if constexpr (std::is_same_v<T, ResetPanelsToDefaultEvent>) {
                handleResetToDefaults(e);
            }
        },
        event);
}

void PanelController::setActiveTab(PanelLocation panel, int tabIndex) {
    dispatch(SetActiveTabEvent{panel, tabIndex});
}

void PanelController::setActiveTabByType(PanelLocation panel, PanelContentType contentType) {
    dispatch(SetActiveTabByTypeEvent{panel, contentType});
}

void PanelController::toggleCollapsed(PanelLocation panel) {
    dispatch(TogglePanelCollapsedEvent{panel});
}

void PanelController::setCollapsed(PanelLocation panel, bool collapsed) {
    dispatch(SetPanelCollapsedEvent{panel, collapsed});
}

void PanelController::resetToDefaults() {
    dispatch(ResetPanelsToDefaultEvent{});
}

void PanelController::addListener(PanelStateListener* listener) {
    if (listener && std::find(listeners_.begin(), listeners_.end(), listener) == listeners_.end()) {
        listeners_.push_back(listener);
    }
}

void PanelController::removeListener(PanelStateListener* listener) {
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
}

void PanelController::notifyPanelChanged(PanelLocation location) {
    const auto& panelState = state_.getPanel(location);
    for (auto* listener : listeners_) {
        listener->panelStateChanged(location, panelState);
    }
}

void PanelController::notifyActiveTabChanged(PanelLocation location) {
    const auto& panelState = state_.getPanel(location);
    for (auto* listener : listeners_) {
        listener->activeTabChanged(location, panelState.activeTabIndex,
                                   panelState.getActiveContentType());
    }
}

void PanelController::notifyCollapsedChanged(PanelLocation location) {
    const auto& panelState = state_.getPanel(location);
    for (auto* listener : listeners_) {
        listener->panelCollapsedChanged(location, panelState.collapsed);
    }
}

void PanelController::handleSetActiveTab(const SetActiveTabEvent& event) {
    auto& panel = state_.getPanel(event.panel);
    if (event.tabIndex >= 0 && event.tabIndex < static_cast<int>(panel.tabs.size())) {
        if (panel.activeTabIndex != event.tabIndex) {
            panel.activeTabIndex = event.tabIndex;
            notifyActiveTabChanged(event.panel);
            notifyPanelChanged(event.panel);
        }
    }
}

void PanelController::handleSetActiveTabByType(const SetActiveTabByTypeEvent& event) {
    auto& panel = state_.getPanel(event.panel);
    int index = panel.getTabIndex(event.contentType);
    if (index >= 0 && panel.activeTabIndex != index) {
        panel.activeTabIndex = index;
        notifyActiveTabChanged(event.panel);
        notifyPanelChanged(event.panel);
    }
}

void PanelController::handleToggleCollapsed(const TogglePanelCollapsedEvent& event) {
    auto& panel = state_.getPanel(event.panel);
    panel.collapsed = !panel.collapsed;
    notifyCollapsedChanged(event.panel);
    notifyPanelChanged(event.panel);
}

void PanelController::handleSetCollapsed(const SetPanelCollapsedEvent& event) {
    auto& panel = state_.getPanel(event.panel);
    if (panel.collapsed != event.collapsed) {
        panel.collapsed = event.collapsed;
        notifyCollapsedChanged(event.panel);
        notifyPanelChanged(event.panel);
    }
}

void PanelController::handleSetSize(const SetPanelSizeEvent& event) {
    auto& panel = state_.getPanel(event.panel);
    if (panel.size != event.size) {
        panel.size = event.size;
        notifyPanelChanged(event.panel);
    }
}

void PanelController::handleAddTab(const AddTabEvent& event) {
    auto& panel = state_.getPanel(event.panel);

    // Max 4 tabs
    if (panel.tabs.size() >= 4)
        return;

    // Don't add duplicate
    if (panel.hasContentType(event.contentType))
        return;

    if (event.insertIndex >= 0 && event.insertIndex < static_cast<int>(panel.tabs.size())) {
        panel.tabs.insert(panel.tabs.begin() + event.insertIndex, event.contentType);
    } else {
        panel.tabs.push_back(event.contentType);
    }

    notifyPanelChanged(event.panel);
}

void PanelController::handleRemoveTab(const RemoveTabEvent& event) {
    auto& panel = state_.getPanel(event.panel);

    // Must have at least 1 tab
    if (panel.tabs.size() <= 1)
        return;

    if (event.tabIndex >= 0 && event.tabIndex < static_cast<int>(panel.tabs.size())) {
        panel.tabs.erase(panel.tabs.begin() + event.tabIndex);

        // Adjust active tab if needed
        if (panel.activeTabIndex >= static_cast<int>(panel.tabs.size())) {
            panel.activeTabIndex = static_cast<int>(panel.tabs.size()) - 1;
        }

        notifyPanelChanged(event.panel);
    }
}

void PanelController::handleReorderTabs(const ReorderTabsEvent& event) {
    auto& panel = state_.getPanel(event.panel);

    // Validate: must have same content types, just reordered
    if (event.newOrder.size() != panel.tabs.size())
        return;

    for (const auto& type : panel.tabs) {
        bool found = false;
        for (const auto& newType : event.newOrder) {
            if (type == newType) {
                found = true;
                break;
            }
        }
        if (!found)
            return;
    }

    // Remember current active type
    auto activeType = panel.getActiveContentType();

    // Apply new order
    panel.tabs = event.newOrder;

    // Restore active tab by type
    panel.activeTabIndex = panel.getTabIndex(activeType);
    if (panel.activeTabIndex < 0)
        panel.activeTabIndex = 0;

    notifyPanelChanged(event.panel);
}

void PanelController::handleResetToDefaults(const ResetPanelsToDefaultEvent& /*event*/) {
    state_ = getDefaultPanelStates();
    notifyPanelChanged(PanelLocation::Left);
    notifyPanelChanged(PanelLocation::Right);
    notifyPanelChanged(PanelLocation::Bottom);
}

}  // namespace magda::daw::ui
