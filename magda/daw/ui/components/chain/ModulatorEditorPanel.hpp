#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

#include "core/ModInfo.hpp"
#include "core/ModulatorEngine.hpp"
#include "ui/components/common/TextSlider.hpp"

namespace magda::daw::ui {

/**
 * @brief Animated waveform display component
 */
class WaveformDisplay : public juce::Component, private juce::Timer {
  public:
    WaveformDisplay() {
        startTimer(33);  // 30 FPS animation
    }

    void setModInfo(const magda::ModInfo* mod) {
        mod_ = mod;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        if (!mod_) {
            return;
        }

        auto bounds = getLocalBounds().toFloat();
        const float width = bounds.getWidth();
        const float height = bounds.getHeight();
        const float centerY = height * 0.5f;

        // Draw waveform path
        juce::Path waveformPath;
        const int numPoints = 100;

        for (int i = 0; i < numPoints; ++i) {
            float phase = static_cast<float>(i) / static_cast<float>(numPoints - 1);
            float value = magda::ModulatorEngine::generateWaveform(mod_->waveform, phase);

            // Invert value so high values are at top
            float y = centerY + (0.5f - value) * (height - 8.0f);
            float x = bounds.getX() + phase * width;

            if (i == 0) {
                waveformPath.startNewSubPath(x, y);
            } else {
                waveformPath.lineTo(x, y);
            }
        }

        // Draw the waveform line
        g.setColour(juce::Colours::orange.withAlpha(0.6f));
        g.strokePath(waveformPath, juce::PathStrokeType(1.5f));

        // Draw current phase indicator (dot)
        float currentX = bounds.getX() + mod_->phase * width;
        float currentValue = mod_->value;
        float currentY = centerY + (0.5f - currentValue) * (height - 8.0f);

        g.setColour(juce::Colours::orange);
        g.fillEllipse(currentX - 3.0f, currentY - 3.0f, 6.0f, 6.0f);
    }

  private:
    void timerCallback() override {
        repaint();
    }

    const magda::ModInfo* mod_ = nullptr;
};

/**
 * @brief Panel for editing modulator settings
 *
 * Shows when a mod is selected from the mods panel.
 * Displays type selector, rate control, and target info.
 *
 * Layout:
 * +------------------+
 * |    MOD NAME      |  <- Header with mod name
 * +------------------+
 * | Type: [LFO   v]  |  <- Type selector
 * +------------------+
 * |   Rate: 1.0 Hz   |  <- Rate slider
 * +------------------+
 * | Target: Device   |  <- Target info
 * |   Param Name     |
 * +------------------+
 */
class ModulatorEditorPanel : public juce::Component {
  public:
    ModulatorEditorPanel();
    ~ModulatorEditorPanel() override = default;

    // Set the mod to edit
    void setModInfo(const magda::ModInfo& mod, const magda::ModInfo* liveMod = nullptr);

    // Set the selected mod index (-1 for none)
    void setSelectedModIndex(int index);
    int getSelectedModIndex() const {
        return selectedModIndex_;
    }

    // Callbacks
    std::function<void(magda::ModType type)> onTypeChanged;
    std::function<void(float rate)> onRateChanged;
    std::function<void(magda::LFOWaveform waveform)> onWaveformChanged;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Preferred width for this panel
    static constexpr int PREFERRED_WIDTH = 120;

  private:
    int selectedModIndex_ = -1;
    magda::ModInfo currentMod_;
    const magda::ModInfo* liveModPtr_ = nullptr;  // Pointer to live mod for waveform animation

    // UI Components
    juce::Label nameLabel_;
    juce::ComboBox typeSelector_;
    juce::ComboBox waveformCombo_;
    WaveformDisplay waveformDisplay_;
    TextSlider rateSlider_{TextSlider::Format::Decimal};
    juce::Label targetLabel_;

    void updateFromMod();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorEditorPanel)
};

}  // namespace magda::daw::ui
