#include "MixerView.hpp"

#include <cmath>

#include "../themes/DarkTheme.hpp"
#include "core/ViewModeController.hpp"

namespace magica {

// dB conversion helpers
namespace {
constexpr float MIN_DB = -60.0f;
constexpr float MAX_DB = 6.0f;
constexpr float UNITY_DB = 0.0f;

// Convert linear gain (0-1) to dB
float gainToDb(float gain) {
    if (gain <= 0.0f)
        return MIN_DB;
    return 20.0f * std::log10(gain);
}

// Convert dB to linear gain
float dbToGain(float db) {
    if (db <= MIN_DB)
        return 0.0f;
    return std::pow(10.0f, db / 20.0f);
}

// Convert dB to normalized fader position (0-1) with proper scaling
// Unity (0dB) at ~75% position
float dbToFaderPos(float db) {
    if (db <= MIN_DB)
        return 0.0f;
    if (db >= MAX_DB)
        return 1.0f;

    // Use a curve that puts 0dB at 0.75
    if (db < UNITY_DB) {
        // Below unity: map MIN_DB..0dB to 0..0.75
        return 0.75f * (db - MIN_DB) / (UNITY_DB - MIN_DB);
    } else {
        // Above unity: map 0dB..MAX_DB to 0.75..1.0
        return 0.75f + 0.25f * (db - UNITY_DB) / (MAX_DB - UNITY_DB);
    }
}

// Convert fader position to dB
float faderPosToDb(float pos) {
    if (pos <= 0.0f)
        return MIN_DB;
    if (pos >= 1.0f)
        return MAX_DB;

    if (pos < 0.75f) {
        // Below unity
        return MIN_DB + (pos / 0.75f) * (UNITY_DB - MIN_DB);
    } else {
        // Above unity
        return UNITY_DB + ((pos - 0.75f) / 0.25f) * (MAX_DB - UNITY_DB);
    }
}
}  // namespace

// Level meter component with dB labels
class MixerView::ChannelStrip::LevelMeter : public juce::Component {
  public:
    LevelMeter() = default;

    void setLevel(float newLevel) {
        level = juce::jlimit(0.0f, 1.0f, newLevel);
        repaint();
    }

    float getLevel() const {
        return level;
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds();

        // Reserve space for labels on the right
        auto labelWidth = 20;
        auto meterBounds = bounds.removeFromLeft(bounds.getWidth() - labelWidth).toFloat();
        auto labelBounds = bounds.toFloat();

        // Background
        g.setColour(DarkTheme::getColour(DarkTheme::SURFACE));
        g.fillRoundedRectangle(meterBounds, 2.0f);

        // Meter fill (using dB-scaled level)
        float meterHeight = meterBounds.getHeight() * level;
        auto fillBounds = meterBounds;
        fillBounds = fillBounds.removeFromBottom(meterHeight);

        // Gradient from green to yellow to red based on dB
        float dbLevel = gainToDb(level);
        if (dbLevel < -12.0f) {
            g.setColour(juce::Colour(0xFF55AA55));  // Green
        } else if (dbLevel < -3.0f) {
            g.setColour(juce::Colour(0xFFAAAA55));  // Yellow
        } else {
            g.setColour(juce::Colour(0xFFAA5555));  // Red
        }
        g.fillRoundedRectangle(fillBounds, 2.0f);

        // Draw dB labels
        g.setColour(DarkTheme::getColour(DarkTheme::TEXT_DIM));
        g.setFont(9.0f);

        // dB markings: +6, 0, -6, -12, -24, -48
        const float dbMarks[] = {6.0f, 0.0f, -6.0f, -12.0f, -24.0f, -48.0f};
        for (float db : dbMarks) {
            float pos = dbToFaderPos(db);
            float y = meterBounds.getBottom() - pos * meterBounds.getHeight();

            juce::String label;
            if (db > 0)
                label = "+" + juce::String((int)db);
            else if (db == 0)
                label = "0";
            else
                label = juce::String((int)db);

            g.drawText(label, labelBounds.getX(), y - 5, labelWidth, 10,
                       juce::Justification::centredLeft, false);

            // Small tick mark
            g.drawHorizontalLine((int)y, meterBounds.getRight() - 2, meterBounds.getRight());
        }
    }

  private:
    float level = 0.0f;
};

// Channel strip implementation
MixerView::ChannelStrip::ChannelStrip(const TrackInfo& track, juce::LookAndFeel* faderLookAndFeel,
                                      bool isMaster)
    : trackId_(track.id),
      isMaster_(isMaster),
      trackColour_(track.colour),
      trackName_(track.name),
      faderLookAndFeel_(faderLookAndFeel) {
    setupControls();
    updateFromTrack(track);
}

MixerView::ChannelStrip::~ChannelStrip() {
    // Clear look and feel before destruction to avoid dangling pointer issues
    if (volumeFader) {
        volumeFader->setLookAndFeel(nullptr);
    }
    if (panKnob) {
        panKnob->setLookAndFeel(nullptr);
    }
}

void MixerView::ChannelStrip::updateFromTrack(const TrackInfo& track) {
    trackColour_ = track.colour;
    trackName_ = track.name;

    if (trackLabel) {
        trackLabel->setText(isMaster_ ? "Master" : track.name, juce::dontSendNotification);
    }
    if (volumeFader) {
        // Convert linear gain to fader position
        float db = gainToDb(track.volume);
        float faderPos = dbToFaderPos(db);
        volumeFader->setValue(faderPos, juce::dontSendNotification);
    }
    if (panKnob) {
        panKnob->setValue(track.pan, juce::dontSendNotification);
    }
    if (muteButton) {
        muteButton->setToggleState(track.muted, juce::dontSendNotification);
    }
    if (soloButton) {
        soloButton->setToggleState(track.soloed, juce::dontSendNotification);
    }
    if (recordButton) {
        recordButton->setToggleState(track.recordArmed, juce::dontSendNotification);
    }

    repaint();
}

void MixerView::ChannelStrip::setupControls() {
    // Track label
    trackLabel = std::make_unique<juce::Label>();
    trackLabel->setText(isMaster_ ? "Master" : trackName_, juce::dontSendNotification);
    trackLabel->setJustificationType(juce::Justification::centred);
    trackLabel->setColour(juce::Label::textColourId, DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    trackLabel->setColour(juce::Label::backgroundColourId,
                          DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));
    addAndMakeVisible(*trackLabel);

    // Pan knob
    panKnob = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                             juce::Slider::NoTextBox);
    panKnob->setRange(-1.0, 1.0, 0.01);
    panKnob->setValue(0.0);
    panKnob->setColour(juce::Slider::rotarySliderFillColourId,
                       DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    panKnob->setColour(juce::Slider::rotarySliderOutlineColourId,
                       DarkTheme::getColour(DarkTheme::SURFACE));
    panKnob->setColour(juce::Slider::thumbColourId, DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    panKnob->onValueChange = [this]() {
        TrackManager::getInstance().setTrackPan(trackId_, static_cast<float>(panKnob->getValue()));
    };
    // Apply custom look and feel for knob styling
    if (faderLookAndFeel_) {
        panKnob->setLookAndFeel(faderLookAndFeel_);
    }
    addAndMakeVisible(*panKnob);

    // Level meter
    levelMeter = std::make_unique<LevelMeter>();
    addAndMakeVisible(*levelMeter);

    // Volume fader - using dB scale with unity at 0.75 position
    volumeFader =
        std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::NoTextBox);
    volumeFader->setRange(0.0, 1.0, 0.001);             // Internal 0-1 range
    volumeFader->setValue(0.75);                        // Unity gain (0 dB) at 75%
    volumeFader->setSliderSnapsToMousePosition(false);  // Relative drag, not jump to click
    volumeFader->setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    volumeFader->setColour(juce::Slider::backgroundColourId,
                           DarkTheme::getColour(DarkTheme::SURFACE));
    volumeFader->setColour(juce::Slider::thumbColourId,
                           DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    volumeFader->onValueChange = [this]() {
        // Convert fader position to dB, then to linear gain for TrackManager
        float faderPos = static_cast<float>(volumeFader->getValue());
        float db = faderPosToDb(faderPos);
        float gain = dbToGain(db);
        TrackManager::getInstance().setTrackVolume(trackId_, gain);
    };
    // Apply custom look and feel for fader styling
    if (faderLookAndFeel_) {
        volumeFader->setLookAndFeel(faderLookAndFeel_);
    }
    addAndMakeVisible(*volumeFader);

    // Mute button
    muteButton = std::make_unique<juce::TextButton>("M");
    muteButton->setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
    muteButton->setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(0xFFAA8855));  // Orange when active
    muteButton->setColour(juce::TextButton::textColourOffId,
                          DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    muteButton->setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    muteButton->setClickingTogglesState(true);
    muteButton->onClick = [this]() {
        TrackManager::getInstance().setTrackMuted(trackId_, muteButton->getToggleState());
    };
    addAndMakeVisible(*muteButton);

    // Solo button
    soloButton = std::make_unique<juce::TextButton>("S");
    soloButton->setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
    soloButton->setColour(juce::TextButton::buttonOnColourId,
                          juce::Colour(0xFFAAAA55));  // Yellow when active
    soloButton->setColour(juce::TextButton::textColourOffId,
                          DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    soloButton->setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    soloButton->setClickingTogglesState(true);
    soloButton->onClick = [this]() {
        TrackManager::getInstance().setTrackSoloed(trackId_, soloButton->getToggleState());
    };
    addAndMakeVisible(*soloButton);

    // Record arm button (not on master)
    if (!isMaster_) {
        recordButton = std::make_unique<juce::TextButton>("R");
        recordButton->setColour(juce::TextButton::buttonColourId,
                                DarkTheme::getColour(DarkTheme::BUTTON_NORMAL));
        recordButton->setColour(juce::TextButton::buttonOnColourId,
                                DarkTheme::getColour(DarkTheme::STATUS_ERROR));  // Red when armed
        recordButton->setColour(juce::TextButton::textColourOffId,
                                DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
        recordButton->setColour(juce::TextButton::textColourOnId,
                                DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
        recordButton->setClickingTogglesState(true);
        recordButton->onClick = [this]() {
            TrackManager::getInstance().setTrackRecordArmed(trackId_,
                                                            recordButton->getToggleState());
        };
        addAndMakeVisible(*recordButton);
    }
}

void MixerView::ChannelStrip::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();

    // Background - slightly brighter if selected
    if (selected) {
        g.setColour(DarkTheme::getColour(DarkTheme::SURFACE));
    } else {
        g.setColour(DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));
    }
    g.fillRect(bounds);

    // Selection border
    if (selected) {
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.drawRect(bounds, 2);
    }

    // Border on right side (separator) - only if not selected
    if (!selected) {
        g.setColour(DarkTheme::getColour(DarkTheme::SEPARATOR));
        g.fillRect(bounds.getRight() - 1, 0, 1, bounds.getHeight());
    }

    // Channel color indicator at top
    if (!isMaster_) {
        g.setColour(trackColour_);
        g.fillRect(selected ? 2 : 0, selected ? 2 : 0, getWidth() - (selected ? 3 : 1), 4);
    } else {
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.fillRect(selected ? 2 : 0, selected ? 2 : 0, getWidth() - (selected ? 3 : 1), 4);
    }
}

void MixerView::ChannelStrip::resized() {
    auto bounds = getLocalBounds().reduced(4);

    // Color indicator space
    bounds.removeFromTop(6);

    // Track label at top
    trackLabel->setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(4);

    // Pan knob
    auto panArea = bounds.removeFromTop(KNOB_SIZE);
    panKnob->setBounds(panArea.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE));
    bounds.removeFromTop(4);

    // Buttons at bottom
    auto buttonArea = bounds.removeFromBottom(BUTTON_SIZE);
    int numButtons = isMaster_ ? 2 : 3;
    int buttonWidth = (buttonArea.getWidth() - (numButtons - 1) * 2) / numButtons;

    muteButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(2);
    soloButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
    if (recordButton) {
        buttonArea.removeFromLeft(2);
        recordButton->setBounds(buttonArea.removeFromLeft(buttonWidth));
    }

    bounds.removeFromBottom(4);

    // Fader and meter in remaining space
    int faderWidth = 24;
    int meterWidth = METER_WIDTH + 22;  // Extra space for dB labels
    int totalWidth = faderWidth + 4 + meterWidth;

    auto faderMeterArea = bounds;

    // Meter on left (with labels)
    levelMeter->setBounds(faderMeterArea.removeFromLeft(meterWidth));
    faderMeterArea.removeFromLeft(4);

    // Fader takes remaining space
    volumeFader->setBounds(faderMeterArea.removeFromLeft(faderWidth));
}

void MixerView::ChannelStrip::setMeterLevel(float level) {
    meterLevel = level;
    if (levelMeter) {
        levelMeter->setLevel(level);
    }
}

void MixerView::ChannelStrip::setSelected(bool shouldBeSelected) {
    if (selected != shouldBeSelected) {
        selected = shouldBeSelected;
        repaint();
    }
}

void MixerView::ChannelStrip::mouseDown(const juce::MouseEvent& /*event*/) {
    if (onClicked) {
        onClicked(trackId_, isMaster_);
    }
}

// MixerView implementation
MixerView::MixerView() {
    // Get current view mode
    currentViewMode_ = ViewModeController::getInstance().getViewMode();

    // Create channel container
    channelContainer = std::make_unique<juce::Component>();

    // Create viewport for scrollable channels
    channelViewport = std::make_unique<juce::Viewport>();
    channelViewport->setViewedComponent(channelContainer.get(), false);
    channelViewport->setScrollBarsShown(false, true);  // Horizontal scroll only
    addAndMakeVisible(*channelViewport);

    // Create master strip (uses shared MasterChannelStrip component)
    masterStrip = std::make_unique<MasterChannelStrip>(MasterChannelStrip::Orientation::Vertical);
    addAndMakeVisible(*masterStrip);

    // Register as TrackManager listener
    TrackManager::getInstance().addListener(this);

    // Register as ViewModeController listener
    ViewModeController::getInstance().addListener(this);

    // Build channel strips from TrackManager
    rebuildChannelStrips();
}

MixerView::~MixerView() {
    TrackManager::getInstance().removeListener(this);
    ViewModeController::getInstance().removeListener(this);
}

void MixerView::rebuildChannelStrips() {
    // Clear existing strips
    channelStrips.clear();

    const auto& tracks = TrackManager::getInstance().getTracks();

    for (const auto& track : tracks) {
        // Only show tracks visible in the current view mode
        if (!track.isVisibleIn(currentViewMode_)) {
            continue;
        }

        auto strip = std::make_unique<ChannelStrip>(track, &mixerLookAndFeel_, false);
        strip->onClicked = [this](int trackId, bool isMaster) {
            // Find the index of this track in the visible strips
            for (size_t i = 0; i < channelStrips.size(); ++i) {
                if (channelStrips[i]->getTrackId() == trackId) {
                    selectChannel(static_cast<int>(i), isMaster);
                    break;
                }
            }
        };
        channelContainer->addAndMakeVisible(*strip);
        channelStrips.push_back(std::move(strip));
    }

    // Update master strip visibility
    const auto& master = TrackManager::getInstance().getMasterChannel();
    bool masterVisible = master.isVisibleIn(currentViewMode_);
    masterStrip->setVisible(masterVisible);

    // Sync selection with TrackManager's current selection
    trackSelectionChanged(TrackManager::getInstance().getSelectedTrack());

    resized();
}

void MixerView::tracksChanged() {
    // Rebuild all channel strips when tracks are added/removed/reordered
    rebuildChannelStrips();
}

void MixerView::trackPropertyChanged(int trackId) {
    // Update the specific channel strip - find it by track ID since indices may differ
    const auto* track = TrackManager::getInstance().getTrack(trackId);
    if (!track)
        return;

    for (auto& strip : channelStrips) {
        if (strip->getTrackId() == trackId) {
            strip->updateFromTrack(*track);
            break;
        }
    }
}

void MixerView::viewModeChanged(ViewMode mode, const AudioEngineProfile& /*profile*/) {
    currentViewMode_ = mode;
    rebuildChannelStrips();
}

void MixerView::masterChannelChanged() {
    // Update master strip visibility
    const auto& master = TrackManager::getInstance().getMasterChannel();
    bool masterVisible = master.isVisibleIn(currentViewMode_);
    masterStrip->setVisible(masterVisible);
    resized();
}

void MixerView::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::BACKGROUND));
}

void MixerView::resized() {
    auto bounds = getLocalBounds();

    // Master strip on the right (only if visible)
    if (masterStrip->isVisible()) {
        masterStrip->setBounds(bounds.removeFromRight(MASTER_WIDTH));
        // Separator between channels and master
        bounds.removeFromRight(2);
    }

    // Channel viewport takes remaining space
    channelViewport->setBounds(bounds);

    // Size the channel container
    int numChannels = static_cast<int>(channelStrips.size());
    int containerWidth = numChannels * CHANNEL_WIDTH;
    int containerHeight = bounds.getHeight();
    channelContainer->setSize(containerWidth, containerHeight);

    // Position channel strips
    for (int i = 0; i < numChannels; ++i) {
        channelStrips[i]->setBounds(i * CHANNEL_WIDTH, 0, CHANNEL_WIDTH, containerHeight);
    }
}

void MixerView::timerCallback() {
    // Timer callback - meters will be driven by actual audio engine in future
}

void MixerView::selectChannel(int index, bool isMaster) {
    // Deselect all channel strips
    for (auto& strip : channelStrips) {
        strip->setSelected(false);
    }

    // Select the clicked channel
    if (isMaster) {
        // Master strip selection is visual-only (no setSelected on MasterChannelStrip)
        selectedChannelIndex = -1;
        selectedIsMaster = true;
        // Master track doesn't have a TrackId, so we clear selection
        TrackManager::getInstance().setSelectedTrack(INVALID_TRACK_ID);
    } else {
        if (index >= 0 && index < static_cast<int>(channelStrips.size())) {
            channelStrips[index]->setSelected(true);
            // Notify TrackManager of selection
            TrackManager::getInstance().setSelectedTrack(channelStrips[index]->getTrackId());
        }
        selectedChannelIndex = index;
        selectedIsMaster = false;
    }

    // Notify listener
    if (onChannelSelected) {
        onChannelSelected(selectedChannelIndex, selectedIsMaster);
    }

    DBG("Selected channel: " << (isMaster ? "Master" : juce::String(index + 1)));
}

void MixerView::trackSelectionChanged(TrackId trackId) {
    // Sync our visual selection with TrackManager's selection
    // Deselect all first
    for (auto& strip : channelStrips) {
        strip->setSelected(false);
    }
    selectedIsMaster = false;
    selectedChannelIndex = -1;

    if (trackId == INVALID_TRACK_ID) {
        return;
    }

    // Find and select the matching channel strip
    for (size_t i = 0; i < channelStrips.size(); ++i) {
        if (channelStrips[i]->getTrackId() == trackId) {
            channelStrips[i]->setSelected(true);
            selectedChannelIndex = static_cast<int>(i);
            break;
        }
    }
}

}  // namespace magica
