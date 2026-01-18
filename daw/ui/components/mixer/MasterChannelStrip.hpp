#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "core/TrackManager.hpp"

namespace magica {

/**
 * @brief Reusable master channel strip component
 *
 * Can be added to any view to display and control the master channel.
 * Syncs with TrackManager's master channel state.
 */
class MasterChannelStrip : public juce::Component, public TrackManagerListener {
  public:
    // Orientation options
    enum class Orientation { Vertical, Horizontal };

    MasterChannelStrip(Orientation orientation = Orientation::Vertical);
    ~MasterChannelStrip() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // TrackManagerListener
    void tracksChanged() override {}
    void masterChannelChanged() override;

    // Set meter level (for future audio integration)
    void setMeterLevel(float level);

  private:
    Orientation orientation_;

    // UI Components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Slider> volumeSlider;
    std::unique_ptr<juce::Slider> panSlider;
    std::unique_ptr<juce::TextButton> muteButton;
    std::unique_ptr<juce::TextButton> soloButton;

    // Meter component
    class LevelMeter;
    std::unique_ptr<LevelMeter> levelMeter;

    void setupControls();
    void updateFromMasterState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterChannelStrip)
};

}  // namespace magica
