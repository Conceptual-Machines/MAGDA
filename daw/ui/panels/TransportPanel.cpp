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
    loopLengthDisplay->setBounds(timeArea.getX() + 250, timeY, 100, 30);

    // Tempo and quantize layout
    auto tempoY = tempoArea.getCentreY() - 15;
    auto tempoX = tempoArea.getX() + 10;

    // Tempo section: [-] [120.0] [+]
    tempoDecreaseButton->setBounds(tempoX, tempoY, 30, 30);
    tempoDisplay->setBounds(tempoX + 35, tempoY, 70, 30);
    tempoIncreaseButton->setBounds(tempoX + 110, tempoY, 30, 30);

    // Quantize and metronome
    quantizeCombo->setBounds(tempoX + 155, tempoY, 80, 30);
    metronomeButton->setBounds(tempoX + 245, tempoY, 40, 30);
}

juce::Rectangle<int> TransportPanel::getTransportControlsArea() const {
    return getLocalBounds().removeFromLeft(250);
}

juce::Rectangle<int> TransportPanel::getTimeDisplayArea() const {
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(250);         // Skip transport controls
    return bounds.removeFromLeft(360);  // Increased to fit loop length display
}

juce::Rectangle<int> TransportPanel::getTempoQuantizeArea() const {
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(610);  // Skip transport and time (250 + 360)
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

    // Loop length display
    loopLengthDisplay = std::make_unique<juce::Label>();
    loopLengthDisplay->setText("", juce::dontSendNotification);
    loopLengthDisplay->setFont(FontManager::getInstance().getUIFont(12.0f));
    loopLengthDisplay->setColour(juce::Label::textColourId,
                                 DarkTheme::getColour(DarkTheme::LOOP_MARKER));
    loopLengthDisplay->setColour(juce::Label::backgroundColourId,
                                 DarkTheme::getColour(DarkTheme::SURFACE));
    loopLengthDisplay->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*loopLengthDisplay);
}

void TransportPanel::setupTempoAndQuantize() {
    // Tempo decrease button (-)
    tempoDecreaseButton =
        std::make_unique<SvgButton>("Decrease", BinaryData::remove_svg, BinaryData::remove_svgSize);
    styleTransportButton(*tempoDecreaseButton, DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
    tempoDecreaseButton->onClick = [this]() { adjustTempo(-1.0); };
    addAndMakeVisible(*tempoDecreaseButton);

    // Tempo display (editable label)
    tempoDisplay = std::make_unique<juce::Label>();
    tempoDisplay->setFont(FontManager::getInstance().getTimeFont(18.0f));
    tempoDisplay->setColour(juce::Label::textColourId,
                            DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
    tempoDisplay->setColour(juce::Label::backgroundColourId,
                            DarkTheme::getColour(DarkTheme::SURFACE).darker(0.2f));
    tempoDisplay->setColour(juce::Label::outlineColourId, DarkTheme::getColour(DarkTheme::BORDER));
    tempoDisplay->setJustificationType(juce::Justification::centred);
    tempoDisplay->setEditable(true);
    tempoDisplay->onTextChange = [this]() {
        double newTempo = tempoDisplay->getText().getDoubleValue();
        if (newTempo >= 20.0 && newTempo <= 999.0) {
            currentTempo = newTempo;
            if (onTempoChange)
                onTempoChange(currentTempo);
        }
        updateTempoDisplay();  // Ensure display shows valid value
    };
    addAndMakeVisible(*tempoDisplay);
    updateTempoDisplay();

    // Tempo increase button (+)
    tempoIncreaseButton =
        std::make_unique<SvgButton>("Increase", BinaryData::add_svg, BinaryData::add_svgSize);
    styleTransportButton(*tempoIncreaseButton, DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
    tempoIncreaseButton->onClick = [this]() { adjustTempo(1.0); };
    addAndMakeVisible(*tempoIncreaseButton);

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
}

void TransportPanel::updateTempoDisplay() {
    tempoDisplay->setText(juce::String(currentTempo, 1), juce::dontSendNotification);
}

void TransportPanel::adjustTempo(double delta) {
    currentTempo = juce::jlimit(20.0, 999.0, currentTempo + delta);
    updateTempoDisplay();
    if (onTempoChange)
        onTempoChange(currentTempo);
}

void TransportPanel::styleTransportButton(SvgButton& button, juce::Colour accentColor) {
    button.setActiveColor(accentColor);
    button.setPressedColor(accentColor);
    button.setHoverColor(DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    button.setNormalColor(DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
}

void TransportPanel::setPlayheadPosition(double positionInSeconds, int bars, int beats, int ticks) {
    // Update bars:beats:ticks display
    juce::String bbtText = juce::String::formatted("%03d:%02d:%03d", bars, beats, ticks);
    timeDisplay->setText(bbtText, juce::dontSendNotification);

    // Update time display (minutes:seconds.milliseconds)
    int minutes = static_cast<int>(positionInSeconds) / 60;
    double seconds = std::fmod(positionInSeconds, 60.0);
    juce::String timeText = juce::String::formatted("%02d:%06.3f", minutes, seconds);
    positionDisplay->setText(timeText, juce::dontSendNotification);
}

void TransportPanel::setLoopLength(double lengthInSeconds, bool loopEnabled, bool useBarsBeats) {
    if (!loopEnabled || lengthInSeconds <= 0) {
        loopLengthDisplay->setText("", juce::dontSendNotification);
        return;
    }

    juce::String lengthText;

    if (useBarsBeats) {
        // Convert seconds to bars and beats
        double secondsPerBeat = 60.0 / currentTempo;
        double secondsPerBar = secondsPerBeat * timeSignatureNumerator;

        double totalBeats = lengthInSeconds / secondsPerBeat;
        int bars = static_cast<int>(totalBeats / timeSignatureNumerator);
        double remainingBeats = std::fmod(totalBeats, static_cast<double>(timeSignatureNumerator));
        int beats = static_cast<int>(remainingBeats);
        int ticks = static_cast<int>((remainingBeats - beats) * 960);  // 960 ticks per beat

        if (bars > 0) {
            lengthText = juce::String::formatted("L: %d.%d.%03d", bars, beats + 1, ticks);
        } else {
            lengthText = juce::String::formatted("L: %d.%03d", beats + 1, ticks);
        }
    } else {
        // Display in seconds format
        if (lengthInSeconds >= 60.0) {
            int minutes = static_cast<int>(lengthInSeconds) / 60;
            double seconds = std::fmod(lengthInSeconds, 60.0);
            lengthText = juce::String::formatted("L: %d:%04.1fs", minutes, seconds);
        } else if (lengthInSeconds >= 1.0) {
            lengthText = juce::String::formatted("L: %.2fs", lengthInSeconds);
        } else {
            lengthText =
                juce::String::formatted("L: %dms", static_cast<int>(lengthInSeconds * 1000));
        }
    }

    loopLengthDisplay->setText(lengthText, juce::dontSendNotification);
}

void TransportPanel::setTimeSignature(int numerator, int denominator) {
    timeSignatureNumerator = numerator;
    timeSignatureDenominator = denominator;
}

void TransportPanel::setTempo(double bpm) {
    currentTempo = juce::jlimit(20.0, 999.0, bpm);
    updateTempoDisplay();
}

}  // namespace magica
