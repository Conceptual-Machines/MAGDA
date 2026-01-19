#include "TrackHeadersPanel.hpp"

#include <algorithm>
#include <functional>

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"

namespace magica {

TrackHeadersPanel::TrackHeader::TrackHeader(const juce::String& trackName) : name(trackName) {
    // Create UI components
    nameLabel = std::make_unique<juce::Label>("trackName", trackName);
    nameLabel->setEditable(true);
    nameLabel->setColour(juce::Label::textColourId, DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    nameLabel->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    nameLabel->setFont(FontManager::getInstance().getUIFont(12.0f));

    muteButton = std::make_unique<juce::TextButton>("M");
    muteButton->setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    muteButton->setColour(juce::TextButton::buttonOnColourId,
                          DarkTheme::getColour(DarkTheme::STATUS_WARNING));
    muteButton->setColour(juce::TextButton::textColourOffId,
                          DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    muteButton->setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::BACKGROUND));
    muteButton->setClickingTogglesState(true);

    soloButton = std::make_unique<juce::TextButton>("S");
    soloButton->setColour(juce::TextButton::buttonColourId,
                          DarkTheme::getColour(DarkTheme::SURFACE));
    soloButton->setColour(juce::TextButton::buttonOnColourId,
                          DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
    soloButton->setColour(juce::TextButton::textColourOffId,
                          DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
    soloButton->setColour(juce::TextButton::textColourOnId,
                          DarkTheme::getColour(DarkTheme::BACKGROUND));
    soloButton->setClickingTogglesState(true);

    volumeSlider =
        std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    volumeSlider->setRange(0.0, 1.0);
    volumeSlider->setValue(volume);
    volumeSlider->setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    volumeSlider->setColour(juce::Slider::thumbColourId,
                            DarkTheme::getColour(DarkTheme::ACCENT_BLUE));

    panSlider =
        std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    panSlider->setRange(-1.0, 1.0);
    panSlider->setValue(pan);
    panSlider->setColour(juce::Slider::trackColourId, DarkTheme::getColour(DarkTheme::SURFACE));
    panSlider->setColour(juce::Slider::thumbColourId, DarkTheme::getColour(DarkTheme::ACCENT_BLUE));

    // Collapse button for groups (triangle indicator)
    collapseButton = std::make_unique<juce::TextButton>();
    collapseButton->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    collapseButton->setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    collapseButton->setColour(juce::TextButton::textColourOffId,
                              DarkTheme::getColour(DarkTheme::TEXT_PRIMARY));
}

TrackHeadersPanel::TrackHeadersPanel() {
    setSize(TRACK_HEADER_WIDTH, 400);

    // Register as TrackManager listener
    TrackManager::getInstance().addListener(this);

    // Register as ViewModeController listener
    ViewModeController::getInstance().addListener(this);
    currentViewMode_ = ViewModeController::getInstance().getViewMode();

    // Build tracks from TrackManager
    tracksChanged();
}

TrackHeadersPanel::~TrackHeadersPanel() {
    TrackManager::getInstance().removeListener(this);
    ViewModeController::getInstance().removeListener(this);
}

void TrackHeadersPanel::viewModeChanged(ViewMode mode, const AudioEngineProfile& /*profile*/) {
    currentViewMode_ = mode;
    tracksChanged();  // Rebuild with new visibility settings
}

void TrackHeadersPanel::tracksChanged() {
    // Clear existing track headers
    for (auto& header : trackHeaders) {
        removeChildComponent(header->nameLabel.get());
        removeChildComponent(header->muteButton.get());
        removeChildComponent(header->soloButton.get());
        removeChildComponent(header->volumeSlider.get());
        removeChildComponent(header->panSlider.get());
        removeChildComponent(header->collapseButton.get());
    }
    trackHeaders.clear();
    visibleTrackIds_.clear();
    selectedTrackIndex = -1;

    // Build visible tracks list (respecting hierarchy)
    auto& trackManager = TrackManager::getInstance();
    auto topLevelTracks = trackManager.getVisibleTopLevelTracks(currentViewMode_);

    // Helper lambda to add track and its visible children recursively
    std::function<void(TrackId, int)> addTrackRecursive = [&](TrackId trackId, int depth) {
        const auto* track = trackManager.getTrack(trackId);
        if (!track || !track->isVisibleIn(currentViewMode_))
            return;

        visibleTrackIds_.push_back(trackId);

        auto header = std::make_unique<TrackHeader>(track->name);
        header->trackId = trackId;
        header->depth = depth;
        header->isGroup = track->isGroup();
        header->isCollapsed = track->isCollapsedIn(currentViewMode_);
        header->muted = track->muted;
        header->solo = track->soloed;
        header->volume = track->volume;
        header->pan = track->pan;

        // Use height from view settings
        header->height = track->viewSettings.getHeight(currentViewMode_);

        // Set up callbacks with track ID (not index)
        setupTrackHeaderWithId(*header, trackId);

        // Add components
        addAndMakeVisible(*header->nameLabel);
        addAndMakeVisible(*header->muteButton);
        addAndMakeVisible(*header->soloButton);
        addAndMakeVisible(*header->volumeSlider);
        addAndMakeVisible(*header->panSlider);

        // Add collapse button for groups
        if (header->isGroup) {
            header->collapseButton->setButtonText(header->isCollapsed ? "▶" : "▼");
            header->collapseButton->onClick = [this, trackId]() { handleCollapseToggle(trackId); };
            addAndMakeVisible(*header->collapseButton);
        }

        // Update UI state
        header->muteButton->setToggleState(track->muted, juce::dontSendNotification);
        header->soloButton->setToggleState(track->soloed, juce::dontSendNotification);
        header->volumeSlider->setValue(track->volume, juce::dontSendNotification);
        header->panSlider->setValue(track->pan, juce::dontSendNotification);

        trackHeaders.push_back(std::move(header));

        // Add children if group is not collapsed
        if (track->isGroup() && !track->isCollapsedIn(currentViewMode_)) {
            for (auto childId : track->childIds) {
                addTrackRecursive(childId, depth + 1);
            }
        }
    };

    // Add all visible top-level tracks (and their children)
    for (auto trackId : topLevelTracks) {
        addTrackRecursive(trackId, 0);
    }

    updateTrackHeaderLayout();
    repaint();
}

void TrackHeadersPanel::trackPropertyChanged(int trackId) {
    const auto* track = TrackManager::getInstance().getTrack(trackId);
    if (!track)
        return;

    // Find the index in our visible tracks list
    int index = -1;
    for (size_t i = 0; i < visibleTrackIds_.size(); ++i) {
        if (visibleTrackIds_[i] == trackId) {
            index = static_cast<int>(i);
            break;
        }
    }

    if (index >= 0 && index < static_cast<int>(trackHeaders.size())) {
        auto& header = *trackHeaders[index];
        header.name = track->name;
        header.muted = track->muted;
        header.solo = track->soloed;
        header.volume = track->volume;
        header.pan = track->pan;

        // Update height from view settings
        header.height = track->viewSettings.getHeight(currentViewMode_);

        header.nameLabel->setText(track->name, juce::dontSendNotification);
        header.muteButton->setToggleState(track->muted, juce::dontSendNotification);
        header.soloButton->setToggleState(track->soloed, juce::dontSendNotification);
        header.volumeSlider->setValue(track->volume, juce::dontSendNotification);
        header.panSlider->setValue(track->pan, juce::dontSendNotification);

        updateTrackHeaderLayout();
        repaint();
    }
}

void TrackHeadersPanel::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getColour(DarkTheme::PANEL_BACKGROUND));

    // Draw border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(getLocalBounds(), 1);

    // Draw track headers
    for (size_t i = 0; i < trackHeaders.size(); ++i) {
        auto headerArea = getTrackHeaderArea(static_cast<int>(i));
        if (headerArea.intersects(getLocalBounds())) {
            paintTrackHeader(g, *trackHeaders[i], headerArea,
                             static_cast<int>(i) == selectedTrackIndex);

            // Draw resize handle
            auto resizeArea = getResizeHandleArea(static_cast<int>(i));
            paintResizeHandle(g, resizeArea);
        }
    }
}

void TrackHeadersPanel::resized() {
    updateTrackHeaderLayout();
}

void TrackHeadersPanel::addTrack() {
    juce::String trackName = "Track " + juce::String(trackHeaders.size() + 1);
    auto header = std::make_unique<TrackHeader>(trackName);

    // Set up callbacks
    int trackIndex = static_cast<int>(trackHeaders.size());
    setupTrackHeader(*header, trackIndex);

    // Add components
    addAndMakeVisible(*header->nameLabel);
    addAndMakeVisible(*header->muteButton);
    addAndMakeVisible(*header->soloButton);
    addAndMakeVisible(*header->volumeSlider);
    addAndMakeVisible(*header->panSlider);

    trackHeaders.push_back(std::move(header));

    updateTrackHeaderLayout();
    repaint();
}

void TrackHeadersPanel::removeTrack(int index) {
    if (index >= 0 && index < trackHeaders.size()) {
        trackHeaders.erase(trackHeaders.begin() + index);

        if (selectedTrackIndex == index) {
            selectedTrackIndex = -1;
        } else if (selectedTrackIndex > index) {
            selectedTrackIndex--;
        }

        updateTrackHeaderLayout();
        repaint();
    }
}

void TrackHeadersPanel::selectTrack(int index) {
    if (index >= 0 && index < trackHeaders.size()) {
        selectedTrackIndex = index;

        if (onTrackSelected) {
            onTrackSelected(index);
        }

        repaint();
    }
}

int TrackHeadersPanel::getNumTracks() const {
    return static_cast<int>(trackHeaders.size());
}

void TrackHeadersPanel::setTrackHeight(int trackIndex, int height) {
    if (trackIndex >= 0 && trackIndex < trackHeaders.size()) {
        height = juce::jlimit(MIN_TRACK_HEIGHT, MAX_TRACK_HEIGHT, height);
        trackHeaders[trackIndex]->height = height;

        updateTrackHeaderLayout();
        repaint();

        if (onTrackHeightChanged) {
            onTrackHeightChanged(trackIndex, height);
        }
    }
}

int TrackHeadersPanel::getTrackHeight(int trackIndex) const {
    if (trackIndex >= 0 && trackIndex < trackHeaders.size()) {
        return trackHeaders[trackIndex]->height;
    }
    return DEFAULT_TRACK_HEIGHT;
}

int TrackHeadersPanel::getTotalTracksHeight() const {
    int totalHeight = 0;
    for (const auto& header : trackHeaders) {
        totalHeight += static_cast<int>(header->height * verticalZoom);
    }
    return totalHeight;
}

int TrackHeadersPanel::getTrackYPosition(int trackIndex) const {
    int yPosition = 0;
    for (int i = 0; i < trackIndex && i < trackHeaders.size(); ++i) {
        yPosition += static_cast<int>(trackHeaders[i]->height * verticalZoom);
    }
    return yPosition;
}

void TrackHeadersPanel::setVerticalZoom(double zoom) {
    verticalZoom = juce::jlimit(0.5, 3.0, zoom);
    updateTrackHeaderLayout();
    repaint();
}

void TrackHeadersPanel::setupTrackHeader(TrackHeader& header, int trackIndex) {
    // Name label callback
    header.nameLabel->onTextChange = [this, trackIndex]() {
        if (trackIndex < trackHeaders.size()) {
            auto& header = *trackHeaders[trackIndex];
            header.name = header.nameLabel->getText();

            if (onTrackNameChanged) {
                onTrackNameChanged(trackIndex, header.name);
            }
        }
    };

    // Mute button callback
    header.muteButton->onClick = [this, trackIndex]() {
        if (trackIndex < trackHeaders.size()) {
            auto& header = *trackHeaders[trackIndex];
            header.muted = header.muteButton->getToggleState();

            if (onTrackMutedChanged) {
                onTrackMutedChanged(trackIndex, header.muted);
            }
        }
    };

    // Solo button callback
    header.soloButton->onClick = [this, trackIndex]() {
        if (trackIndex < trackHeaders.size()) {
            auto& header = *trackHeaders[trackIndex];
            header.solo = header.soloButton->getToggleState();

            if (onTrackSoloChanged) {
                onTrackSoloChanged(trackIndex, header.solo);
            }
        }
    };

    // Volume slider callback
    header.volumeSlider->onValueChange = [this, trackIndex]() {
        if (trackIndex < trackHeaders.size()) {
            auto& header = *trackHeaders[trackIndex];
            header.volume = static_cast<float>(header.volumeSlider->getValue());

            if (onTrackVolumeChanged) {
                onTrackVolumeChanged(trackIndex, header.volume);
            }
        }
    };

    // Pan slider callback
    header.panSlider->onValueChange = [this, trackIndex]() {
        if (trackIndex < trackHeaders.size()) {
            auto& header = *trackHeaders[trackIndex];
            header.pan = static_cast<float>(header.panSlider->getValue());

            if (onTrackPanChanged) {
                onTrackPanChanged(trackIndex, header.pan);
            }
        }
    };
}

void TrackHeadersPanel::setupTrackHeaderWithId(TrackHeader& header, int trackId) {
    // Name label callback - updates TrackManager
    header.nameLabel->onTextChange = [this, trackId]() {
        int index = TrackManager::getInstance().getTrackIndex(trackId);
        if (index >= 0 && index < static_cast<int>(trackHeaders.size())) {
            auto& header = *trackHeaders[index];
            header.name = header.nameLabel->getText();
            TrackManager::getInstance().setTrackName(trackId, header.name);
        }
    };

    // Mute button callback - updates TrackManager
    header.muteButton->onClick = [this, trackId]() {
        int index = TrackManager::getInstance().getTrackIndex(trackId);
        if (index >= 0 && index < static_cast<int>(trackHeaders.size())) {
            auto& header = *trackHeaders[index];
            header.muted = header.muteButton->getToggleState();
            TrackManager::getInstance().setTrackMuted(trackId, header.muted);
        }
    };

    // Solo button callback - updates TrackManager
    header.soloButton->onClick = [this, trackId]() {
        int index = TrackManager::getInstance().getTrackIndex(trackId);
        if (index >= 0 && index < static_cast<int>(trackHeaders.size())) {
            auto& header = *trackHeaders[index];
            header.solo = header.soloButton->getToggleState();
            TrackManager::getInstance().setTrackSoloed(trackId, header.solo);
        }
    };

    // Volume slider callback - updates TrackManager
    header.volumeSlider->onValueChange = [this, trackId]() {
        int index = TrackManager::getInstance().getTrackIndex(trackId);
        if (index >= 0 && index < static_cast<int>(trackHeaders.size())) {
            auto& header = *trackHeaders[index];
            header.volume = static_cast<float>(header.volumeSlider->getValue());
            TrackManager::getInstance().setTrackVolume(trackId, header.volume);
        }
    };

    // Pan slider callback - updates TrackManager
    header.panSlider->onValueChange = [this, trackId]() {
        int index = TrackManager::getInstance().getTrackIndex(trackId);
        if (index >= 0 && index < static_cast<int>(trackHeaders.size())) {
            auto& header = *trackHeaders[index];
            header.pan = static_cast<float>(header.panSlider->getValue());
            TrackManager::getInstance().setTrackPan(trackId, header.pan);
        }
    };
}

void TrackHeadersPanel::paintTrackHeader(juce::Graphics& g, const TrackHeader& header,
                                         juce::Rectangle<int> area, bool isSelected) {
    // Calculate indent
    int indent = header.depth * INDENT_WIDTH;

    // Draw indent guide lines for nested tracks
    if (header.depth > 0) {
        g.setColour(DarkTheme::getColour(DarkTheme::BORDER).withAlpha(0.5f));
        for (int d = 0; d < header.depth; ++d) {
            int x = d * INDENT_WIDTH + INDENT_WIDTH / 2;
            g.drawLine(x, area.getY(), x, area.getBottom(), 1.0f);
        }
    }

    // Background - groups have slightly different color
    auto bgArea = area.withTrimmedLeft(indent);
    if (header.isGroup) {
        g.setColour(isSelected ? DarkTheme::getColour(DarkTheme::TRACK_SELECTED)
                               : DarkTheme::getColour(DarkTheme::SURFACE).brighter(0.05f));
    } else {
        g.setColour(isSelected ? DarkTheme::getColour(DarkTheme::TRACK_SELECTED)
                               : DarkTheme::getColour(DarkTheme::TRACK_BACKGROUND));
    }
    g.fillRect(bgArea);

    // Border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(bgArea, 1);

    // Group indicator color strip on the left
    if (header.isGroup) {
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_ORANGE).withAlpha(0.7f));
        g.fillRect(bgArea.getX(), bgArea.getY(), 3, bgArea.getHeight());
    }
}

void TrackHeadersPanel::paintResizeHandle(juce::Graphics& g, juce::Rectangle<int> area) {
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.fillRect(area);

    // Draw resize grip
    g.setColour(DarkTheme::getColour(DarkTheme::TEXT_SECONDARY));
    int centerY = area.getCentreY();
    for (int i = 0; i < 3; ++i) {
        int x = area.getX() + 5 + i * 3;
        g.drawLine(x, centerY - 1, x, centerY + 1, 1.0f);
    }
}

juce::Rectangle<int> TrackHeadersPanel::getTrackHeaderArea(int trackIndex) const {
    if (trackIndex < 0 || trackIndex >= trackHeaders.size()) {
        return {};
    }

    int yPosition = getTrackYPosition(trackIndex);
    int height = static_cast<int>(trackHeaders[trackIndex]->height * verticalZoom);

    return juce::Rectangle<int>(0, yPosition, getWidth(), height - RESIZE_HANDLE_HEIGHT);
}

juce::Rectangle<int> TrackHeadersPanel::getResizeHandleArea(int trackIndex) const {
    if (trackIndex < 0 || trackIndex >= trackHeaders.size()) {
        return {};
    }

    int yPosition = getTrackYPosition(trackIndex);
    int height = static_cast<int>(trackHeaders[trackIndex]->height * verticalZoom);

    return juce::Rectangle<int>(0, yPosition + height - RESIZE_HANDLE_HEIGHT, getWidth(),
                                RESIZE_HANDLE_HEIGHT);
}

bool TrackHeadersPanel::isResizeHandleArea(const juce::Point<int>& point, int& trackIndex) const {
    for (int i = 0; i < trackHeaders.size(); ++i) {
        if (getResizeHandleArea(i).contains(point)) {
            trackIndex = i;
            return true;
        }
    }
    return false;
}

void TrackHeadersPanel::updateTrackHeaderLayout() {
    for (size_t i = 0; i < trackHeaders.size(); ++i) {
        auto& header = *trackHeaders[i];
        auto headerArea = getTrackHeaderArea(static_cast<int>(i));

        if (!headerArea.isEmpty()) {
            // Apply indentation based on depth
            int indent = header.depth * INDENT_WIDTH;
            auto contentArea = headerArea.withTrimmedLeft(indent).reduced(5);

            // Top row: collapse button (if group) + name label
            auto topRow = contentArea.removeFromTop(20);

            if (header.isGroup) {
                // Collapse button for groups
                header.collapseButton->setBounds(topRow.removeFromLeft(COLLAPSE_BUTTON_SIZE));
                topRow.removeFromLeft(3);  // Spacing
                header.collapseButton->setVisible(true);
            } else {
                header.collapseButton->setVisible(false);
            }

            header.nameLabel->setBounds(topRow);
            contentArea.removeFromTop(5);  // Spacing

            // Mute and Solo buttons (always visible)
            auto buttonArea = contentArea.removeFromTop(20);
            header.muteButton->setBounds(buttonArea.removeFromLeft(30));
            buttonArea.removeFromLeft(5);  // Spacing
            header.soloButton->setBounds(buttonArea.removeFromLeft(30));

            contentArea.removeFromTop(5);  // Spacing

            // Volume slider - only show if enough space
            if (contentArea.getHeight() >= 20) {
                header.volumeSlider->setBounds(contentArea.removeFromTop(15));
                header.volumeSlider->setVisible(true);
                contentArea.removeFromTop(5);  // Spacing
            } else {
                header.volumeSlider->setVisible(false);
            }

            // Pan slider - only show if enough space
            if (contentArea.getHeight() >= 15) {
                header.panSlider->setBounds(contentArea.removeFromTop(15));
                header.panSlider->setVisible(true);
            } else {
                header.panSlider->setVisible(false);
            }
        }
    }
}

void TrackHeadersPanel::mouseDown(const juce::MouseEvent& event) {
    // Handle vertical track height resizing and track selection
    int trackIndex;
    if (isResizeHandleArea(event.getPosition(), trackIndex)) {
        // Start resizing
        isResizing = true;
        resizingTrackIndex = trackIndex;
        resizeStartY = event.y;
        resizeStartHeight = trackHeaders[trackIndex]->height;
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    } else {
        // Find which track was clicked
        for (int i = 0; i < static_cast<int>(trackHeaders.size()); ++i) {
            if (getTrackHeaderArea(i).contains(event.getPosition())) {
                selectTrack(i);

                // Right-click shows context menu
                if (event.mods.isPopupMenu()) {
                    showContextMenu(i, event.getPosition());
                }
                break;
            }
        }
    }
}

void TrackHeadersPanel::mouseDrag(const juce::MouseEvent& event) {
    // Handle vertical track height resizing
    if (isResizing && resizingTrackIndex >= 0) {
        int deltaY = event.y - resizeStartY;
        int newHeight =
            juce::jlimit(MIN_TRACK_HEIGHT, MAX_TRACK_HEIGHT, resizeStartHeight + deltaY);
        setTrackHeight(resizingTrackIndex, newHeight);
    }
}

void TrackHeadersPanel::mouseUp(const juce::MouseEvent& event) {
    // Handle vertical track height resizing cleanup
    if (isResizing) {
        isResizing = false;
        resizingTrackIndex = -1;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void TrackHeadersPanel::mouseMove(const juce::MouseEvent& event) {
    // Handle vertical track height resizing
    int trackIndex;
    if (isResizeHandleArea(event.getPosition(), trackIndex)) {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    } else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

// Setter methods
void TrackHeadersPanel::setTrackName(int trackIndex, const juce::String& name) {
    if (trackIndex >= 0 && trackIndex < trackHeaders.size()) {
        trackHeaders[trackIndex]->name = name;
        trackHeaders[trackIndex]->nameLabel->setText(name, juce::dontSendNotification);
    }
}

void TrackHeadersPanel::setTrackMuted(int trackIndex, bool muted) {
    if (trackIndex >= 0 && trackIndex < trackHeaders.size()) {
        trackHeaders[trackIndex]->muted = muted;
        trackHeaders[trackIndex]->muteButton->setToggleState(muted, juce::dontSendNotification);
    }
}

void TrackHeadersPanel::setTrackSolo(int trackIndex, bool solo) {
    if (trackIndex >= 0 && trackIndex < trackHeaders.size()) {
        trackHeaders[trackIndex]->solo = solo;
        trackHeaders[trackIndex]->soloButton->setToggleState(solo, juce::dontSendNotification);
    }
}

void TrackHeadersPanel::setTrackVolume(int trackIndex, float volume) {
    if (trackIndex >= 0 && trackIndex < trackHeaders.size()) {
        trackHeaders[trackIndex]->volume = volume;
        trackHeaders[trackIndex]->volumeSlider->setValue(volume, juce::dontSendNotification);
    }
}

void TrackHeadersPanel::setTrackPan(int trackIndex, float pan) {
    if (trackIndex >= 0 && trackIndex < trackHeaders.size()) {
        trackHeaders[trackIndex]->pan = pan;
        trackHeaders[trackIndex]->panSlider->setValue(pan, juce::dontSendNotification);
    }
}

void TrackHeadersPanel::handleCollapseToggle(TrackId trackId) {
    auto& trackManager = TrackManager::getInstance();
    const auto* track = trackManager.getTrack(trackId);
    if (track && track->isGroup()) {
        bool currentlyCollapsed = track->isCollapsedIn(currentViewMode_);
        trackManager.setTrackCollapsed(trackId, currentViewMode_, !currentlyCollapsed);
    }
}

void TrackHeadersPanel::showContextMenu(int trackIndex, juce::Point<int> position) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(trackHeaders.size()))
        return;

    auto& header = *trackHeaders[trackIndex];
    auto& trackManager = TrackManager::getInstance();
    const auto* track = trackManager.getTrack(header.trackId);
    if (!track)
        return;

    juce::PopupMenu menu;

    // Track type info
    menu.addSectionHeader(track->name);
    menu.addSeparator();

    // Group operations
    if (track->isGroup()) {
        // Collapse/expand
        menu.addItem(1, track->isCollapsedIn(currentViewMode_) ? "Expand Group" : "Collapse Group");
        menu.addSeparator();
    }

    // Move to group submenu
    juce::PopupMenu moveToGroupMenu;
    const auto& allTracks = trackManager.getTracks();
    bool hasGroups = false;

    for (const auto& t : allTracks) {
        if (t.isGroup() && t.id != header.trackId) {
            // Don't allow moving a group into its own descendants
            if (track->isGroup()) {
                auto descendants = trackManager.getAllDescendants(header.trackId);
                if (std::find(descendants.begin(), descendants.end(), t.id) != descendants.end())
                    continue;
            }
            moveToGroupMenu.addItem(100 + t.id, t.name);
            hasGroups = true;
        }
    }

    if (hasGroups) {
        menu.addSubMenu("Move to Group", moveToGroupMenu);
    }

    // Remove from group (if track has a parent)
    if (!track->isTopLevel()) {
        menu.addItem(2, "Remove from Group");
    }

    menu.addSeparator();

    // Delete track
    menu.addItem(3, "Delete Track");

    // Show menu and handle result
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
                           localAreaToGlobal(juce::Rectangle<int>(position.x, position.y, 1, 1))),
                       [this, trackId = header.trackId](int result) {
                           if (result == 1) {
                               // Toggle collapse
                               handleCollapseToggle(trackId);
                           } else if (result == 2) {
                               // Remove from group
                               TrackManager::getInstance().removeTrackFromGroup(trackId);
                           } else if (result == 3) {
                               // Delete track
                               TrackManager::getInstance().deleteTrack(trackId);
                           } else if (result >= 100) {
                               // Move to group
                               TrackId groupId = result - 100;
                               TrackManager::getInstance().addTrackToGroup(trackId, groupId);
                           }
                       });
}

}  // namespace magica
