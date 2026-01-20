#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace magda {

/**
 * Manages custom mouse cursors for the DAW UI.
 * Loads cursors from SVG assets and caches them.
 */
class CursorManager {
  public:
    static CursorManager& getInstance();

    // Get zoom cursors
    const juce::MouseCursor& getZoomCursor() const {
        return zoomCursor;
    }
    const juce::MouseCursor& getZoomInCursor() const {
        return zoomInCursor;
    }
    const juce::MouseCursor& getZoomOutCursor() const {
        return zoomOutCursor;
    }

  private:
    CursorManager();
    ~CursorManager() = default;

    // Disable copying
    CursorManager(const CursorManager&) = delete;
    CursorManager& operator=(const CursorManager&) = delete;

    // Create a cursor from SVG binary data
    juce::MouseCursor createCursorFromSvg(const char* svgData, int svgSize, int cursorSize,
                                          int hotspotX, int hotspotY);

    // Cached cursors
    juce::MouseCursor zoomCursor;
    juce::MouseCursor zoomInCursor;
    juce::MouseCursor zoomOutCursor;
};

}  // namespace magda
