#include "MagdaUIBehaviour.hpp"

namespace magda {

std::unique_ptr<juce::Component> MagdaUIBehaviour::createPluginWindow(
    tracktion::PluginWindowState& state) {
    // Cast to Plugin::WindowState to access the plugin
    auto* pluginState = dynamic_cast<tracktion::Plugin::WindowState*>(&state);
    if (!pluginState) {
        DBG("MagdaUIBehaviour::createPluginWindow - not a Plugin::WindowState");
        return nullptr;
    }

    auto& plugin = pluginState->plugin;
    DBG("MagdaUIBehaviour::createPluginWindow - creating window for: " << plugin.getName());

    // Create the window
    auto window = std::make_unique<PluginEditorWindow>(plugin, state);

    // Window might fail to create if plugin has no editor
    if (window->getContentComponent() == nullptr) {
        DBG("  -> Plugin has no editor component");
        return nullptr;
    }

    DBG("  -> Window created successfully");
    return window;
}

// =============================================================================
// PluginEditorWindow Implementation
// =============================================================================

PluginEditorWindow::PluginEditorWindow(tracktion::Plugin& plugin,
                                       tracktion::PluginWindowState& state)
    : DocumentWindow(plugin.getName(),
                     juce::LookAndFeel::getDefaultLookAndFeel().findColour(
                         juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::minimiseButton | DocumentWindow::closeButton),
      plugin_(plugin),
      state_(state) {
    setUsingNativeTitleBar(true);

    // Try to create the plugin's editor
    std::unique_ptr<juce::Component> editor;

    // For external plugins, get the native editor
    if (auto* extPlugin = dynamic_cast<tracktion::ExternalPlugin*>(&plugin)) {
        if (auto* audioPluginInstance = extPlugin->getAudioPluginInstance()) {
            if (audioPluginInstance->hasEditor()) {
                editor.reset(audioPluginInstance->createEditorIfNeeded());
                DBG("PluginEditorWindow: Created native editor for: " << plugin.getName());
            }
        }
    }

    // If no native editor, try the plugin's generic editor
    if (!editor) {
        auto editorComponent = plugin.createEditor();
        if (editorComponent) {
            editor = std::move(editorComponent);
            DBG("PluginEditorWindow: Created generic editor for: " << plugin.getName());
        }
    }

    if (editor) {
        setContentOwned(editor.release(), true);

        // Check if editor is resizable (only AudioProcessorEditor has this property)
        bool isResizable = false;
        if (auto* processorEditor =
                dynamic_cast<juce::AudioProcessorEditor*>(getContentComponent())) {
            isResizable = processorEditor->isResizable();
        }
        setResizable(isResizable, false);

        // Set initial size from editor
        int width = getContentComponent()->getWidth();
        int height = getContentComponent()->getHeight();
        if (width > 0 && height > 0) {
            setSize(width, height);
        } else {
            setSize(400, 300);  // Default size
        }

        // Position window
        auto pos = state.choosePositionForPluginWindow();
        setTopLeftPosition(pos.x, pos.y);

        setVisible(true);
    } else {
        DBG("PluginEditorWindow: Failed to create editor for: " << plugin.getName());
    }
}

PluginEditorWindow::~PluginEditorWindow() {
    clearContentComponent();
}

void PluginEditorWindow::closeButtonPressed() {
    DBG("PluginEditorWindow::closeButtonPressed - setting close requested flag");
    // Just set a flag - PluginWindowManager will detect this and close properly
    // from OUTSIDE the window's event handler (avoiding malloc errors)
    // We don't call setVisible(false) because that can trigger internal JUCE state changes
    // that interfere with the subsequent closeWindowExplicitly() call.
    closeRequested_ = true;
}

void PluginEditorWindow::moved() {
    // Save window position for next time
    if (state_.lastWindowBounds.has_value()) {
        state_.lastWindowBounds = getBounds();
    }
}

}  // namespace magda
