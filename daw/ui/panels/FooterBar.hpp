#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

#include "core/ViewModeController.hpp"
#include "core/ViewModeState.hpp"

namespace magica {

/**
 * @brief Footer bar with view mode buttons
 *
 * Displays four buttons (Live/Arrange/Mix/Master) to switch between
 * different view modes. The active mode is highlighted.
 */
class FooterBar : public juce::Component, public ViewModeListener {
  public:
    FooterBar();
    ~FooterBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ViewModeListener interface
    void viewModeChanged(ViewMode mode, const AudioEngineProfile& profile) override;

  private:
    static constexpr int NUM_MODES = 4;
    static constexpr int BUTTON_WIDTH = 80;
    static constexpr int BUTTON_HEIGHT = 28;
    static constexpr int BUTTON_SPACING = 8;

    std::array<std::unique_ptr<juce::TextButton>, NUM_MODES> modeButtons;

    void setupButtons();
    void updateButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterBar)
};

}  // namespace magica
