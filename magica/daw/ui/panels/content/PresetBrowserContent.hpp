#pragma once

#include "PanelContent.hpp"

namespace magica::daw::ui {

/**
 * @brief Preset browser panel content
 *
 * Browse and manage presets for plugins and instruments.
 */
class PresetBrowserContent : public PanelContent {
  public:
    PresetBrowserContent();
    ~PresetBrowserContent() override = default;

    PanelContentType getContentType() const override {
        return PanelContentType::PresetBrowser;
    }

    PanelContentInfo getContentInfo() const override {
        return {PanelContentType::PresetBrowser, "Presets", "Browse presets", "Preset"};
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void onActivated() override;
    void onDeactivated() override;

  private:
    juce::TextEditor searchBox_;
    juce::Label titleLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserContent)
};

}  // namespace magica::daw::ui
