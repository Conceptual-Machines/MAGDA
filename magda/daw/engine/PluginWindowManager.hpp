#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <tracktion_engine/tracktion_engine.h>

#include <unordered_map>

#include "../core/TypeIds.hpp"

namespace magda {

namespace te = tracktion;

/**
 * @brief Manages plugin editor window lifecycle safely
 *
 * Key responsibilities:
 * - Owns window state tracking independently from AudioBridge
 * - Handles window close events safely (from timer, not from window's own event handler)
 * - Must be destroyed BEFORE AudioBridge in shutdown sequence
 *
 * The problem this solves:
 * When a user clicks the X button on a plugin window, the window's closeButtonPressed()
 * is called from within its own event handler. If we delete the window there (via
 * closeWindowExplicitly()), we get a malloc error because we're deleting an object
 * during its own callback.
 *
 * Solution:
 * - Window's closeButtonPressed() just calls setVisible(false)
 * - This timer detects hidden windows and closes them from OUTSIDE the window
 * - On shutdown, closeAllWindows() is called BEFORE AudioBridge is destroyed
 */
class PluginWindowManager : public juce::Timer {
  public:
    PluginWindowManager(te::Engine& engine, te::Edit& edit);
    ~PluginWindowManager() override;

    // =========================================================================
    // Window Control
    // =========================================================================

    /**
     * @brief Show the plugin's native editor window
     * @param deviceId MAGDA device ID
     * @param plugin The plugin to show
     */
    void showPluginWindow(DeviceId deviceId, te::Plugin::Ptr plugin);

    /**
     * @brief Hide/close the plugin's native editor window
     * @param deviceId MAGDA device ID
     * @param plugin The plugin to hide
     */
    void hidePluginWindow(DeviceId deviceId, te::Plugin::Ptr plugin);

    /**
     * @brief Toggle the plugin's window (open if closed, close if open)
     * @param deviceId MAGDA device ID
     * @param plugin The plugin to toggle
     * @return true if the window is now open, false if now closed
     */
    bool togglePluginWindow(DeviceId deviceId, te::Plugin::Ptr plugin);

    /**
     * @brief Check if a plugin window is currently open
     * @param deviceId MAGDA device ID
     * @param plugin The plugin to check
     * @return true if the plugin window is visible
     */
    bool isPluginWindowOpen(DeviceId deviceId, te::Plugin::Ptr plugin) const;

    // =========================================================================
    // Bulk Operations
    // =========================================================================

    /**
     * @brief Close all open plugin windows
     * Call this during shutdown BEFORE destroying AudioBridge
     */
    void closeAllWindows();

    /**
     * @brief Close all windows for a specific device (when device is removed)
     * @param deviceId MAGDA device ID being removed
     */
    void closeWindowsForDevice(DeviceId deviceId);

    // =========================================================================
    // Callbacks
    // =========================================================================

    /**
     * @brief Callback when window state changes (for UI updates)
     * Parameters: deviceId, isOpen
     */
    std::function<void(DeviceId, bool)> onWindowStateChanged;

  private:
    void timerCallback() override;

    te::Engine& engine_;
    te::Edit& edit_;

    // Track which windows we've opened and their last known state
    struct WindowInfo {
        te::Plugin::Ptr plugin;
        bool wasOpen = false;
    };
    std::unordered_map<DeviceId, WindowInfo> trackedWindows_;
    mutable juce::CriticalSection windowLock_;

    // Shutdown flag to prevent timer operations during cleanup
    std::atomic<bool> isShuttingDown_{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindowManager)
};

}  // namespace magda
