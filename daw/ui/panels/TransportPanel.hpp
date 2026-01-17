#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "../components/common/SvgButton.hpp"

namespace magica {

class TransportPanel : public juce::Component {
  public:
    TransportPanel();
    ~TransportPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Transport control callbacks
    std::function<void()> onPlay;
    std::function<void()> onStop;
    std::function<void()> onRecord;
    std::function<void()> onPause;
    std::function<void(bool)> onLoop;
    std::function<void(double)> onTempoChange;

    // Update displays
    void setPlayheadPosition(double positionInSeconds, int bars, int beats, int ticks);
    void setLoopLength(double lengthInSeconds, bool loopEnabled, bool useBarsBeats = false);
    void setTempo(double bpm);
    void setTimeSignature(int numerator, int denominator);

  private:
    // Transport controls (left section)
    std::unique_ptr<SvgButton> playButton;
    std::unique_ptr<SvgButton> stopButton;
    std::unique_ptr<SvgButton> recordButton;
    std::unique_ptr<SvgButton> pauseButton;
    std::unique_ptr<SvgButton> loopButton;

    // Time display (center section)
    std::unique_ptr<juce::Label> timeDisplay;
    std::unique_ptr<juce::Label> positionDisplay;
    std::unique_ptr<juce::Label> loopLengthDisplay;

    // Tempo and quantize (right section)
    std::unique_ptr<juce::Label> tempoDisplay;       // Main BPM display
    std::unique_ptr<SvgButton> tempoDecreaseButton;  // Minus button
    std::unique_ptr<SvgButton> tempoIncreaseButton;  // Plus button
    std::unique_ptr<juce::ComboBox> quantizeCombo;
    std::unique_ptr<SvgButton> metronomeButton;

    // Tempo editing
    void updateTempoDisplay();
    void adjustTempo(double delta);

    // Layout sections
    juce::Rectangle<int> getTransportControlsArea() const;
    juce::Rectangle<int> getTimeDisplayArea() const;
    juce::Rectangle<int> getTempoQuantizeArea() const;

    // Button styling
    void styleTransportButton(SvgButton& button, juce::Colour accentColor);
    void setupTransportButtons();
    void setupTimeDisplay();
    void setupTempoAndQuantize();

    // State
    bool isPlaying = false;
    bool isRecording = false;
    bool isPaused = false;
    bool isLooping = false;
    double currentTempo = 120.0;
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportPanel)
};

}  // namespace magica
