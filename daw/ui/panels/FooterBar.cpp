#include "FooterBar.hpp"

#include "../themes/DarkTheme.hpp"

namespace magica {

FooterBar::FooterBar() {
    setupButtons();
    ViewModeController::getInstance().addListener(this);
    updateButtonStates();
}

FooterBar::~FooterBar() {
    ViewModeController::getInstance().removeListener(this);
}

void FooterBar::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));

    // Draw top border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawLine(0.0f, 0.0f, static_cast<float>(getWidth()), 0.0f, 1.0f);
}

void FooterBar::resized() {
    auto bounds = getLocalBounds();

    // Center the buttons horizontally
    int totalButtonsWidth = NUM_MODES * BUTTON_WIDTH + (NUM_MODES - 1) * BUTTON_SPACING;
    int startX = (bounds.getWidth() - totalButtonsWidth) / 2;

    int buttonY = (bounds.getHeight() - BUTTON_HEIGHT) / 2;

    for (int i = 0; i < NUM_MODES; ++i) {
        int buttonX = startX + i * (BUTTON_WIDTH + BUTTON_SPACING);
        modeButtons[static_cast<size_t>(i)]->setBounds(buttonX, buttonY, BUTTON_WIDTH,
                                                       BUTTON_HEIGHT);
    }
}

void FooterBar::viewModeChanged(ViewMode /*mode*/, const AudioEngineProfile& /*profile*/) {
    updateButtonStates();
}

void FooterBar::setupButtons() {
    const std::array<ViewMode, NUM_MODES> modes = {ViewMode::Live, ViewMode::Arrange, ViewMode::Mix,
                                                   ViewMode::Master};

    for (size_t i = 0; i < NUM_MODES; ++i) {
        modeButtons[i] = std::make_unique<juce::TextButton>(getViewModeName(modes[i]));

        modeButtons[i]->setClickingTogglesState(false);
        modeButtons[i]->onClick = [mode = modes[i]]() {
            ViewModeController::getInstance().setViewMode(mode);
        };

        // Style the button
        modeButtons[i]->setColour(juce::TextButton::buttonColourId,
                                  DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
        modeButtons[i]->setColour(juce::TextButton::buttonOnColourId,
                                  DarkTheme::getColour(DarkTheme::BUTTON_ACTIVE));
        modeButtons[i]->setColour(juce::TextButton::textColourOffId,
                                  DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
        modeButtons[i]->setColour(juce::TextButton::textColourOnId,
                                  DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));

        addAndMakeVisible(*modeButtons[i]);
    }
}

void FooterBar::updateButtonStates() {
    auto currentMode = ViewModeController::getInstance().getViewMode();

    const std::array<ViewMode, NUM_MODES> modes = {ViewMode::Live, ViewMode::Arrange, ViewMode::Mix,
                                                   ViewMode::Master};

    for (size_t i = 0; i < NUM_MODES; ++i) {
        bool isActive = (modes[i] == currentMode);
        modeButtons[i]->setToggleState(isActive, juce::dontSendNotification);

        // Update button appearance based on active state
        if (isActive) {
            modeButtons[i]->setColour(juce::TextButton::buttonColourId,
                                      DarkTheme::getColour(DarkTheme::BUTTON_ACTIVE));
        } else {
            modeButtons[i]->setColour(juce::TextButton::buttonColourId,
                                      DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
        }
    }

    repaint();
}

}  // namespace magica
