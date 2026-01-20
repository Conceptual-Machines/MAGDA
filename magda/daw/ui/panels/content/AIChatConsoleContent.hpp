#pragma once

#include "PanelContent.hpp"

namespace magda::daw::ui {

/**
 * @brief AI Chat console panel content
 *
 * Chat interface for interacting with AI assistant.
 */
class AIChatConsoleContent : public PanelContent {
  public:
    AIChatConsoleContent();
    ~AIChatConsoleContent() override = default;

    PanelContentType getContentType() const override {
        return PanelContentType::AIChatConsole;
    }

    PanelContentInfo getContentInfo() const override {
        return {PanelContentType::AIChatConsole, "AI Chat", "AI assistant chat", "AIChat"};
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void onActivated() override;
    void onDeactivated() override;

  private:
    juce::Label titleLabel_;
    juce::TextEditor chatHistory_;
    juce::TextEditor inputBox_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIChatConsoleContent)
};

}  // namespace magda::daw::ui
