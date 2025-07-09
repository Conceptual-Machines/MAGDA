#include "TransportPanel.hpp"

#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"
#include "BinaryData.h"

namespace magica {

TransportPanel::TransportPanel() {
    setupTransportButtons();
    setupTimeDisplay();
    setupTempoAndQuantize();
}

TransportPanel::~TransportPanel() = default;

void TransportPanel::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::TRANSPORT_BACKGROUND));

    // Draw subtle borders between sections
    g.setColour(DarkTheme::getColour(DarkTheme::SEPARATOR));

    auto bounds = getLocalBounds();
    auto transportArea = getTransportControlsArea();
    auto timeArea = getTimeDisplayArea();

    // Vertical separators
    g.drawVerticalLine(transportArea.getRight(), bounds.getY(), bounds.getBottom());
    g.drawVerticalLine(timeArea.getRight(), bounds.getY(), bounds.getBottom());
}

void TransportPanel::resized() {
    auto transportArea = getTransportControlsArea();
    auto timeArea = getTimeDisplayArea();
    auto tempoArea = getTempoQuantizeArea();

    // Transport controls layout
    auto buttonWidth = 40;
    auto buttonHeight = 30;
    auto buttonY = transportArea.getCentreY() - buttonHeight / 2;
    auto buttonSpacing = 5;

    auto x = transportArea.getX() + 10;

    playButton->setBounds(x, buttonY, buttonWidth, buttonHeight);
    x += buttonWidth + buttonSpacing;

    stopButton->setBounds(x, buttonY, buttonWidth, buttonHeight);
    x += buttonWidth + buttonSpacing;

    recordButton->setBounds(x, buttonY, buttonWidth, buttonHeight);
    x += buttonWidth + buttonSpacing;

    pauseButton->setBounds(x, buttonY, buttonWidth, buttonHeight);
    x += buttonWidth + buttonSpacing + 10;

    loopButton->setBounds(x, buttonY, buttonWidth, buttonHeight);

    // Time display layout
    auto timeY = timeArea.getCentreY() - 15;
    timeDisplay->setBounds(timeArea.getX() + 10, timeY, 120, 30);
    positionDisplay->setBounds(timeArea.getX() + 140, timeY, 100, 30);

    // Tempo and quantize layout
    auto tempoY = tempoArea.getCentreY() - 15;
    auto tempoX = tempoArea.getX() + 10;

    tempoLabel->setBounds(tempoX, tempoY, 50, 30);
    tempoSlider->setBounds(tempoX + 55, tempoY, 80, 30);
    quantizeCombo->setBounds(tempoX + 145, tempoY, 80, 30);
    metronomeButton->setBounds(tempoX + 235, tempoY, 60, 30);
    clickButton->setBounds(tempoX + 305, tempoY, 50, 30);
}

juce::Rectangle<int> TransportPanel::getTransportControlsArea() const {
    return getLocalBounds().removeFromLeft(250);
}

juce::Rectangle<int> TransportPanel::getTimeDisplayArea() const {
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(250);  // Skip transport controls
    return bounds.removeFromLeft(250);
}

juce::Rectangle<int> TransportPanel::getTempoQuantizeArea() const {
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(500);  // Skip transport and time
    return bounds;
}

void TransportPanel::setupTransportButtons() {
    // Play button
    playButton =
        std::make_unique<SvgButton>("Play", BinaryData::play_svg, BinaryData::play_svgSize);
    styleTransportButton(*playButton, DarkTheme::getColour(DarkTheme::ACCENT_GREEN));
    playButton->onClick = [this]() {
        isPlaying = !isPlaying;
        if (isPlaying) {
            isPaused = false;
            if (onPlay)
                onPlay();
        } else {
            if (onStop)
                onStop();
        }
        playButton->setActive(isPlaying);
        repaint();
    };
    addAndMakeVisible(*playButton);

    // Stop button
    stopButton =
        std::make_unique<SvgButton>("Stop", BinaryData::stop_svg, BinaryData::stop_svgSize);
    styleTransportButton(*stopButton, DarkTheme::getColour(DarkTheme::ACCENT_RED));
    stopButton->onClick = [this]() {
        isPlaying = false;
        isPaused = false;
        isRecording = false;
        playButton->setActive(false);
        recordButton->setActive(false);
        if (onStop)
            onStop();
        repaint();
    };
    addAndMakeVisible(*stopButton);

    // Record button
    recordButton =
        std::make_unique<SvgButton>("Record", BinaryData::record_svg, BinaryData::record_svgSize);
    styleTransportButton(*recordButton, DarkTheme::getColour(DarkTheme::ACCENT_RED));
    recordButton->onClick = [this]() {
        isRecording = !isRecording;
        recordButton->setActive(isRecording);
        if (isRecording && onRecord) {
            onRecord();
        }
        repaint();
    };
    addAndMakeVisible(*recordButton);

    // Pause button
    pauseButton =
        std::make_unique<SvgButton>("Pause", BinaryData::pause_svg, BinaryData::pause_svgSize);
    styleTransportButton(*pauseButton, DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
    pauseButton->onClick = [this]() {
        if (isPlaying) {
            isPaused = !isPaused;
            pauseButton->setActive(isPaused);
            if (onPause)
                onPause();
        }
        repaint();
    };
    addAndMakeVisible(*pauseButton);

    // Loop button
    loopButton =
        std::make_unique<SvgButton>("Loop", BinaryData::loop_svg, BinaryData::loop_svgSize);
    styleTransportButton(*loopButton, DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    loopButton->onClick = [this]() {
        isLooping = !isLooping;
        loopButton->setActive(isLooping);
        if (onLoop)
            onLoop(isLooping);
    };
    addAndMakeVisible(*loopButton);
}

void TransportPanel::setupTimeDisplay() {
    // Time display (bars:beats:ticks)
    timeDisplay = std::make_unique<juce::Label>();
    timeDisplay->setText("001:01:000", juce::dontSendNotification);
    timeDisplay->setFont(FontManager::getInstance().getTimeFont(16.0f));
    timeDisplay->setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    timeDisplay->setColour(juce::Label::backgroundColourId,
                           DarkTheme::getColour(DarkTheme::SURFACE));
    timeDisplay->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*timeDisplay);

    // Position display (time)
    positionDisplay = std::make_unique<juce::Label>();
    positionDisplay->setText("00:00.000", juce::dontSendNotification);
    positionDisplay->setFont(FontManager::getInstance().getUIFont(14.0f));
    positionDisplay->setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    positionDisplay->setColour(juce::Label::backgroundColourId,
                               DarkTheme::getColour(DarkTheme::SURFACE));
    positionDisplay->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*positionDisplay);
}

void TransportPanel::setupTempoAndQuantize() {
    // Tempo label
    tempoLabel = std::make_unique<juce::Label>();
    tempoLabel->setText("BPM:", juce::dontSendNotification);
    tempoLabel->setColour(juce::Label::textColourId, DarkTheme::getTextColour());
    tempoLabel->setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(*tempoLabel);

    // Tempo slider
    tempoSlider = std::make_unique<juce::Slider>();
    tempoSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    tempoSlider->setRange(60.0, 200.0, 1.0);
    tempoSlider->setValue(currentTempo);
    tempoSlider->onValueChange = [this]() {
        currentTempo = tempoSlider->getValue();
        if (onTempoChange)
            onTempoChange(currentTempo);
    };
    addAndMakeVisible(*tempoSlider);

    // Quantize combo
    quantizeCombo = std::make_unique<juce::ComboBox>();
    quantizeCombo->addItem("Off", 1);
    quantizeCombo->addItem("1/4", 2);
    quantizeCombo->addItem("1/8", 3);
    quantizeCombo->addItem("1/16", 4);
    quantizeCombo->addItem("1/32", 5);
    quantizeCombo->setSelectedId(2);  // Default to 1/4 note
    addAndMakeVisible(*quantizeCombo);

    // Metronome button
    metronomeButton = std::make_unique<SvgButton>("Metronome", BinaryData::volume_up_svg,
                                                  BinaryData::volume_up_svgSize);
    styleTransportButton(*metronomeButton, DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    metronomeButton->onClick = [this]() {
        bool newState = !metronomeButton->isActive();
        metronomeButton->setActive(newState);
    };
    addAndMakeVisible(*metronomeButton);

    // Click button
    clickButton = std::make_unique<SvgButton>("Click", BinaryData::volume_up_svg,
                                              BinaryData::volume_up_svgSize);
    styleTransportButton(*clickButton, DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    clickButton->onClick = [this]() {
        bool newState = !clickButton->isActive();
        clickButton->setActive(newState);
    };
    addAndMakeVisible(*clickButton);
}

void TransportPanel::styleTransportButton(SvgButton& button, juce::Colour accentColor) {
    button.setActiveColor(accentColor);
    button.setPressedColor(accentColor);
    button.setHoverColor(DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    button.setNormalColor(DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
}

}  // namespace magica