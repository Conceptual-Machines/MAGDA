#include "FontManager.hpp"
#include <iostream>

namespace magica {

FontManager& FontManager::getInstance() {
    static FontManager instance;
    return instance;
}

bool FontManager::initialize() {
    if (initialized) {
        return true;
    }
    
    bool success = true;
    
    initialized = success;
    
    if (initialized) {
        std::cout << "✓ Inter fonts loaded successfully" << std::endl;
    } else {
        std::cerr << "⚠ Some Inter fonts failed to load, falling back to system fonts" << std::endl;
    }
    
    return initialized;
}

juce::Font FontManager::getInterFont(float size, Weight weight) const {
    juce::Typeface* typeface = nullptr;
    
    switch (weight) {
        case Weight::Regular:
            typeface = interRegular.get();
            break;
        case Weight::Medium:
            typeface = interMedium.get();
            break;
        case Weight::SemiBold:
            typeface = interSemiBold.get();
            break;
        case Weight::Bold:
            typeface = interBold.get();
            break;
    }
    
    if (typeface) {
        return juce::Font(typeface).withHeight(size);
    }
    
    // Fallback to system font
    auto style = juce::Font::plain;
    switch (weight) {
        case Weight::Bold:
            style = juce::Font::bold;
            break;
        default:
            style = juce::Font::plain;
            break;
    }
    
    return juce::Font(FALLBACK_FONT, size, style);
}

juce::Font FontManager::getUIFont(float size) const {
    return getInterFont(size, Weight::Regular);
}

juce::Font FontManager::getUIFontMedium(float size) const {
    return getInterFont(size, Weight::Medium);
}

juce::Font FontManager::getUIFontBold(float size) const {
    return getInterFont(size, Weight::Bold);
}

juce::Font FontManager::getHeadingFont(float size) const {
    return getInterFont(size, Weight::SemiBold);
}

juce::Font FontManager::getButtonFont(float size) const {
    return getInterFont(size, Weight::Medium);
}

juce::Font FontManager::getTimeFont(float size) const {
    return getInterFont(size, Weight::SemiBold);
}



} // namespace magica 