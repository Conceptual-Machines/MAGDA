#include "CursorManager.hpp"

#include "BinaryData.h"

namespace magda {

CursorManager& CursorManager::getInstance() {
    static CursorManager instance;
    return instance;
}

CursorManager::CursorManager() {
    // Load zoom cursors from dedicated cursor SVG assets
    // These already have white outline built in
    // Cursor size 24x24, hotspot at center of lens (~9, 9 for 24px)
    const int cursorSize = 24;
    const int hotspotX = 9;
    const int hotspotY = 9;

    zoomCursor = createCursorFromSvg(BinaryData::search_svg2, BinaryData::search_svg2Size,
                                     cursorSize, hotspotX, hotspotY);
    zoomInCursor = createCursorFromSvg(BinaryData::zoom_in_svg2, BinaryData::zoom_in_svg2Size,
                                       cursorSize, hotspotX, hotspotY);
    zoomOutCursor = createCursorFromSvg(BinaryData::zoom_out_svg2, BinaryData::zoom_out_svg2Size,
                                        cursorSize, hotspotX, hotspotY);
}

juce::MouseCursor CursorManager::createCursorFromSvg(const char* svgData, int svgSize,
                                                     int cursorSize, int hotspotX, int hotspotY) {
    // Parse the SVG
    auto drawable = juce::Drawable::createFromImageData(svgData, static_cast<size_t>(svgSize));
    if (drawable == nullptr) {
        return juce::MouseCursor::CrosshairCursor;
    }

    // Create an image to render the cursor
    juce::Image cursorImage(juce::Image::ARGB, cursorSize, cursorSize, true);
    juce::Graphics g(cursorImage);

    // Get the original bounds and calculate scale
    auto bounds = drawable->getDrawableBounds();
    float scale = static_cast<float>(cursorSize) / std::max(bounds.getWidth(), bounds.getHeight());

    // Apply transform: scale to cursor size
    juce::AffineTransform transform = juce::AffineTransform::scale(scale);

    // Draw the SVG (already has black fill with white outline)
    drawable->draw(g, 1.0f, transform);

    return juce::MouseCursor(cursorImage, hotspotX, hotspotY);
}

}  // namespace magda
