#pragma once

#include <tracktion_engine/tracktion_engine.h>

namespace tracktion {
inline namespace engine {
class Plugin;
struct PluginWindowState;
}  // namespace engine
}  // namespace tracktion

namespace magda {

/**
 * @brief Custom UIBehaviour implementation for MAGDA
 *
 * Provides plugin window creation for external plugins.
 * This is required for Tracktion Engine to display native plugin UIs.
 */
class MagdaUIBehaviour : public tracktion::UIBehaviour {
  public:
    MagdaUIBehaviour() = default;
    ~MagdaUIBehaviour() override = default;

    /**
     * @brief Create a plugin window for the given plugin state
     * @param state The PluginWindowState containing the plugin to display
     * @return A unique_ptr to the created window component, or nullptr if failed
     */
    std::unique_ptr<juce::Component> createPluginWindow(
        tracktion::PluginWindowState& state) override;
};

/**
 * @brief Window component that displays a plugin's native editor UI
 *
 * This is a DocumentWindow subclass that wraps the plugin's AudioProcessorEditor
 * and manages its lifecycle.
 */
class PluginEditorWindow : public juce::DocumentWindow {
  public:
    PluginEditorWindow(tracktion::Plugin& plugin, tracktion::PluginWindowState& state);
    ~PluginEditorWindow() override;

    void closeButtonPressed() override;
    void moved() override;

    /**
     * @brief Check if close has been requested (user clicked X)
     * PluginWindowManager polls this to safely close the window from outside
     */
    bool isCloseRequested() const {
        return closeRequested_;
    }

    /**
     * @brief Clear the close request flag (after handling it)
     */
    void clearCloseRequest() {
        closeRequested_ = false;
    }

  private:
    tracktion::Plugin& plugin_;
    tracktion::PluginWindowState& state_;
    bool closeRequested_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

}  // namespace magda
