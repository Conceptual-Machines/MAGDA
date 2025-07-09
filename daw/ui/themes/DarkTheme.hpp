#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace magica {

class DarkTheme {
public:
    // Main colors
    static constexpr auto BACKGROUND = 0xFF1E1E1E;           // Very dark grey
    static constexpr auto PANEL_BACKGROUND = 0xFF2D2D2D;     // Slightly lighter panel background
    static constexpr auto SURFACE = 0xFF3C3C3C;              // Surface elements
    static constexpr auto SURFACE_HOVER = 0xFF4A4A4A;        // Hovered surface elements
    
    // Transport and controls
    static constexpr auto TRANSPORT_BACKGROUND = 0xFF252525; // Transport bar background
    static constexpr auto BUTTON_NORMAL = 0xFF404040;        // Normal button
    static constexpr auto BUTTON_HOVER = 0xFF505050;         // Hovered button
    static constexpr auto BUTTON_PRESSED = 0xFF606060;       // Pressed button
    static constexpr auto BUTTON_ACTIVE = 0xFF0078D4;        // Active/selected button (blue)
    
    // Text colors
    static constexpr auto TEXT_PRIMARY = 0xFFFFFFFF;         // Primary text (white)
    static constexpr auto TEXT_SECONDARY = 0xFFB3B3B3;       // Secondary text (light grey)
    static constexpr auto TEXT_DISABLED = 0xFF666666;        // Disabled text
    
    // Accent colors
    static constexpr auto ACCENT_BLUE = 0xFF0078D4;          // Primary accent (Microsoft blue)
    static constexpr auto ACCENT_GREEN = 0xFF107C10;         // Success/record (green)
    static constexpr auto ACCENT_RED = 0xFFD13438;           // Error/stop (red)
    static constexpr auto ACCENT_ORANGE = 0xFFFF8C00;        // Warning/pause (orange)
    
    // Track colors
    static constexpr auto TRACK_BACKGROUND = 0xFF333333;     // Track background
    static constexpr auto TRACK_SELECTED = 0xFF404040;       // Selected track
    static constexpr auto TRACK_SEPARATOR = 0xFF1A1A1A;      // Track separator lines
    
    // Timeline and grid
    static constexpr auto TIMELINE_BACKGROUND = 0xFF2A2A2A;  // Timeline background
    static constexpr auto GRID_LINE = 0xFF404040;            // Grid lines
    static constexpr auto BEAT_LINE = 0xFF505050;            // Beat lines (stronger)
    static constexpr auto BAR_LINE = 0xFF606060;             // Bar lines (strongest)
    
    // Borders and separators
    static constexpr auto BORDER = 0xFF404040;               // General borders
    static constexpr auto SEPARATOR = 0xFF2A2A2A;            // Panel separators
    static constexpr auto RESIZE_HANDLE = 0xFF505050;        // Resize handles
    
    // Audio visualization
    static constexpr auto WAVEFORM_NORMAL = 0xFF00D4AA;      // Waveform color
    static constexpr auto WAVEFORM_SELECTED = 0xFF00F5C4;    // Selected waveform
    static constexpr auto LEVEL_METER_GREEN = 0xFF107C10;    // Level meter (low)
    static constexpr auto LEVEL_METER_YELLOW = 0xFFFFB900;   // Level meter (mid)
    static constexpr auto LEVEL_METER_RED = 0xFFD13438;      // Level meter (high)

    // Apply the theme to JUCE's LookAndFeel
    static void applyToLookAndFeel(juce::LookAndFeel& laf);
    
    // Get color as JUCE Colour object
    static juce::Colour getColour(juce::uint32 colorValue) {
        return juce::Colour(colorValue);
    }
    
    // Helper methods for common color combinations
    static juce::Colour getBackgroundColour() { return getColour(BACKGROUND); }
    static juce::Colour getPanelBackgroundColour() { return getColour(PANEL_BACKGROUND); }
    static juce::Colour getTextColour() { return getColour(TEXT_PRIMARY); }
    static juce::Colour getSecondaryTextColour() { return getColour(TEXT_SECONDARY); }
    static juce::Colour getAccentColour() { return getColour(ACCENT_BLUE); }
    static juce::Colour getBorderColour() { return getColour(BORDER); }
};

} // namespace magica 