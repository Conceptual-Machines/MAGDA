#include "SampleBrowserContent.hpp"

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"

namespace magda::daw::ui {

SampleBrowserContent::SampleBrowserContent() {
    setName("Sample Browser");

    // Setup title
    titleLabel_.setText("Samples", juce::dontSendNotification);
    titleLabel_.setFont(FontManager::getInstance().getUIFont(14.0f));
    titleLabel_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    addAndMakeVisible(titleLabel_);

    // Setup search box
    searchBox_.setTextToShowWhenEmpty("Search samples...", DarkTheme::getSecondaryTextColour());
    searchBox_.setColour(juce::TextEditor::backgroundColourId,
                         DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
    searchBox_.setColour(juce::TextEditor::textColourId, DarkTheme::getTextColour());
    searchBox_.setColour(juce::TextEditor::outlineColourId, DarkTheme::getBorderColour());
    addAndMakeVisible(searchBox_);
}

void SampleBrowserContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());

    // Placeholder content area
    auto contentArea = getLocalBounds().reduced(10).withTrimmedTop(70);
    g.setColour(DarkTheme::getSecondaryTextColour());
    g.setFont(FontManager::getInstance().getUIFont(12.0f));
    g.drawText("Sample browser will appear here", contentArea, juce::Justification::centredTop);
}

void SampleBrowserContent::resized() {
    auto bounds = getLocalBounds().reduced(10);

    titleLabel_.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(8);  // Spacing
    searchBox_.setBounds(bounds.removeFromTop(28));
}

void SampleBrowserContent::onActivated() {
    // Could scan for samples here
}

void SampleBrowserContent::onDeactivated() {
    // Could save browse state here
}

}  // namespace magda::daw::ui
