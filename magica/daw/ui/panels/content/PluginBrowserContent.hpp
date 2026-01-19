#pragma once

#include "PanelContent.hpp"

namespace magica::daw::ui {

/**
 * @brief Plugin browser panel content
 *
 * Displays a tree view of available plugins organized by category,
 * with search functionality.
 */
class PluginBrowserContent : public PanelContent {
  public:
    PluginBrowserContent();
    ~PluginBrowserContent() override = default;

    PanelContentType getContentType() const override {
        return PanelContentType::PluginBrowser;
    }

    PanelContentInfo getContentInfo() const override {
        return {PanelContentType::PluginBrowser, "Plugins", "Browse and insert plugins", "Plugin"};
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void onActivated() override;
    void onDeactivated() override;

  private:
    juce::TextEditor searchBox_;
    juce::Label titleLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserContent)
};

}  // namespace magica::daw::ui
