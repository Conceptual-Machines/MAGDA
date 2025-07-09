#include "BottomPanel.hpp"
#include "../themes/DarkTheme.hpp"
#include "../themes/FontManager.hpp"

namespace magica {

BottomPanel::BottomPanel() {
    setName("Bottom Panel");
}

BottomPanel::~BottomPanel() = default;

void BottomPanel::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());
    
    // Draw a border
    g.setColour(DarkTheme::getBorderColour());
    g.drawRect(getLocalBounds(), 1);
    
    // Draw placeholder text
    g.setColour(DarkTheme::getSecondaryTextColour());
    g.setFont(FontManager::getInstance().getUIFont(14.0f));
    g.drawText("Bottom Panel\n(Mixer/Effects/Piano Roll)", getLocalBounds(), juce::Justification::centred);
}

void BottomPanel::resized() {
    // Layout will be implemented later
}

} // namespace magica 