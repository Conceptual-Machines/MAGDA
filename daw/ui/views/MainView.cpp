#include "MainView.hpp"
#include "../themes/DarkTheme.hpp"
#include <BinaryData.h>

namespace magica {

MainView::MainView() {
    // Enable keyboard focus for shortcut handling
    setWantsKeyboardFocus(true);
    
    // Create timeline viewport
    timelineViewport = std::make_unique<juce::Viewport>();
    timeline = std::make_unique<TimelineComponent>();
    timelineViewport->setViewedComponent(timeline.get(), false);
    timelineViewport->setScrollBarsShown(false, false);
    addAndMakeVisible(*timelineViewport);
    
    // Set up timeline callbacks
    timeline->onPlayheadPositionChanged = [this](double position) {
        setPlayheadPosition(position);
    };
    
    timeline->onZoomChanged = [this](double newZoom) {
        horizontalZoom = newZoom;
        // Timeline already updated its zoom value, just sync track content
        trackContentPanel->setZoom(newZoom);
        updateContentSizes();
        repaint(); // Ensure playhead is redrawn
    };
    
    // Create track headers panel
    trackHeadersPanel = std::make_unique<TrackHeadersPanel>();
    addAndMakeVisible(trackHeadersPanel.get());
    
    // Create arrangement lock button
    arrangementLockButton = std::make_unique<SvgButton>("ArrangementLock", BinaryData::lock_svg, BinaryData::lock_svgSize);
    arrangementLockButton->setTooltip("Toggle arrangement lock (F4)");
    arrangementLockButton->onClick = [this]() {
        toggleArrangementLock();
    };
    addAndMakeVisible(arrangementLockButton.get());
    
    // Create track content viewport
    trackContentViewport = std::make_unique<juce::Viewport>();
    trackContentPanel = std::make_unique<TrackContentPanel>();
    trackContentViewport->setViewedComponent(trackContentPanel.get(), false);
    trackContentViewport->setScrollBarsShown(true, true);
    addAndMakeVisible(*trackContentViewport);
    
    // Create playhead component (always on top)
    playheadComponent = std::make_unique<PlayheadComponent>(*this);
    addAndMakeVisible(*playheadComponent);
    playheadComponent->toFront(false);
    
    // Set up scroll synchronization
    trackContentViewport->getHorizontalScrollBar().addListener(this);
    
    // Set up track synchronization between headers and content
    setupTrackSynchronization();
    
    // Set initial timeline length and zoom
    setTimelineLength(120.0);
}

MainView::~MainView() = default;

void MainView::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::BACKGROUND));
}

void MainView::resized() {
    auto bounds = getLocalBounds();
    
    // Timeline viewport at the top - offset by track header width
    auto timelineArea = bounds.removeFromTop(TIMELINE_HEIGHT);
    
    // Position lock button in the top-left corner above track headers
    auto lockButtonArea = timelineArea.removeFromLeft(TRACK_HEADER_WIDTH);
    lockButtonArea = lockButtonArea.removeFromTop(30).reduced(5); // 30px height, 5px margin
    arrangementLockButton->setBounds(lockButtonArea);
    
    // Timeline takes the remaining width
    timelineViewport->setBounds(timelineArea);
    
    // Track headers panel on the left (fixed width)
    auto trackHeadersArea = bounds.removeFromLeft(TRACK_HEADER_WIDTH);
    trackHeadersPanel->setBounds(trackHeadersArea);
    
    // Track content viewport gets the remaining space
    trackContentViewport->setBounds(bounds);
    
    // Playhead component covers only the content area (not scrollbars)
    auto playheadArea = bounds;
    // Reduce the area to avoid covering scrollbars
    int scrollBarThickness = trackContentViewport->getScrollBarThickness();
    playheadArea = playheadArea.withTrimmedRight(scrollBarThickness).withTrimmedBottom(scrollBarThickness);
    playheadComponent->setBounds(playheadArea);
    
    // Always recalculate zoom to ensure proper timeline distribution
    auto viewportWidth = timelineViewport->getWidth();
    if (viewportWidth > 0) {
        // Show about 60 seconds initially, but ensure minimum zoom for visibility
        double newZoom = juce::jmax(1.0, viewportWidth / 60.0);
        if (std::abs(horizontalZoom - newZoom) > 0.1) { // Only update if significantly different
            horizontalZoom = newZoom;
            // Update zoom on timeline and track content
            timeline->setZoom(horizontalZoom);
            trackContentPanel->setZoom(horizontalZoom);
        }
    }
    
    updateContentSizes();
}

void MainView::setHorizontalZoom(double zoomFactor) {
    // Update zoom
    horizontalZoom = juce::jmax(0.1, zoomFactor);
    
    // Update zoom on timeline and track content
    timeline->setZoom(horizontalZoom);
    trackContentPanel->setZoom(horizontalZoom);
    
    updateContentSizes();
    
    // Center playhead in viewport after zoom
    int viewportWidth = trackContentViewport->getWidth();
    int contentWidth = static_cast<int>(timelineLength * horizontalZoom);
    
    if (contentWidth > viewportWidth) {
        // Calculate playhead position in pixels with new zoom
        int playheadPixelPos = static_cast<int>(playheadPosition * horizontalZoom);
        
        // Center playhead in viewport
        int newScrollX = playheadPixelPos - (viewportWidth / 2);
        
        // Clamp scroll position to valid range
        int maxScrollX = juce::jmax(0, contentWidth - viewportWidth);
        newScrollX = juce::jlimit(0, maxScrollX, newScrollX);
        
        // Apply the new scroll position to both viewports
        isUpdatingFromZoom = true;
        timelineViewport->setViewPosition(newScrollX, 0);
        trackContentViewport->setViewPosition(newScrollX, trackContentViewport->getViewPositionY());
        isUpdatingFromZoom = false;
    } else {
        // If content fits in viewport, scroll to beginning
        isUpdatingFromZoom = true;
        timelineViewport->setViewPosition(0, 0);
        trackContentViewport->setViewPosition(0, trackContentViewport->getViewPositionY());
        isUpdatingFromZoom = false;
    }
    
    repaint(); // Repaint for unified playhead
}

void MainView::setVerticalZoom(double zoomFactor) {
    verticalZoom = juce::jmax(0.5, juce::jmin(3.0, zoomFactor));
    updateContentSizes();
}

void MainView::scrollToPosition(double timePosition) {
    auto pixelPosition = static_cast<int>(timePosition * horizontalZoom);
    timelineViewport->setViewPosition(pixelPosition, 0);
    trackContentViewport->setViewPosition(pixelPosition, trackContentViewport->getViewPositionY());
}

void MainView::scrollToTrack(int trackIndex) {
    if (trackIndex >= 0 && trackIndex < trackHeadersPanel->getNumTracks()) {
        int yPosition = trackHeadersPanel->getTrackYPosition(trackIndex);
        trackContentViewport->setViewPosition(trackContentViewport->getViewPositionX(), yPosition);
    }
}

void MainView::addTrack() {
    trackHeadersPanel->addTrack();
    trackContentPanel->addTrack();
    updateContentSizes();
}

void MainView::removeTrack(int trackIndex) {
    trackHeadersPanel->removeTrack(trackIndex);
    trackContentPanel->removeTrack(trackIndex);
    updateContentSizes();
}

void MainView::selectTrack(int trackIndex) {
    trackHeadersPanel->selectTrack(trackIndex);
    trackContentPanel->selectTrack(trackIndex);
}

void MainView::setTimelineLength(double lengthInSeconds) {
    timelineLength = lengthInSeconds;
    timeline->setTimelineLength(lengthInSeconds);
    trackContentPanel->setTimelineLength(lengthInSeconds);
    updateContentSizes();
}

void MainView::setPlayheadPosition(double position) {
    playheadPosition = juce::jlimit(0.0, timelineLength, position);
    playheadComponent->setPlayheadPosition(playheadPosition);
    playheadComponent->repaint();
}

void MainView::toggleArrangementLock() {
    timeline->setArrangementLocked(!timeline->isArrangementLocked());
    timeline->repaint();
    
    // Update lock button icon
    if (timeline->isArrangementLocked()) {
        arrangementLockButton->updateSvgData(BinaryData::lock_svg, BinaryData::lock_svgSize);
        arrangementLockButton->setTooltip("Arrangement locked - Click to unlock (F4)");
    } else {
        arrangementLockButton->updateSvgData(BinaryData::lock_open_svg, BinaryData::lock_open_svgSize);
        arrangementLockButton->setTooltip("Arrangement unlocked - Click to lock (F4)");
    }
}

bool MainView::isArrangementLocked() const {
    return timeline->isArrangementLocked();
}

bool MainView::keyPressed(const juce::KeyPress& key) {
    // Toggle arrangement lock with F4
    if (key.isKeyCode(juce::KeyPress::F4Key)) {
        toggleArrangementLock();
        return true;
    }
    
    return false;
}

void MainView::updateContentSizes() {
    auto contentWidth = static_cast<int>(timelineLength * horizontalZoom);
    auto trackContentHeight = trackHeadersPanel->getTotalTracksHeight();
    
    // Update timeline size
    timeline->setSize(juce::jmax(contentWidth, timelineViewport->getWidth()), TIMELINE_HEIGHT);
    
    // Update track content size - let viewport handle scrollbar ranges automatically
    trackContentPanel->setSize(contentWidth, trackContentHeight);
    
    // Update track headers panel height to match content
    trackHeadersPanel->setSize(TRACK_HEADER_WIDTH, juce::jmax(trackContentHeight, trackContentViewport->getHeight()));
    
    // Repaint playhead after content size changes
    playheadComponent->repaint();
}

void MainView::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) {
    // Don't interfere if this is triggered by zoom logic
    if (isUpdatingFromZoom) {
        return;
    }
    
    // Sync timeline viewport when track content viewport scrolls horizontally
    if (scrollBarThatHasMoved == &trackContentViewport->getHorizontalScrollBar()) {
        timelineViewport->setViewPosition(static_cast<int>(newRangeStart), 0);
        // Force playhead repaint when scrolling
        playheadComponent->repaint();
    }
}

void MainView::syncTrackHeights() {
    // Sync track heights between headers and content panels
    int numTracks = trackHeadersPanel->getNumTracks();
    for (int i = 0; i < numTracks; ++i) {
        int headerHeight = trackHeadersPanel->getTrackHeight(i);
        int contentHeight = trackContentPanel->getTrackHeight(i);
        
        if (headerHeight != contentHeight) {
            // Sync to the header height (headers are the source of truth)
            trackContentPanel->setTrackHeight(i, headerHeight);
        }
    }
}

void MainView::setupTrackSynchronization() {
    // Set up callbacks to keep track headers and content in sync
    trackHeadersPanel->onTrackHeightChanged = [this](int trackIndex, int newHeight) {
        trackContentPanel->setTrackHeight(trackIndex, newHeight);
        updateContentSizes();
    };
    
    trackHeadersPanel->onTrackSelected = [this](int trackIndex) {
        if (!isUpdatingTrackSelection) {
            isUpdatingTrackSelection = true;
            trackContentPanel->selectTrack(trackIndex);
            isUpdatingTrackSelection = false;
        }
    };
    
    trackContentPanel->onTrackSelected = [this](int trackIndex) {
        if (!isUpdatingTrackSelection) {
            isUpdatingTrackSelection = true;
            trackHeadersPanel->selectTrack(trackIndex);
            isUpdatingTrackSelection = false;
        }
    };
}

// PlayheadComponent implementation
MainView::PlayheadComponent::PlayheadComponent(MainView& owner) : owner(owner) {
    setInterceptsMouseClicks(true, true); // Enable mouse interaction for dragging
}

MainView::PlayheadComponent::~PlayheadComponent() = default;

void MainView::PlayheadComponent::paint(juce::Graphics& g) {
    if (playheadPosition < 0 || playheadPosition > owner.timelineLength) {
        return;
    }
    
    // Calculate playhead position in pixels
    int playheadX = static_cast<int>(playheadPosition * owner.horizontalZoom);
    
    // Adjust for horizontal scroll offset from track content viewport (not timeline viewport)
    int scrollOffset = owner.trackContentViewport->getViewPositionX();
    playheadX -= scrollOffset;
    
    // Only draw if playhead is visible
    if (playheadX >= 0 && playheadX < getWidth()) {
        // Draw shadow for better visibility
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(playheadX + 1, 0, playheadX + 1, getHeight(), 5.0f);
        
        // Draw main playhead line
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        g.drawLine(playheadX, 0, playheadX, getHeight(), 4.0f);
        
        // Draw playhead handle at the top
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
        juce::Path triangle;
        triangle.addTriangle(playheadX - 6, 0, playheadX + 6, 0, playheadX, 12);
        g.fillPath(triangle);
    }
}

void MainView::PlayheadComponent::setPlayheadPosition(double position) {
    playheadPosition = position;
    repaint();
}

void MainView::PlayheadComponent::mouseDown(const juce::MouseEvent& event) {
    // Check if clicking near the playhead line
    int playheadX = static_cast<int>(playheadPosition * owner.horizontalZoom);
    int scrollOffset = owner.trackContentViewport->getViewPositionX();
    playheadX -= scrollOffset;
    
    // Allow clicking within 10 pixels of the playhead line
    if (std::abs(event.x - playheadX) <= 10) {
        isDragging = true;
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    }
}

void MainView::PlayheadComponent::mouseDrag(const juce::MouseEvent& event) {
    if (isDragging) {
        // Calculate new playhead position based on mouse X position
        int scrollOffset = owner.trackContentViewport->getViewPositionX();
        int adjustedX = event.x + scrollOffset;
        double newPosition = static_cast<double>(adjustedX) / owner.horizontalZoom;
        
        // Clamp to timeline bounds
        newPosition = juce::jlimit(0.0, owner.timelineLength, newPosition);
        
        // Update playhead position
        owner.setPlayheadPosition(newPosition);
    }
}

void MainView::PlayheadComponent::mouseUp(const juce::MouseEvent& event) {
    isDragging = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MainView::mouseDown(const juce::MouseEvent& event) {
    // Removed timeline zoom handling - let timeline component handle its own zoom
    // The timeline component now handles zoom gestures in its lower half
}

void MainView::mouseDrag(const juce::MouseEvent& event) {
    // Removed zoom handling - timeline component handles its own zoom
}

void MainView::mouseUp(const juce::MouseEvent& event) {
    // Removed zoom handling - timeline component handles its own zoom
}

} // namespace magica 