#pragma once

#include <functional>
#include <vector>

namespace magda::daw::ui {

/**
 * @brief Singleton for runtime-adjustable debug settings
 */
class DebugSettings {
  public:
    static DebugSettings& getInstance() {
        static DebugSettings instance;
        return instance;
    }

    // Bottom panel height
    int getBottomPanelHeight() const {
        return bottomPanelHeight_;
    }
    void setBottomPanelHeight(int height) {
        bottomPanelHeight_ = height;
        notifyListeners();
    }

    // Device slot width
    int getDeviceSlotWidth() const {
        return deviceSlotWidth_;
    }
    void setDeviceSlotWidth(int width) {
        deviceSlotWidth_ = width;
        notifyListeners();
    }

    // Listener for settings changes
    using Listener = std::function<void()>;
    void addListener(Listener listener) {
        listeners_.push_back(listener);
    }

    void notifyListeners() {
        for (auto& listener : listeners_) {
            listener();
        }
    }

  private:
    DebugSettings() = default;

    int bottomPanelHeight_ = 315;
    int deviceSlotWidth_ = 235;

    std::vector<Listener> listeners_;
};

}  // namespace magda::daw::ui
