#pragma once

#include "PanelContent.hpp"

namespace magda::daw::ui {

/**
 * @brief Scripting console panel content
 *
 * Code editor and REPL for scripting automation.
 */
class ScriptingConsoleContent : public PanelContent {
  public:
    ScriptingConsoleContent();
    ~ScriptingConsoleContent() override = default;

    PanelContentType getContentType() const override {
        return PanelContentType::ScriptingConsole;
    }

    PanelContentInfo getContentInfo() const override {
        return {PanelContentType::ScriptingConsole, "Script", "Script editor/REPL", "Script"};
    }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void onActivated() override;
    void onDeactivated() override;

  private:
    juce::Label titleLabel_;
    juce::TextEditor outputArea_;
    juce::TextEditor inputBox_;

    void executeCommand(const juce::String& command);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingConsoleContent)
};

}  // namespace magda::daw::ui
