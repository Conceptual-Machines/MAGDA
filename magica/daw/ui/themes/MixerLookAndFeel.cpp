#include "MixerLookAndFeel.hpp"

#include "BinaryData.h"
#include "DarkTheme.hpp"

namespace magica {

MixerLookAndFeel::MixerLookAndFeel() {
    loadIcons();

    // Set default slider colors
    setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    setColour(juce::Slider::backgroundColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    setColour(juce::Slider::thumbColourId, DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
}

void MixerLookAndFeel::loadIcons() {
    // Load fader thumb SVG
    faderThumb_ = juce::Drawable::createFromImageData(BinaryData::fader_thumb_svg,
                                                      BinaryData::fader_thumb_svgSize);

    // Load fader track SVG
    faderTrack_ = juce::Drawable::createFromImageData(BinaryData::fader_track_svg,
                                                      BinaryData::fader_track_svgSize);

    // Load knob body SVG
    knobBody_ = juce::Drawable::createFromImageData(BinaryData::knob_body_svg,
                                                    BinaryData::knob_body_svgSize);

    // Load knob pointer SVG
    knobPointer_ = juce::Drawable::createFromImageData(BinaryData::knob_pointer_svg,
                                                       BinaryData::knob_pointer_svgSize);
}

void MixerLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        const juce::Slider::SliderStyle style,
                                        juce::Slider& slider) {
    // Only customize vertical sliders (faders)
    if (style != juce::Slider::LinearVertical) {
        // Use default for other styles
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos,
                                         maxSliderPos, style, slider);
        return;
    }

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

    // Get slider colors
    auto trackColour = slider.findColour(juce::Slider::trackColourId);
    auto thumbColour = slider.findColour(juce::Slider::thumbColourId);

    // Constants for fader sizing
    const float trackWidth = 2.0f;
    const float thumbWidth = 24.0f;
    const float thumbHeight = 12.0f;

    // Draw track (thin vertical line)
    float trackX = bounds.getCentreX() - trackWidth / 2.0f;
    auto trackRect = juce::Rectangle<float>(trackX, bounds.getY(), trackWidth, bounds.getHeight());

    if (faderTrack_) {
        // Scale and draw the SVG track
        faderTrack_->setTransformToFit(trackRect, juce::RectanglePlacement::stretchToFit);
        faderTrack_->draw(g, 1.0f);
    } else {
        // Fallback: draw simple line
        g.setColour(trackColour);
        g.fillRoundedRectangle(trackRect, 1.0f);
    }

    // Draw filled portion below the thumb (value indicator)
    float thumbY = sliderPos - thumbHeight / 2.0f;
    auto filledTrackRect =
        juce::Rectangle<float>(trackX, thumbY + thumbHeight / 2.0f, trackWidth,
                               bounds.getBottom() - (thumbY + thumbHeight / 2.0f));
    g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE).withAlpha(0.6f));
    g.fillRoundedRectangle(filledTrackRect, 1.0f);

    // Draw thumb - simple rounded rectangle matching knob style
    float thumbX = bounds.getCentreX() - thumbWidth / 2.0f;
    auto thumbRect = juce::Rectangle<float>(thumbX, thumbY, thumbWidth, thumbHeight);

    // Fill
    g.setColour(DarkTheme::getColour(DarkTheme::SURFACE));
    g.fillRoundedRectangle(thumbRect, thumbHeight / 2.0f);

    // Center line indicator
    g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    float lineY = thumbY + thumbHeight / 2.0f;
    g.drawLine(thumbX + 6.0f, lineY, thumbX + thumbWidth - 6.0f, lineY, 2.0f);
}

int MixerLookAndFeel::getSliderThumbRadius(juce::Slider& slider) {
    // Return half the thumb height for proper mouse interaction
    return slider.isVertical() ? 6 : LookAndFeel_V4::getSliderThumbRadius(slider);
}

void MixerLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& /*slider*/) {
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f * 0.7f;

    // Circle
    g.setColour(DarkTheme::getColour(DarkTheme::SURFACE));
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // Pointer line with rounded corners
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineLength = radius * 0.5f;
    auto lineWidth = 3.0f;
    auto lineStartRadius = radius * 0.2f;

    juce::Path pointerPath;
    pointerPath.addRoundedRectangle(-lineWidth / 2.0f, -radius + 4.0f, lineWidth, lineLength,
                                    lineWidth / 2.0f);

    g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
    g.fillPath(pointerPath, juce::AffineTransform::rotation(angle).translated(centreX, centreY));
}

}  // namespace magica
