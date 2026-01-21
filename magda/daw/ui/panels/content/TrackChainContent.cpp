#include "TrackChainContent.hpp"

#include <cmath>

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"
#include "../../themes/MixerMetrics.hpp"
#include "core/DeviceInfo.hpp"

namespace magda::daw::ui {

//==============================================================================
// DeviceSlotComponent - Interactive device display
//==============================================================================
class TrackChainContent::DeviceSlotComponent : public juce::Component {
  public:
    static constexpr int GAIN_SLIDER_WIDTH = 24;

    DeviceSlotComponent(TrackChainContent& owner, magda::TrackId trackId,
                        const magda::DeviceInfo& device)
        : owner_(owner), trackId_(trackId), device_(device) {
        // Bypass button
        bypassButton_.setButtonText("B");
        bypassButton_.setColour(juce::TextButton::buttonColourId,
                                DarkTheme::getColour(DarkTheme::SURFACE));
        bypassButton_.setColour(juce::TextButton::buttonOnColourId,
                                DarkTheme::getColour(DarkTheme::STATUS_WARNING));
        bypassButton_.setColour(juce::TextButton::textColourOffId,
                                DarkTheme::getSecondaryTextColour());
        bypassButton_.setColour(juce::TextButton::textColourOnId,
                                DarkTheme::getColour(DarkTheme::BACKGROUND));
        bypassButton_.setClickingTogglesState(true);
        bypassButton_.setToggleState(device_.bypassed, juce::dontSendNotification);
        bypassButton_.onClick = [this]() {
            magda::TrackManager::getInstance().setDeviceBypassed(trackId_, device_.id,
                                                                 bypassButton_.getToggleState());
        };
        addAndMakeVisible(bypassButton_);

        // Gain toggle button
        gainToggleButton_.setButtonText("G");
        gainToggleButton_.setColour(juce::TextButton::buttonColourId,
                                    DarkTheme::getColour(DarkTheme::SURFACE));
        gainToggleButton_.setColour(juce::TextButton::buttonOnColourId,
                                    DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        gainToggleButton_.setColour(juce::TextButton::textColourOffId,
                                    DarkTheme::getSecondaryTextColour());
        gainToggleButton_.setColour(juce::TextButton::textColourOnId,
                                    DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
        gainToggleButton_.setClickingTogglesState(true);
        gainToggleButton_.setToggleState(gainSliderVisible_, juce::dontSendNotification);
        gainToggleButton_.onClick = [this]() {
            gainSliderVisible_ = gainToggleButton_.getToggleState();
            gainSlider_.setVisible(gainSliderVisible_);
            gainLabel_.setVisible(gainSliderVisible_);
            resized();
            repaint();
        };
        addAndMakeVisible(gainToggleButton_);

        // Gain slider (vertical)
        gainSlider_.setSliderStyle(juce::Slider::LinearVertical);
        gainSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        gainSlider_.setRange(-60.0, 6.0, 0.1);  // dB range
        gainSlider_.setValue(0.0);              // Unity gain
        gainSlider_.setDoubleClickReturnValue(true, 0.0);
        gainSlider_.setColour(juce::Slider::trackColourId,
                              DarkTheme::getColour(DarkTheme::SURFACE));
        gainSlider_.setColour(juce::Slider::backgroundColourId,
                              DarkTheme::getColour(DarkTheme::BACKGROUND));
        gainSlider_.setColour(juce::Slider::thumbColourId,
                              DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        gainSlider_.onValueChange = [this]() {
            // Update the gain label
            double db = gainSlider_.getValue();
            if (db <= -60.0) {
                gainLabel_.setText("-inf", juce::dontSendNotification);
            } else {
                gainLabel_.setText(juce::String(db, 1), juce::dontSendNotification);
            }
            // TODO: Update device gain value in TrackManager
        };
        gainSlider_.setVisible(false);
        addChildComponent(gainSlider_);

        // Gain value label
        gainLabel_.setText("0.0", juce::dontSendNotification);
        gainLabel_.setFont(FontManager::getInstance().getUIFont(8.0f));
        gainLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
        gainLabel_.setJustificationType(juce::Justification::centred);
        gainLabel_.setVisible(false);
        addChildComponent(gainLabel_);

        // Delete button
        deleteButton_.setButtonText(juce::String::fromUTF8("✕"));
        deleteButton_.setColour(juce::TextButton::buttonColourId,
                                DarkTheme::getColour(DarkTheme::SURFACE));
        deleteButton_.setColour(juce::TextButton::textColourOffId,
                                DarkTheme::getSecondaryTextColour());
        deleteButton_.onClick = [this]() {
            magda::TrackManager::getInstance().removeDeviceFromTrack(trackId_, device_.id);
        };
        addAndMakeVisible(deleteButton_);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds();

        // If gain slider visible, account for it
        if (gainSliderVisible_) {
            bounds.removeFromRight(GAIN_SLIDER_WIDTH);
        }

        // Background
        auto bgColour = device_.bypassed ? DarkTheme::getColour(DarkTheme::SURFACE).withAlpha(0.5f)
                                         : DarkTheme::getColour(DarkTheme::SURFACE);
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        // Border
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

        // Device name
        auto textBounds = bounds.reduced(6);
        textBounds.removeFromTop(22);     // Skip button row
        textBounds.removeFromBottom(16);  // Skip format label

        auto textColour = device_.bypassed ? DarkTheme::getSecondaryTextColour().withAlpha(0.5f)
                                           : DarkTheme::getTextColour();
        g.setColour(textColour);
        g.setFont(FontManager::getInstance().getUIFontBold(11.0f));
        g.drawText(device_.name, textBounds, juce::Justification::centred);

        // Manufacturer
        auto mfrBounds = textBounds;
        mfrBounds.removeFromTop(16);
        g.setColour(DarkTheme::getSecondaryTextColour());
        g.setFont(FontManager::getInstance().getUIFont(9.0f));
        g.drawText(device_.manufacturer, mfrBounds, juce::Justification::centred);

        // Format badge at bottom
        auto formatBounds = bounds.reduced(6).removeFromBottom(14);
        g.setColour(DarkTheme::getSecondaryTextColour());
        g.setFont(FontManager::getInstance().getUIFont(8.0f));
        g.drawText(device_.getFormatString(), formatBounds, juce::Justification::centredRight);

        // Draw gain slider background if visible
        if (gainSliderVisible_) {
            auto sliderArea = getLocalBounds().removeFromRight(GAIN_SLIDER_WIDTH);
            g.setColour(DarkTheme::getColour(DarkTheme::BACKGROUND));
            g.fillRoundedRectangle(sliderArea.toFloat(), 4.0f);
            g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
            g.drawRoundedRectangle(sliderArea.toFloat(), 4.0f, 1.0f);
        }
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(4);

        // Gain slider on the right if visible
        if (gainSliderVisible_) {
            auto sliderArea = bounds.removeFromRight(GAIN_SLIDER_WIDTH - 4);
            gainLabel_.setBounds(sliderArea.removeFromBottom(12));
            gainSlider_.setBounds(sliderArea.reduced(2, 4));
        }

        auto buttonRow = bounds.removeFromTop(18);

        bypassButton_.setBounds(buttonRow.removeFromLeft(20));
        buttonRow.removeFromLeft(2);
        gainToggleButton_.setBounds(buttonRow.removeFromLeft(20));
        deleteButton_.setBounds(buttonRow.removeFromRight(20));
    }

    bool isGainSliderVisible() const {
        return gainSliderVisible_;
    }

  private:
    TrackChainContent& owner_;
    magda::TrackId trackId_;
    magda::DeviceInfo device_;
    juce::TextButton bypassButton_;
    juce::TextButton gainToggleButton_;
    juce::TextButton deleteButton_;
    juce::Slider gainSlider_;
    juce::Label gainLabel_;
    bool gainSliderVisible_ = false;
};

// dB conversion helpers
namespace {
constexpr float MIN_DB = -60.0f;
constexpr float MAX_DB = 6.0f;
constexpr float UNITY_DB = 0.0f;

float gainToDb(float gain) {
    if (gain <= 0.0f)
        return MIN_DB;
    return 20.0f * std::log10(gain);
}

float dbToGain(float db) {
    if (db <= MIN_DB)
        return 0.0f;
    return std::pow(10.0f, db / 20.0f);
}

float dbToFaderPos(float db) {
    if (db <= MIN_DB)
        return 0.0f;
    if (db >= MAX_DB)
        return 1.0f;

    if (db < UNITY_DB) {
        return 0.75f * (db - MIN_DB) / (UNITY_DB - MIN_DB);
    } else {
        return 0.75f + 0.25f * (db - UNITY_DB) / (MAX_DB - UNITY_DB);
    }
}

float faderPosToDb(float pos) {
    if (pos <= 0.0f)
        return MIN_DB;
    if (pos >= 1.0f)
        return MAX_DB;

    if (pos < 0.75f) {
        return MIN_DB + (pos / 0.75f) * (UNITY_DB - MIN_DB);
    } else {
        return UNITY_DB + ((pos - 0.75f) / 0.25f) * (MAX_DB - UNITY_DB);
    }
}
}  // namespace

TrackChainContent::TrackChainContent() {
    setName("Track Chain");

    // No selection label
    noSelectionLabel_.setText("Select a track to view its signal chain",
                              juce::dontSendNotification);
    noSelectionLabel_.setFont(FontManager::getInstance().getUIFont(12.0f));
    noSelectionLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    noSelectionLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(noSelectionLabel_);

    // Track name at right strip
    trackNameLabel_.setFont(FontManager::getInstance().getUIFont(11.0f));
    trackNameLabel_.setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    trackNameLabel_.setJustificationType(juce::Justification::centredLeft);
    addChildComponent(trackNameLabel_);

    // Mute button
    muteButton_.setButtonText("M");
    muteButton_.setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    muteButton_.setColour(juce::TextButton::buttonOnColourId,
                          DarkTheme::getColour(DarkTheme::STATUS_WARNING));
    muteButton_.setColour(juce::TextButton::textColourOffId, DarkTheme::getTextColour());
    muteButton_.setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::BACKGROUND));
    muteButton_.setClickingTogglesState(true);
    muteButton_.onClick = [this]() {
        if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
            magda::TrackManager::getInstance().setTrackMuted(selectedTrackId_,
                                                             muteButton_.getToggleState());
        }
    };
    addChildComponent(muteButton_);

    // Solo button
    soloButton_.setButtonText("S");
    soloButton_.setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    soloButton_.setColour(juce::TextButton::buttonOnColourId,
                          DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
    soloButton_.setColour(juce::TextButton::textColourOffId, DarkTheme::getTextColour());
    soloButton_.setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::BACKGROUND));
    soloButton_.setClickingTogglesState(true);
    soloButton_.onClick = [this]() {
        if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
            magda::TrackManager::getInstance().setTrackSoloed(selectedTrackId_,
                                                              soloButton_.getToggleState());
        }
    };
    addChildComponent(soloButton_);

    // Gain slider - using dB scale with unity at 0.75 position
    gainSlider_.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainSlider_.setRange(0.0, 1.0, 0.001);
    gainSlider_.setValue(0.75);  // Unity gain (0 dB)
    gainSlider_.setSliderSnapsToMousePosition(false);
    gainSlider_.setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    gainSlider_.setColour(juce::Slider::backgroundColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    gainSlider_.setColour(juce::Slider::thumbColourId,
                          DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    gainSlider_.setLookAndFeel(&mixerLookAndFeel_);
    gainSlider_.onValueChange = [this]() {
        if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
            float faderPos = static_cast<float>(gainSlider_.getValue());
            float db = faderPosToDb(faderPos);
            float gain = dbToGain(db);
            magda::TrackManager::getInstance().setTrackVolume(selectedTrackId_, gain);
            // Update gain label
            juce::String dbText;
            if (db <= MIN_DB) {
                dbText = "-inf";
            } else {
                dbText = juce::String(db, 1) + " dB";
            }
            gainValueLabel_.setText(dbText, juce::dontSendNotification);
        }
    };
    addChildComponent(gainSlider_);

    // Gain value label
    gainValueLabel_.setText("0.0 dB", juce::dontSendNotification);
    gainValueLabel_.setJustificationType(juce::Justification::centred);
    gainValueLabel_.setColour(juce::Label::textColourId,
                              DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
    gainValueLabel_.setFont(FontManager::getInstance().getUIFont(9.0f));
    addChildComponent(gainValueLabel_);

    // Pan slider (rotary knob)
    panSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider_.setRange(-1.0, 1.0, 0.01);
    panSlider_.setColour(juce::Slider::rotarySliderFillColourId,
                         DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    panSlider_.setColour(juce::Slider::rotarySliderOutlineColourId,
                         DarkTheme::getColour(DarkTheme::SURFACE));
    panSlider_.setLookAndFeel(&mixerLookAndFeel_);
    panSlider_.onValueChange = [this]() {
        if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
            magda::TrackManager::getInstance().setTrackPan(
                selectedTrackId_, static_cast<float>(panSlider_.getValue()));
            // Update pan label
            float pan = static_cast<float>(panSlider_.getValue());
            juce::String panText;
            if (std::abs(pan) < 0.01f) {
                panText = "C";
            } else if (pan < 0) {
                panText = juce::String(static_cast<int>(std::abs(pan) * 100)) + "L";
            } else {
                panText = juce::String(static_cast<int>(pan * 100)) + "R";
            }
            panValueLabel_.setText(panText, juce::dontSendNotification);
        }
    };
    addChildComponent(panSlider_);

    // Pan value label
    panValueLabel_.setText("C", juce::dontSendNotification);
    panValueLabel_.setJustificationType(juce::Justification::centred);
    panValueLabel_.setColour(juce::Label::textColourId,
                             DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
    panValueLabel_.setFont(FontManager::getInstance().getUIFont(10.0f));
    addChildComponent(panValueLabel_);

    // Add device button
    addDeviceButton_.setButtonText("+");
    addDeviceButton_.setColour(juce::TextButton::buttonColourId,
                               DarkTheme::getColour(DarkTheme::SURFACE));
    addDeviceButton_.setColour(juce::TextButton::textColourOffId,
                               DarkTheme::getSecondaryTextColour());
    addDeviceButton_.onClick = [this]() {
        // Would open plugin browser or show plugin selector
        DBG("Add device clicked - would show plugin selector");
    };
    addChildComponent(addDeviceButton_);

    // Register as listener
    magda::TrackManager::getInstance().addListener(this);

    // Check if there's already a selected track
    selectedTrackId_ = magda::TrackManager::getInstance().getSelectedTrack();
    updateFromSelectedTrack();
}

TrackChainContent::~TrackChainContent() {
    magda::TrackManager::getInstance().removeListener(this);
    // Clear look and feel before destruction
    gainSlider_.setLookAndFeel(nullptr);
    panSlider_.setLookAndFeel(nullptr);
}

void TrackChainContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());

    if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
        // Draw the chain area background
        auto bounds = getLocalBounds();
        auto stripWidth = 100;
        auto chainArea = bounds.withTrimmedRight(stripWidth);

        // Draw arrows between device slots
        auto slotArea = chainArea.reduced(8);
        int slotWidth = 100;
        int slotSpacing = 8;
        int arrowWidth = 20;

        int x = slotArea.getX();
        for (size_t i = 0; i < deviceSlots_.size(); ++i) {
            x += slotWidth;  // After device slot

            // Draw arrow after each device (except the last one before add button)
            auto arrowArea =
                juce::Rectangle<int>(x, slotArea.getY(), arrowWidth, slotArea.getHeight());
            g.setColour(DarkTheme::getSecondaryTextColour());

            int arrowY = arrowArea.getCentreY();
            int arrowX = arrowArea.getCentreX();
            g.drawLine(static_cast<float>(arrowX - 6), static_cast<float>(arrowY),
                       static_cast<float>(arrowX + 6), static_cast<float>(arrowY), 1.5f);
            // Arrow head
            g.drawLine(static_cast<float>(arrowX + 2), static_cast<float>(arrowY - 4),
                       static_cast<float>(arrowX + 6), static_cast<float>(arrowY), 1.5f);
            g.drawLine(static_cast<float>(arrowX + 2), static_cast<float>(arrowY + 4),
                       static_cast<float>(arrowX + 6), static_cast<float>(arrowY), 1.5f);

            x += arrowWidth + slotSpacing;
        }

        // Draw separator line before track strip
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawLine(static_cast<float>(chainArea.getRight()), 0.0f,
                   static_cast<float>(chainArea.getRight()), static_cast<float>(getHeight()), 1.0f);
    }
}

void TrackChainContent::resized() {
    auto bounds = getLocalBounds();
    const auto& metrics = magda::MixerMetrics::getInstance();

    if (selectedTrackId_ == magda::INVALID_TRACK_ID) {
        noSelectionLabel_.setBounds(bounds);
        addDeviceButton_.setVisible(false);
    } else {
        // Track info strip at right border
        auto stripWidth = 100;
        auto strip = bounds.removeFromRight(stripWidth).reduced(4);

        // Chain area (left of strip)
        auto chainArea = bounds.reduced(8);

        // Layout device slots horizontally
        int slotWidth = 100;
        int slotHeight = chainArea.getHeight();
        int arrowWidth = 20;
        int slotSpacing = 8;

        int x = chainArea.getX();
        for (auto& slot : deviceSlots_) {
            slot->setBounds(x, chainArea.getY(), slotWidth, slotHeight);
            x += slotWidth + arrowWidth + slotSpacing;
        }

        // Add device button after all slots
        addDeviceButton_.setBounds(x, chainArea.getY(), 40, slotHeight);
        addDeviceButton_.setVisible(true);

        // Track name at top of strip
        trackNameLabel_.setBounds(strip.removeFromTop(20));
        strip.removeFromTop(4);

        // Pan knob
        auto panArea = strip.removeFromTop(metrics.knobSize);
        panSlider_.setBounds(panArea.withSizeKeepingCentre(metrics.knobSize, metrics.knobSize));

        // Pan value label
        panValueLabel_.setBounds(strip.removeFromTop(14));
        strip.removeFromTop(4);

        // M/S buttons
        auto buttonRow = strip.removeFromTop(24);
        muteButton_.setBounds(buttonRow.removeFromLeft(36));
        buttonRow.removeFromLeft(4);
        soloButton_.setBounds(buttonRow.removeFromLeft(36));
        strip.removeFromTop(4);

        // Gain value label
        gainValueLabel_.setBounds(strip.removeFromTop(12));

        // Gain slider (vertical) - takes remaining space
        gainSlider_.setBounds(strip);
    }
}

void TrackChainContent::onActivated() {
    selectedTrackId_ = magda::TrackManager::getInstance().getSelectedTrack();
    updateFromSelectedTrack();
}

void TrackChainContent::onDeactivated() {
    // Nothing to do
}

void TrackChainContent::tracksChanged() {
    if (selectedTrackId_ != magda::INVALID_TRACK_ID) {
        const auto* track = magda::TrackManager::getInstance().getTrack(selectedTrackId_);
        if (!track) {
            selectedTrackId_ = magda::INVALID_TRACK_ID;
            updateFromSelectedTrack();
        }
    }
}

void TrackChainContent::trackPropertyChanged(int trackId) {
    if (static_cast<magda::TrackId>(trackId) == selectedTrackId_) {
        updateFromSelectedTrack();
    }
}

void TrackChainContent::trackSelectionChanged(magda::TrackId trackId) {
    selectedTrackId_ = trackId;
    updateFromSelectedTrack();
}

void TrackChainContent::trackDevicesChanged(magda::TrackId trackId) {
    if (trackId == selectedTrackId_) {
        rebuildDeviceSlots();
    }
}

void TrackChainContent::updateFromSelectedTrack() {
    if (selectedTrackId_ == magda::INVALID_TRACK_ID) {
        showTrackStrip(false);
        noSelectionLabel_.setVisible(true);
        deviceSlots_.clear();
    } else {
        const auto* track = magda::TrackManager::getInstance().getTrack(selectedTrackId_);
        if (track) {
            trackNameLabel_.setText(track->name, juce::dontSendNotification);
            muteButton_.setToggleState(track->muted, juce::dontSendNotification);
            soloButton_.setToggleState(track->soloed, juce::dontSendNotification);

            // Convert linear gain to fader position
            float db = gainToDb(track->volume);
            float faderPos = dbToFaderPos(db);
            gainSlider_.setValue(faderPos, juce::dontSendNotification);

            // Update gain label
            juce::String dbText;
            if (db <= MIN_DB) {
                dbText = "-inf";
            } else {
                dbText = juce::String(db, 1) + " dB";
            }
            gainValueLabel_.setText(dbText, juce::dontSendNotification);

            panSlider_.setValue(track->pan, juce::dontSendNotification);

            // Update pan label
            float pan = track->pan;
            juce::String panText;
            if (std::abs(pan) < 0.01f) {
                panText = "C";
            } else if (pan < 0) {
                panText = juce::String(static_cast<int>(std::abs(pan) * 100)) + "L";
            } else {
                panText = juce::String(static_cast<int>(pan * 100)) + "R";
            }
            panValueLabel_.setText(panText, juce::dontSendNotification);

            showTrackStrip(true);
            noSelectionLabel_.setVisible(false);
            rebuildDeviceSlots();
        } else {
            showTrackStrip(false);
            noSelectionLabel_.setVisible(true);
            deviceSlots_.clear();
        }
    }

    resized();
    repaint();
}

void TrackChainContent::showTrackStrip(bool show) {
    trackNameLabel_.setVisible(show);
    muteButton_.setVisible(show);
    soloButton_.setVisible(show);
    gainSlider_.setVisible(show);
    gainValueLabel_.setVisible(show);
    panSlider_.setVisible(show);
    panValueLabel_.setVisible(show);
}

void TrackChainContent::rebuildDeviceSlots() {
    // Remove existing slots
    deviceSlots_.clear();

    if (selectedTrackId_ == magda::INVALID_TRACK_ID) {
        return;
    }

    const auto* devices = magda::TrackManager::getInstance().getDevices(selectedTrackId_);
    if (!devices) {
        return;
    }

    // Create a slot component for each device
    for (const auto& device : *devices) {
        auto slot = std::make_unique<DeviceSlotComponent>(*this, selectedTrackId_, device);
        addAndMakeVisible(*slot);
        deviceSlots_.push_back(std::move(slot));
    }

    resized();
    repaint();
}

}  // namespace magda::daw::ui
