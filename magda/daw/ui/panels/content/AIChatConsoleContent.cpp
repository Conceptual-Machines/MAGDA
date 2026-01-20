#include "AIChatConsoleContent.hpp"

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"

namespace magda::daw::ui {

AIChatConsoleContent::AIChatConsoleContent() {
    setName("AI Chat");

    // Setup title
    titleLabel_.setText("AI Assistant", juce::dontSendNotification);
    titleLabel_.setFont(FontManager::getInstance().getUIFont(14.0f));
    titleLabel_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    addAndMakeVisible(titleLabel_);

    // Chat history area
    chatHistory_.setMultiLine(true);
    chatHistory_.setReadOnly(true);
    chatHistory_.setColour(juce::TextEditor::backgroundColourId,
                           DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
    chatHistory_.setColour(juce::TextEditor::textColourId, DarkTheme::getTextColour());
    chatHistory_.setColour(juce::TextEditor::outlineColourId, DarkTheme::getBorderColour());
    chatHistory_.setText("Welcome! Ask me anything about your project...\n");
    addAndMakeVisible(chatHistory_);

    // Input box
    inputBox_.setTextToShowWhenEmpty("Type a message...", DarkTheme::getSecondaryTextColour());
    inputBox_.setColour(juce::TextEditor::backgroundColourId,
                        DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
    inputBox_.setColour(juce::TextEditor::textColourId, DarkTheme::getTextColour());
    inputBox_.setColour(juce::TextEditor::outlineColourId, DarkTheme::getBorderColour());
    inputBox_.onReturnKey = [this]() {
        auto text = inputBox_.getText();
        if (text.isNotEmpty()) {
            chatHistory_.moveCaretToEnd();
            chatHistory_.insertTextAtCaret("\nYou: " + text + "\n");
            chatHistory_.insertTextAtCaret("AI: [Response would appear here]\n");
            inputBox_.clear();
        }
    };
    addAndMakeVisible(inputBox_);
}

void AIChatConsoleContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());
}

void AIChatConsoleContent::resized() {
    auto bounds = getLocalBounds().reduced(10);

    titleLabel_.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(8);  // Spacing

    inputBox_.setBounds(bounds.removeFromBottom(28));
    bounds.removeFromBottom(8);  // Spacing

    chatHistory_.setBounds(bounds);
}

void AIChatConsoleContent::onActivated() {
    inputBox_.grabKeyboardFocus();
}

void AIChatConsoleContent::onDeactivated() {
    // Could save chat history here
}

}  // namespace magda::daw::ui
