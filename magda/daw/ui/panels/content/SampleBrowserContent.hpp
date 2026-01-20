#pragma once

#include "PanelContent.hpp"

namespace magda::daw::ui {

/**
 * @brief Sample browser panel content
 *
 * File browser for audio samples with preview functionality.
 */
class SampleBrowserContent : public PanelContent {
  public:
    SampleBrowserContent();
    ~SampleBrowserContent() override = default;

    PanelContentType getContentType() const override {
        return PanelContentType::SampleBrowser;
    }

    PanelContentInfo getContentInfo() const override {
        return {PanelContentType::SampleBrowser, "Samples", "Browse audio samples", "Sample"};
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void onActivated() override;
    void onDeactivated() override;

  private:
    juce::TextEditor searchBox_;
    juce::Label titleLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBrowserContent)
};

}  // namespace magda::daw::ui
