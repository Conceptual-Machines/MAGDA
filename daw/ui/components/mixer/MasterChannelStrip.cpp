#include "MasterChannelStrip.hpp"

#include "../../themes/DarkTheme.hpp"

namespace magica {

// Level meter component
class MasterChannelStrip::LevelMeter : public juce::Component {
  public:
    void setLevel(float newLevel) {
        level = newLevel;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(DarkTheme::getColour(DarkTheme::SURFACE));
        g.fillRoundedRectangle(bounds, 2.0f);

        // Meter level
        float meterHeight = bounds.getHeight() * level;
        auto meterBounds = bounds.removeFromBottom(meterHeight).reduced(1.0f, 1.0f);

        // Gradient from green to yellow to red
        if (level < 0.6f) {
            g.setColour(DarkTheme::getColour(DarkTheme::LEVEL_METER_GREEN));
        } else if (level < 0.85f) {
            g.setColour(DarkTheme::getColour(DarkTheme::LEVEL_METER_YELLOW));
        } else {
            g.setColour(DarkTheme::getColour(DarkTheme::LEVEL_METER_RED));
        }

        g.fillRoundedRectangle(meterBounds, 1.0f);
    }

  private:
    float level = 0.0f;
};

MasterChannelStrip::MasterChannelStrip(Orientation orientation) : orientation_(orientation) {
    setupControls();

    // Register as TrackManager listener
    TrackManager::getInstance().addListener(this);

    // Load initial state
    updateFromMasterState();
}

MasterChannelStrip::~MasterChannelStrip() {
    TrackManager::getInstance().removeListener(this);
}

void MasterChannelStrip::setupControls() {
    // Title label
    titleLabel = std::make_unique<juce::Label>("Master", "Master");
    titleLabel->setColour(juce::Label::textColourId, DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    titleLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*titleLabel);

    // Volume slider
    volumeSlider = std::make_unique<juce::Slider>(orientation_ == Orientation::Vertical
                                                      ? juce::Slider::LinearVertical
                                                      : juce::Slider::LinearHorizontal,
                                                  juce::Slider::NoTextBox);
    volumeSlider->setRange(0.0, 1.0);
    volumeSlider->setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    volumeSlider->setColour(juce::Slider::thumbColourId,
                            DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    volumeSlider->onValueChange = [this]() {
        TrackManager::getInstance().setMasterVolume(static_cast<float>(volumeSlider->getValue()));
    };
    addAndMakeVisible(*volumeSlider);

    // Pan slider
    panSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                               juce::Slider::NoTextBox);
    panSlider->setRange(-1.0, 1.0);
    panSlider->setColour(juce::Slider::rotarySliderFillColourId,
                         DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    panSlider->setColour(juce::Slider::rotarySliderOutlineColourId,
                         DarkTheme::getColour(DarkTheme::SURFACE));
    panSlider->onValueChange = [this]() {
        TrackManager::getInstance().setMasterPan(static_cast<float>(panSlider->getValue()));
    };
    addAndMakeVisible(*panSlider);

    // Mute button
    muteButton = std::make_unique<juce::TextButton>("M");
    muteButton->setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    muteButton->setColour(juce::TextButton::buttonOnColourId,
                          DarkTheme::getColour(DarkTheme::STATUS_WARNING));
    muteButton->setColour(juce::TextButton::textColourOffId,
                          DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    muteButton->setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::BACKGROUND));
    muteButton->setClickingTogglesState(true);
    muteButton->onClick = [this]() {
        TrackManager::getInstance().setMasterMuted(muteButton->getToggleState());
    };
    addAndMakeVisible(*muteButton);

    // Solo button
    soloButton = std::make_unique<juce::TextButton>("S");
    soloButton->setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    soloButton->setColour(juce::TextButton::buttonOnColourId,
                          DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
    soloButton->setColour(juce::TextButton::textColourOffId,
                          DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    soloButton->setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::BACKGROUND));
    soloButton->setClickingTogglesState(true);
    soloButton->onClick = [this]() {
        TrackManager::getInstance().setMasterSoloed(soloButton->getToggleState());
    };
    addAndMakeVisible(*soloButton);

    // Level meter
    levelMeter = std::make_unique<LevelMeter>();
    addAndMakeVisible(*levelMeter);
}

void MasterChannelStrip::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));

    // Draw border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(getLocalBounds(), 1);
}

void MasterChannelStrip::resized() {
    auto bounds = getLocalBounds().reduced(4);

    if (orientation_ == Orientation::Vertical) {
        // Vertical layout (for MixerView and SessionView)
        titleLabel->setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(4);

        auto panArea = bounds.removeFromTop(40);
        panSlider->setBounds(panArea.reduced(8, 0));
        bounds.removeFromTop(4);

        auto buttonArea = bounds.removeFromTop(24);
        int buttonWidth = (buttonArea.getWidth() - 4) / 2;
        muteButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        buttonArea.removeFromLeft(4);
        soloButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
        bounds.removeFromTop(4);

        // Fader and meter take remaining space
        auto faderMeterArea = bounds;
        levelMeter->setBounds(faderMeterArea.removeFromRight(12));
        faderMeterArea.removeFromRight(4);
        volumeSlider->setBounds(faderMeterArea);
    } else {
        // Horizontal layout (for Arrange view - at bottom of track content)
        titleLabel->setBounds(bounds.removeFromLeft(60));
        bounds.removeFromLeft(8);

        auto buttonArea = bounds.removeFromLeft(60);
        int buttonHeight = (buttonArea.getHeight() - 4) / 2;
        muteButton->setBounds(buttonArea.removeFromTop(buttonHeight).reduced(2, 0));
        buttonArea.removeFromTop(4);
        soloButton->setBounds(buttonArea.removeFromTop(buttonHeight).reduced(2, 0));
        bounds.removeFromLeft(8);

        panSlider->setBounds(bounds.removeFromLeft(50));
        bounds.removeFromLeft(8);

        levelMeter->setBounds(bounds.removeFromRight(12));
        bounds.removeFromRight(4);
        volumeSlider->setBounds(bounds);
    }
}

void MasterChannelStrip::masterChannelChanged() {
    updateFromMasterState();
}

void MasterChannelStrip::updateFromMasterState() {
    const auto& master = TrackManager::getInstance().getMasterChannel();

    volumeSlider->setValue(master.volume, juce::dontSendNotification);
    panSlider->setValue(master.pan, juce::dontSendNotification);
    muteButton->setToggleState(master.muted, juce::dontSendNotification);
    soloButton->setToggleState(master.soloed, juce::dontSendNotification);
}

void MasterChannelStrip::setMeterLevel(float level) {
    levelMeter->setLevel(level);
}

}  // namespace magica
