#pragma once

#include "PanelContent.hpp"

namespace magica::daw::ui {

/**
 * @brief Inspector panel content
 *
 * Displays properties of the currently selected item(s).
 */
class InspectorContent : public PanelContent {
  public:
    InspectorContent();
    ~InspectorContent() override = default;

    PanelContentType getContentType() const override {
        return PanelContentType::Inspector;
    }

    PanelContentInfo getContentInfo() const override {
        return {PanelContentType::Inspector, "Inspector", "Selection properties", "Inspector"};
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void onActivated() override;
    void onDeactivated() override;

  private:
    juce::Label titleLabel_;
    juce::Label noSelectionLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorContent)
};

}  // namespace magica::daw::ui
