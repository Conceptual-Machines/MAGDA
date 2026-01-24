#include "TensionHandleComponent.hpp"

namespace magda {

TensionHandleComponent::TensionHandleComponent(AutomationPointId pointId) : pointId_(pointId) {
    setSize(HANDLE_SIZE, HANDLE_SIZE);
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
}

void TensionHandleComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    // Background - diamond shape
    juce::Path diamond;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float hw = bounds.getWidth() / 2.0f;
    float hh = bounds.getHeight() / 2.0f;

    diamond.startNewSubPath(cx, cy - hh);  // Top
    diamond.lineTo(cx + hw, cy);           // Right
    diamond.lineTo(cx, cy + hh);           // Bottom
    diamond.lineTo(cx - hw, cy);           // Left
    diamond.closeSubPath();

    // Fill color based on state
    juce::Colour fillColour;
    if (isDragging_) {
        fillColour = juce::Colour(0xFFFFAA44);  // Orange when dragging
    } else if (isHovered_) {
        fillColour = juce::Colour(0xFFCCAA88);  // Light when hovered
    } else {
        fillColour = juce::Colour(0xFF888888);  // Gray normally
    }

    g.setColour(fillColour);
    g.fillPath(diamond);

    // Border
    g.setColour(juce::Colour(0xFFCCCCCC));
    g.strokePath(diamond, juce::PathStrokeType(1.0f));
}

void TensionHandleComponent::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isLeftButtonDown()) {
        isDragging_ = true;
        dragStartY_ = e.y;
        dragStartTension_ = tension_;
        repaint();
    }
}

void TensionHandleComponent::mouseDrag(const juce::MouseEvent& e) {
    if (!isDragging_)
        return;

    // Dragging up increases tension (convex), down decreases (concave)
    // 50 pixels of drag = full range (-1 to +1)
    int deltaY = dragStartY_ - e.y;
    double deltaTension = static_cast<double>(deltaY) / 50.0;

    double newTension = juce::jlimit(-1.0, 1.0, dragStartTension_ + deltaTension);

    if (newTension != tension_) {
        tension_ = newTension;

        if (onTensionDragPreview) {
            onTensionDragPreview(pointId_, tension_);
        }

        repaint();
    }
}

void TensionHandleComponent::mouseUp(const juce::MouseEvent& /*e*/) {
    if (isDragging_) {
        isDragging_ = false;

        if (onTensionChanged) {
            onTensionChanged(pointId_, tension_);
        }

        repaint();
    }
}

void TensionHandleComponent::mouseEnter(const juce::MouseEvent& /*e*/) {
    isHovered_ = true;
    repaint();
}

void TensionHandleComponent::mouseExit(const juce::MouseEvent& /*e*/) {
    isHovered_ = false;
    repaint();
}

}  // namespace magda
