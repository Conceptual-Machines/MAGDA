#include "TrackChainContent.hpp"

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"

namespace magica::daw::ui {

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
        if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
            magica::TrackManager::getInstance().setTrackMuted(selectedTrackId_,
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
        if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
            magica::TrackManager::getInstance().setTrackSoloed(selectedTrackId_,
                                                               soloButton_.getToggleState());
        }
    };
    addChildComponent(soloButton_);

    // Gain slider
    gainSlider_.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainSlider_.setRange(0.0, 1.0, 0.01);
    gainSlider_.setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    gainSlider_.setColour(juce::Slider::thumbColourId,
                          DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    gainSlider_.onValueChange = [this]() {
        if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
            magica::TrackManager::getInstance().setTrackVolume(
                selectedTrackId_, static_cast<float>(gainSlider_.getValue()));
        }
    };
    addChildComponent(gainSlider_);

    // Pan slider
    panSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider_.setRange(-1.0, 1.0, 0.01);
    panSlider_.setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    panSlider_.setColour(juce::Slider::thumbColourId, DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    panSlider_.onValueChange = [this]() {
        if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
            magica::TrackManager::getInstance().setTrackPan(
                selectedTrackId_, static_cast<float>(panSlider_.getValue()));
        }
    };
    addChildComponent(panSlider_);

    // Register as listener
    magica::TrackManager::getInstance().addListener(this);

    // Check if there's already a selected track
    selectedTrackId_ = magica::TrackManager::getInstance().getSelectedTrack();
    updateFromSelectedTrack();
}

TrackChainContent::~TrackChainContent() {
    magica::TrackManager::getInstance().removeListener(this);
}

void TrackChainContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());

    if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
        // Draw the chain mockup area (horizontal layout)
        auto bounds = getLocalBounds();
        auto stripWidth = 100;
        auto chainArea = bounds.withTrimmedRight(stripWidth);

        paintChainMockup(g, chainArea);

        // Draw separator line before strip
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawLine(static_cast<float>(chainArea.getRight()), 0.0f,
                   static_cast<float>(chainArea.getRight()), static_cast<float>(getHeight()), 1.0f);
    }
}

void TrackChainContent::paintChainMockup(juce::Graphics& g, juce::Rectangle<int> area) {
    // Draw mockup FX chain slots horizontally
    auto slotArea = area.reduced(8);
    int slotWidth = 120;
    int slotSpacing = 8;
    int arrowWidth = 20;

    // Draw empty FX slots horizontally
    juce::StringArray slotLabels = {"Input", "Insert 1", "Insert 2", "Insert 3", "Send"};
    for (size_t i = 0; i < slotLabels.size(); ++i) {
        if (slotArea.getWidth() < slotWidth)
            break;

        auto slot = slotArea.removeFromLeft(slotWidth);

        // Slot background
        g.setColour(DarkTheme::getColour(DarkTheme::SURFACE));
        g.fillRoundedRectangle(slot.toFloat(), 4.0f);

        // Slot border
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
        g.drawRoundedRectangle(slot.toFloat(), 4.0f, 1.0f);

        // Slot label at top
        g.setColour(DarkTheme::getSecondaryTextColour());
        g.setFont(FontManager::getInstance().getUIFont(10.0f));
        g.drawText(slotLabels[static_cast<int>(i)], slot.reduced(6).removeFromTop(16),
                   juce::Justification::centredLeft);

        // "(empty)" hint centered
        g.setColour(DarkTheme::getSecondaryTextColour().withAlpha(0.5f));
        g.setFont(FontManager::getInstance().getUIFont(9.0f));
        g.drawText("(empty)", slot, juce::Justification::centred);

        // Draw arrow between slots (except after last)
        if (i < slotLabels.size() - 1) {
            auto arrowArea = slotArea.removeFromLeft(arrowWidth);
            g.setColour(DarkTheme::getSecondaryTextColour());

            // Draw arrow: →
            int arrowY = arrowArea.getCentreY();
            int arrowX = arrowArea.getCentreX();
            g.drawLine(static_cast<float>(arrowX - 6), static_cast<float>(arrowY),
                       static_cast<float>(arrowX + 6), static_cast<float>(arrowY), 1.5f);
            // Arrow head
            g.drawLine(static_cast<float>(arrowX + 2), static_cast<float>(arrowY - 4),
                       static_cast<float>(arrowX + 6), static_cast<float>(arrowY), 1.5f);
            g.drawLine(static_cast<float>(arrowX + 2), static_cast<float>(arrowY + 4),
                       static_cast<float>(arrowX + 6), static_cast<float>(arrowY), 1.5f);

            slotArea.removeFromLeft(slotSpacing);
        }
    }
}

void TrackChainContent::resized() {
    auto bounds = getLocalBounds();

    if (selectedTrackId_ == magica::INVALID_TRACK_ID) {
        noSelectionLabel_.setBounds(bounds);
    } else {
        // Track info strip at right border
        auto stripWidth = 80;
        auto strip = bounds.removeFromRight(stripWidth).reduced(4);

        // Track name at top
        trackNameLabel_.setBounds(strip.removeFromTop(20));
        strip.removeFromTop(8);

        // M/S buttons
        auto buttonRow = strip.removeFromTop(24);
        muteButton_.setBounds(buttonRow.removeFromLeft(32));
        buttonRow.removeFromLeft(4);
        soloButton_.setBounds(buttonRow.removeFromLeft(32));
        strip.removeFromTop(8);

        // Gain slider (vertical)
        gainSlider_.setBounds(strip.removeFromTop(80));
        strip.removeFromTop(8);

        // Pan slider (horizontal)
        panSlider_.setBounds(strip.removeFromTop(20));
    }
}

void TrackChainContent::onActivated() {
    selectedTrackId_ = magica::TrackManager::getInstance().getSelectedTrack();
    updateFromSelectedTrack();
}

void TrackChainContent::onDeactivated() {
    // Nothing to do
}

void TrackChainContent::tracksChanged() {
    if (selectedTrackId_ != magica::INVALID_TRACK_ID) {
        const auto* track = magica::TrackManager::getInstance().getTrack(selectedTrackId_);
        if (!track) {
            selectedTrackId_ = magica::INVALID_TRACK_ID;
            updateFromSelectedTrack();
        }
    }
}

void TrackChainContent::trackPropertyChanged(int trackId) {
    if (static_cast<magica::TrackId>(trackId) == selectedTrackId_) {
        updateFromSelectedTrack();
    }
}

void TrackChainContent::trackSelectionChanged(magica::TrackId trackId) {
    selectedTrackId_ = trackId;
    updateFromSelectedTrack();
}

void TrackChainContent::updateFromSelectedTrack() {
    if (selectedTrackId_ == magica::INVALID_TRACK_ID) {
        showTrackStrip(false);
        noSelectionLabel_.setVisible(true);
    } else {
        const auto* track = magica::TrackManager::getInstance().getTrack(selectedTrackId_);
        if (track) {
            trackNameLabel_.setText(track->name, juce::dontSendNotification);
            muteButton_.setToggleState(track->muted, juce::dontSendNotification);
            soloButton_.setToggleState(track->soloed, juce::dontSendNotification);
            gainSlider_.setValue(track->volume, juce::dontSendNotification);
            panSlider_.setValue(track->pan, juce::dontSendNotification);

            showTrackStrip(true);
            noSelectionLabel_.setVisible(false);
        } else {
            showTrackStrip(false);
            noSelectionLabel_.setVisible(true);
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
    panSlider_.setVisible(show);
}

}  // namespace magica::daw::ui
