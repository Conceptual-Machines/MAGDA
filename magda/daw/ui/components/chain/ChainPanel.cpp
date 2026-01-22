#include "ChainPanel.hpp"

#include <BinaryData.h>

#include "NodeComponent.hpp"
#include "RackComponent.hpp"
#include "core/MacroInfo.hpp"
#include "core/ModInfo.hpp"
#include "core/SelectionManager.hpp"
#include "ui/components/common/SvgButton.hpp"
#include "ui/components/common/TextSlider.hpp"
#include "ui/debug/DebugSettings.hpp"
#include "ui/themes/DarkTheme.hpp"
#include "ui/themes/FontManager.hpp"
#include "ui/themes/SmallButtonLookAndFeel.hpp"

namespace magda::daw::ui {

//==============================================================================
// DeviceSlotComponent - Device display within chain panel (inherits from NodeComponent)
//==============================================================================
class ChainPanel::DeviceSlotComponent : public NodeComponent {
  public:
    static constexpr int BASE_SLOT_WIDTH = 200;  // Base width without panels
    static constexpr int NUM_PARAMS = 16;
    static constexpr int PARAMS_PER_ROW = 4;

    DeviceSlotComponent(ChainPanel& owner, const magda::DeviceInfo& device)
        : owner_(owner), device_(device), gainSlider_(TextSlider::Format::Decibels) {
        setNodeName(device.name);
        setBypassed(device.bypassed);

        // Hide built-in bypass button - we'll add our own in the header
        setBypassButtonVisible(false);

        // Set up callbacks - use path-based methods for proper nesting support
        onDeleteClicked = [this]() {
            magda::TrackManager::getInstance().removeDeviceFromChainByPath(nodePath_);
        };

        // Mod panel toggle updates layout
        onModPanelToggled = [this](bool /*visible*/) { owner_.onDeviceLayoutChanged(); };

        onLayoutChanged = [this]() {
            // Notify ChainPanel to recalculate container size
            owner_.onDeviceLayoutChanged();
        };

        // Mod button (toggle mod panel) - sine wave icon
        modButton_ = std::make_unique<magda::SvgButton>("Mod", BinaryData::sinewavebright_svg,
                                                        BinaryData::sinewavebright_svgSize);
        modButton_->setClickingTogglesState(true);
        modButton_->setNormalColor(DarkTheme::getSecondaryTextColour());
        modButton_->setActiveColor(juce::Colours::white);
        modButton_->setActiveBackgroundColor(DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
        modButton_->onClick = [this]() {
            modButton_->setActive(modButton_->getToggleState());
            // Use inherited method to properly handle editor visibility
            setModPanelVisible(modButton_->getToggleState());
        };
        addAndMakeVisible(*modButton_);

        // Macro button (toggle macro panel) - link icon
        macroButton_ = std::make_unique<magda::SvgButton>("Macro", BinaryData::link_bright_svg,
                                                          BinaryData::link_bright_svgSize);
        macroButton_->setClickingTogglesState(true);
        macroButton_->setNormalColor(DarkTheme::getSecondaryTextColour());
        macroButton_->setActiveColor(juce::Colours::white);
        macroButton_->setActiveBackgroundColor(DarkTheme::getColour(DarkTheme::ACCENT_PURPLE));
        macroButton_->onClick = [this]() {
            macroButton_->setActive(macroButton_->getToggleState());
            // Use inherited method to properly handle editor visibility
            setParamPanelVisible(macroButton_->getToggleState());
        };
        addAndMakeVisible(*macroButton_);

        // Initialize mods/macros panels from base class
        initializeModsMacrosPanels();

        // Gain text slider in header
        gainSlider_.setRange(-60.0, 12.0, 0.1);
        gainSlider_.setValue(device_.gainDb, juce::dontSendNotification);
        gainSlider_.onValueChanged = [this](double value) {
            // Use path-based lookup for proper nesting support
            if (auto* dev = magda::TrackManager::getInstance().getDeviceInChainByPath(nodePath_)) {
                dev->gainDb = static_cast<float>(value);
            }
        };
        addAndMakeVisible(gainSlider_);

        // UI button (open plugin window) - open in new icon
        uiButton_ = std::make_unique<magda::SvgButton>("UI", BinaryData::open_in_new_svg,
                                                       BinaryData::open_in_new_svgSize);
        uiButton_->setNormalColor(DarkTheme::getSecondaryTextColour());
        uiButton_->onClick = [this]() { DBG("Open plugin UI for: " << device_.name); };
        addAndMakeVisible(*uiButton_);

        // Bypass/On button (power icon)
        onButton_ = std::make_unique<magda::SvgButton>("Power", BinaryData::power_on_svg,
                                                       BinaryData::power_on_svgSize);
        onButton_->setClickingTogglesState(true);
        onButton_->setToggleState(!device.bypassed,
                                  juce::dontSendNotification);  // On = not bypassed
        onButton_->setNormalColor(DarkTheme::getColour(DarkTheme::STATUS_ERROR));
        onButton_->setActiveColor(juce::Colours::white);
        onButton_->setActiveBackgroundColor(
            DarkTheme::getColour(DarkTheme::ACCENT_GREEN).darker(0.3f));
        onButton_->setActive(!device.bypassed);
        onButton_->onClick = [this]() {
            bool active = onButton_->getToggleState();
            onButton_->setActive(active);
            setBypassed(!active);  // Active = not bypassed
            // Use path-based method for proper nesting support
            magda::TrackManager::getInstance().setDeviceInChainBypassedByPath(nodePath_, !active);
        };
        addAndMakeVisible(*onButton_);

        // Create inline param sliders with labels (mock params)
        // clang-format off
        static const char* mockParamNames[NUM_PARAMS] = {
            "Cutoff", "Resonance", "Drive", "Mix",
            "Attack", "Decay", "Sustain", "Release",
            "LFO Rate", "LFO Depth", "Feedback", "Width",
            "Low", "Mid", "High", "Output"
        };
        // clang-format on

        for (int i = 0; i < NUM_PARAMS; ++i) {
            paramLabels_[i] = std::make_unique<juce::Label>();
            paramLabels_[i]->setText(mockParamNames[i], juce::dontSendNotification);
            paramLabels_[i]->setFont(FontManager::getInstance().getUIFont(9.0f));
            paramLabels_[i]->setColour(juce::Label::textColourId,
                                       DarkTheme::getSecondaryTextColour());
            paramLabels_[i]->setJustificationType(juce::Justification::centredLeft);
            paramLabels_[i]->setInterceptsMouseClicks(false, false);  // Pass through for selection
            addAndMakeVisible(*paramLabels_[i]);

            paramSliders_[i] = std::make_unique<TextSlider>(TextSlider::Format::Decimal);
            paramSliders_[i]->setRange(0.0, 1.0, 0.01);
            paramSliders_[i]->setValue(0.5, juce::dontSendNotification);
            addAndMakeVisible(*paramSliders_[i]);
        }
    }

    ~DeviceSlotComponent() override = default;

    magda::DeviceId getDeviceId() const {
        return device_.id;
    }

    int getPreferredWidth() const override {
        if (collapsed_) {
            // When collapsed, still add side panel widths
            return getLeftPanelsWidth() + COLLAPSED_WIDTH + getRightPanelsWidth();
        }
        return getTotalWidth(BASE_SLOT_WIDTH);
    }

    void updateFromDevice(const magda::DeviceInfo& device) {
        device_ = device;
        setNodeName(device.name);
        setBypassed(device.bypassed);
        onButton_->setToggleState(!device.bypassed, juce::dontSendNotification);
        onButton_->setActive(!device.bypassed);
        gainSlider_.setValue(device.gainDb, juce::dontSendNotification);
        repaint();
    }

  protected:
    void resizedHeaderExtra(juce::Rectangle<int>& headerArea) override {
        // Header layout: [M] [Macro] [Name...] [gain slider] [UI] [on]
        // Note: delete (X) is handled by NodeComponent on the right

        // Mod button on the left (before name)
        modButton_->setBounds(headerArea.removeFromLeft(BUTTON_SIZE));
        headerArea.removeFromLeft(4);

        // Macro button
        macroButton_->setBounds(headerArea.removeFromLeft(BUTTON_SIZE));
        headerArea.removeFromLeft(4);

        // Power button on the right (before delete which is handled by parent)
        onButton_->setBounds(headerArea.removeFromRight(BUTTON_SIZE));
        headerArea.removeFromRight(4);

        // UI button
        uiButton_->setBounds(headerArea.removeFromRight(BUTTON_SIZE));
        headerArea.removeFromRight(4);

        // Gain slider takes some space on the right
        gainSlider_.setBounds(headerArea.removeFromRight(50));
        headerArea.removeFromRight(4);

        // Remaining space is for the name label (handled by NodeComponent)
    }

    // Devices show mod panel and macro panel as side panels
    int getModPanelWidth() const override {
        return modPanelVisible_ ? DEFAULT_PANEL_WIDTH : 0;
    }
    int getParamPanelWidth() const override {
        return paramPanelVisible_ ? DEFAULT_PANEL_WIDTH : 0;  // Macros panel as param panel
    }
    int getGainPanelWidth() const override {
        return 0;  // Gain in header
    }

    void paintContent(juce::Graphics& g, juce::Rectangle<int> contentArea) override {
        // Manufacturer label at top
        auto labelArea = contentArea.removeFromTop(12);
        auto textColour = isBypassed() ? DarkTheme::getSecondaryTextColour().withAlpha(0.5f)
                                       : DarkTheme::getSecondaryTextColour();
        g.setColour(textColour);
        g.setFont(FontManager::getInstance().getUIFont(8.0f));
        g.drawText(device_.manufacturer, labelArea.reduced(2, 0), juce::Justification::centredLeft);
    }

    void resizedContent(juce::Rectangle<int> contentArea) override {
        // When collapsed, hide content controls only (buttons handled by resizedCollapsed)
        if (collapsed_) {
            for (int i = 0; i < NUM_PARAMS; ++i) {
                paramLabels_[i]->setVisible(false);
                paramSliders_[i]->setVisible(false);
            }
            gainSlider_.setVisible(false);
            return;
        }

        // Show header controls when expanded
        modButton_->setVisible(true);
        macroButton_->setVisible(true);
        uiButton_->setVisible(true);
        onButton_->setVisible(true);
        gainSlider_.setVisible(true);

        // Skip manufacturer label area
        contentArea.removeFromTop(12);
        contentArea = contentArea.reduced(2, 0);

        // Update param fonts from debug settings
        auto labelFont = FontManager::getInstance().getUIFont(
            DebugSettings::getInstance().getParamLabelFontSize());
        auto valueFont = FontManager::getInstance().getUIFont(
            DebugSettings::getInstance().getParamValueFontSize());
        for (int i = 0; i < NUM_PARAMS; ++i) {
            paramLabels_[i]->setFont(labelFont);
            paramSliders_[i]->setFont(valueFont);
            paramLabels_[i]->setVisible(true);
            paramSliders_[i]->setVisible(true);
        }

        // Layout params in a 4-column grid, scaled to fit available space
        int numRows = (NUM_PARAMS + PARAMS_PER_ROW - 1) / PARAMS_PER_ROW;
        int cellWidth = contentArea.getWidth() / PARAMS_PER_ROW;
        int cellHeight = contentArea.getHeight() / numRows;
        int labelHeight = juce::jmin(10, cellHeight / 3);
        int sliderHeight = cellHeight - labelHeight - 2;

        for (int i = 0; i < NUM_PARAMS; ++i) {
            int row = i / PARAMS_PER_ROW;
            int col = i % PARAMS_PER_ROW;
            int x = contentArea.getX() + col * cellWidth;
            int y = contentArea.getY() + row * cellHeight;

            paramLabels_[i]->setBounds(x, y, cellWidth - 2, labelHeight);
            paramSliders_[i]->setBounds(x, y + labelHeight, cellWidth - 2, sliderHeight);
        }
    }

    void resizedCollapsed(juce::Rectangle<int>& area) override {
        // Add device-specific buttons vertically when collapsed
        // Order: X (from base), ON, UI, Mod, Macro
        int buttonSize = juce::jmin(16, area.getWidth() - 4);

        // On/power button (right after X)
        onButton_->setBounds(
            area.removeFromTop(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize));
        onButton_->setVisible(true);
        area.removeFromTop(4);

        // UI button
        uiButton_->setBounds(
            area.removeFromTop(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize));
        uiButton_->setVisible(true);
        area.removeFromTop(4);

        // Mod button
        modButton_->setBounds(
            area.removeFromTop(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize));
        modButton_->setVisible(true);
        area.removeFromTop(4);

        // Macro button
        macroButton_->setBounds(
            area.removeFromTop(buttonSize).withSizeKeepingCentre(buttonSize, buttonSize));
        macroButton_->setVisible(true);
    }

    // === Virtual data provider overrides ===
    const magda::ModArray* getModsData() const override {
        // Use path-based lookup to get current device data
        if (auto* dev = magda::TrackManager::getInstance().getDeviceInChainByPath(nodePath_)) {
            return &dev->mods;
        }
        return nullptr;
    }

    const magda::MacroArray* getMacrosData() const override {
        if (auto* dev = magda::TrackManager::getInstance().getDeviceInChainByPath(nodePath_)) {
            return &dev->macros;
        }
        return nullptr;
    }

    std::vector<std::pair<magda::DeviceId, juce::String>> getAvailableDevices() const override {
        // For device mods/macros, the target is this device itself
        return {{device_.id, device_.name}};
    }

    // === Virtual callback overrides for mod/macro persistence ===
    void onModAmountChangedInternal(int modIndex, float amount) override {
        magda::TrackManager::getInstance().setDeviceModAmount(nodePath_, modIndex, amount);
    }

    void onModTargetChangedInternal(int modIndex, magda::ModTarget target) override {
        magda::TrackManager::getInstance().setDeviceModTarget(nodePath_, modIndex, target);
    }

    void onModNameChangedInternal(int modIndex, const juce::String& name) override {
        magda::TrackManager::getInstance().setDeviceModName(nodePath_, modIndex, name);
    }

    void onModTypeChangedInternal(int modIndex, magda::ModType type) override {
        magda::TrackManager::getInstance().setDeviceModType(nodePath_, modIndex, type);
    }

    void onModRateChangedInternal(int modIndex, float rate) override {
        magda::TrackManager::getInstance().setDeviceModRate(nodePath_, modIndex, rate);
    }

    void onMacroValueChangedInternal(int macroIndex, float value) override {
        magda::TrackManager::getInstance().setDeviceMacroValue(nodePath_, macroIndex, value);
    }

    void onMacroTargetChangedInternal(int macroIndex, magda::MacroTarget target) override {
        magda::TrackManager::getInstance().setDeviceMacroTarget(nodePath_, macroIndex, target);
    }

    void onMacroNameChangedInternal(int macroIndex, const juce::String& name) override {
        magda::TrackManager::getInstance().setDeviceMacroName(nodePath_, macroIndex, name);
    }

    void onModClickedInternal(int modIndex) override {
        magda::SelectionManager::getInstance().selectMod(nodePath_, modIndex);
    }

    void onMacroClickedInternal(int macroIndex) override {
        magda::SelectionManager::getInstance().selectMacro(nodePath_, macroIndex);
    }

    void onModPageAddRequested(int /*itemsToAdd*/) override {
        magda::TrackManager::getInstance().addDeviceModPage(nodePath_);
    }

    void onModPageRemoveRequested(int /*itemsToRemove*/) override {
        magda::TrackManager::getInstance().removeDeviceModPage(nodePath_);
    }

    void onMacroPageAddRequested(int /*itemsToAdd*/) override {
        magda::TrackManager::getInstance().addDeviceMacroPage(nodePath_);
    }

    void onMacroPageRemoveRequested(int /*itemsToRemove*/) override {
        magda::TrackManager::getInstance().removeDeviceMacroPage(nodePath_);
    }

  private:
    ChainPanel& owner_;
    magda::DeviceInfo device_;

    // Header controls
    std::unique_ptr<magda::SvgButton> modButton_;
    std::unique_ptr<magda::SvgButton> macroButton_;
    TextSlider gainSlider_;
    std::unique_ptr<magda::SvgButton> uiButton_;
    std::unique_ptr<magda::SvgButton> onButton_;

    std::unique_ptr<juce::Label> paramLabels_[NUM_PARAMS];
    std::unique_ptr<TextSlider> paramSliders_[NUM_PARAMS];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceSlotComponent)
};

//==============================================================================
// ZoomableViewport - Viewport that supports Cmd+scroll for zooming
//==============================================================================
class ChainPanel::ZoomableViewport : public juce::Viewport {
  public:
    explicit ZoomableViewport(ChainPanel& owner) : owner_(owner) {}

    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override {
        DBG("ZoomableViewport::mouseWheelMove - deltaY="
            << wheel.deltaY << " isCommandDown=" << (event.mods.isCommandDown() ? "yes" : "no"));

        // Cmd/Ctrl + scroll wheel = zoom
        if (event.mods.isCommandDown()) {
            float delta = wheel.deltaY > 0 ? ChainPanel::ZOOM_STEP : -ChainPanel::ZOOM_STEP;
            DBG("  -> Zooming by " << delta << " to " << (owner_.getZoomLevel() + delta));
            owner_.setZoomLevel(owner_.getZoomLevel() + delta);
        } else {
            // Normal scroll - let viewport handle horizontal scrolling
            Viewport::mouseWheelMove(event, wheel);
        }
    }

  private:
    ChainPanel& owner_;
};

//==============================================================================
// ElementSlotsContainer - Custom container that paints arrows between chain elements
//==============================================================================
class ChainPanel::ElementSlotsContainer : public juce::Component, public juce::DragAndDropTarget {
  public:
    explicit ElementSlotsContainer(ChainPanel& owner) : owner_(owner) {}

    void setElementSlots(const std::vector<std::unique_ptr<NodeComponent>>* slots) {
        elementSlots_ = slots;
    }

    void mouseDown(const juce::MouseEvent& /*e*/) override {
        // Click on empty area - clear device selection
        owner_.clearDeviceSelection();
    }

    void mouseMove(const juce::MouseEvent&) override {
        // Check if drop state is stale (drag was cancelled)
        checkAndResetStaleDropState();
    }

    void mouseEnter(const juce::MouseEvent&) override {
        // Check if drop state is stale (drag was cancelled while outside)
        checkAndResetStaleDropState();
    }

    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override {
        DBG("ElementSlotsContainer::mouseWheelMove - deltaY="
            << wheel.deltaY << " isCommandDown=" << (event.mods.isCommandDown() ? "yes" : "no"));

        // Cmd/Ctrl + scroll wheel = zoom
        if (event.mods.isCommandDown()) {
            float delta = wheel.deltaY > 0 ? ChainPanel::ZOOM_STEP : -ChainPanel::ZOOM_STEP;
            owner_.setZoomLevel(owner_.getZoomLevel() + delta);
        } else {
            // Normal scroll - let parent handle it (viewport scrolling)
            Component::mouseWheelMove(event, wheel);
        }
    }

    void paint(juce::Graphics& g) override {
        if (!elementSlots_)
            return;

        // Draw arrows between elements
        int arrowY = getHeight() / 2;

        for (size_t i = 0; i < elementSlots_->size(); ++i) {
            auto& slot = (*elementSlots_)[i];
            int x = slot->getRight();  // Arrow starts after the slot

            // Draw arrow after each element
            g.setColour(DarkTheme::getSecondaryTextColour());
            int arrowStart = x + 4;
            int arrowEnd = x + 12;
            g.drawLine(static_cast<float>(arrowStart), static_cast<float>(arrowY),
                       static_cast<float>(arrowEnd), static_cast<float>(arrowY), 1.5f);
            // Arrow head
            g.drawLine(static_cast<float>(arrowEnd - 4), static_cast<float>(arrowY - 3),
                       static_cast<float>(arrowEnd), static_cast<float>(arrowY), 1.5f);
            g.drawLine(static_cast<float>(arrowEnd - 4), static_cast<float>(arrowY + 3),
                       static_cast<float>(arrowEnd), static_cast<float>(arrowY), 1.5f);
        }

        // Draw insertion indicator during drag (reorder or drop)
        if (owner_.dragInsertIndex_ >= 0 || owner_.dropInsertIndex_ >= 0) {
            int indicatorIndex =
                owner_.dragInsertIndex_ >= 0 ? owner_.dragInsertIndex_ : owner_.dropInsertIndex_;
            int indicatorX = owner_.calculateIndicatorX(indicatorIndex);
            g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_BLUE));
            g.fillRect(indicatorX - 2, 0, 4, getHeight());
        }

        // Draw ghost image during drag
        if (owner_.dragGhostImage_.isValid()) {
            g.setOpacity(0.6f);
            int ghostX = owner_.dragMousePos_.x - owner_.dragGhostImage_.getWidth() / 2;
            int ghostY = owner_.dragMousePos_.y - owner_.dragGhostImage_.getHeight() / 2;
            g.drawImageAt(owner_.dragGhostImage_, ghostX, ghostY);
            g.setOpacity(1.0f);
        }
    }

    // DragAndDropTarget implementation
    bool isInterestedInDragSource(const SourceDetails& details) override {
        // Accept plugin drops if we have a valid chain
        if (!owner_.hasChain_) {
            return false;
        }
        if (auto* obj = details.description.getDynamicObject()) {
            return obj->getProperty("type").toString() == "plugin";
        }
        return false;
    }

    void itemDragEnter(const SourceDetails& details) override {
        owner_.dropInsertIndex_ = owner_.calculateInsertIndex(details.localPosition.x);
        owner_.startTimerHz(10);  // Start timer to detect stale drop state
        owner_.resized();         // Trigger relayout to add left padding
        repaint();
    }

    void itemDragMove(const SourceDetails& details) override {
        owner_.dropInsertIndex_ = owner_.calculateInsertIndex(details.localPosition.x);
        repaint();
    }

    void itemDragExit(const SourceDetails&) override {
        owner_.dropInsertIndex_ = -1;
        owner_.stopTimer();
        owner_.resized();  // Trigger relayout to remove left padding
        repaint();
    }

    void itemDropped(const SourceDetails& details) override {
        if (auto* obj = details.description.getDynamicObject()) {
            // Create DeviceInfo from dropped plugin
            magda::DeviceInfo device;
            device.name = obj->getProperty("name").toString().toStdString();
            device.manufacturer = obj->getProperty("manufacturer").toString().toStdString();
            device.pluginId = obj->getProperty("name").toString().toStdString() + "_" +
                              obj->getProperty("format").toString().toStdString();
            device.isInstrument = static_cast<bool>(obj->getProperty("isInstrument"));

            juce::String format = obj->getProperty("format").toString();
            if (format == "VST3") {
                device.format = magda::PluginFormat::VST3;
            } else if (format == "AU") {
                device.format = magda::PluginFormat::AU;
            } else if (format == "VST") {
                device.format = magda::PluginFormat::VST;
            }

            // Insert at the drop position
            int insertIndex = owner_.dropInsertIndex_ >= 0
                                  ? owner_.dropInsertIndex_
                                  : static_cast<int>(elementSlots_->size());
            magda::TrackManager::getInstance().addDeviceToChainByPath(owner_.chainPath_, device,
                                                                      insertIndex);

            DBG("Dropped plugin: " + juce::String(device.name) + " into chain at index " +
                juce::String(insertIndex));
        }
        owner_.dropInsertIndex_ = -1;
        owner_.stopTimer();
        owner_.resized();  // Trigger relayout to remove left padding
        repaint();
    }

  private:
    void checkAndResetStaleDropState() {
        if (owner_.dropInsertIndex_ >= 0) {
            if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
                if (!container->isDragAndDropActive()) {
                    owner_.dropInsertIndex_ = -1;
                    owner_.resized();
                    repaint();
                }
            }
        }
    }

    ChainPanel& owner_;
    const std::vector<std::unique_ptr<NodeComponent>>* elementSlots_ = nullptr;
};

//==============================================================================
// ChainPanel
//==============================================================================

ChainPanel::ChainPanel()
    : elementViewport_(std::make_unique<ZoomableViewport>(*this)),
      elementSlotsContainer_(std::make_unique<ElementSlotsContainer>(*this)) {
    // No header - controls are on the chain row

    // Listen for debug settings changes
    DebugSettings::getInstance().addListener([this]() {
        // Force all element slots to update their fonts
        for (auto& slot : elementSlots_) {
            slot->resized();
            slot->repaint();
        }
        resized();
        repaint();
    });

    onLayoutChanged = [this]() {
        // Recalculate container size when a slot's size changes (e.g., panel toggle)
        resized();
        repaint();
        if (auto* parent = getParentComponent()) {
            parent->resized();
            parent->repaint();
        }
    };

    // Viewport for horizontal scrolling of element slots
    elementViewport_->setViewedComponent(elementSlotsContainer_.get(), false);
    elementViewport_->setScrollBarsShown(false, true);  // Horizontal only
    addAndMakeVisible(*elementViewport_);

    // Add device button (inside the container, after all slots)
    addDeviceButton_.setButtonText("+");
    addDeviceButton_.setColour(juce::TextButton::buttonColourId,
                               DarkTheme::getColour(DarkTheme::SURFACE));
    addDeviceButton_.setColour(juce::TextButton::textColourOffId,
                               DarkTheme::getSecondaryTextColour());
    addDeviceButton_.onClick = [this]() { onAddDeviceClicked(); };
    addDeviceButton_.setLookAndFeel(&SmallButtonLookAndFeel::getInstance());
    elementSlotsContainer_->addAndMakeVisible(addDeviceButton_);

    setVisible(false);
}

ChainPanel::~ChainPanel() {
    stopTimer();
}

void ChainPanel::paintContent(juce::Graphics& /*g*/, juce::Rectangle<int> /*contentArea*/) {
    // Chain panels no longer have chain-level mods/macros - these are at rack level only
}

void ChainPanel::resizedContent(juce::Rectangle<int> contentArea) {
    // Viewport fills the content area
    elementViewport_->setBounds(contentArea);

    // Calculate total width needed for all element slots
    int totalWidth = calculateTotalContentWidth();
    int containerHeight = contentArea.getHeight();

    // Account for horizontal scrollbar if needed
    if (totalWidth > contentArea.getWidth()) {
        containerHeight = contentArea.getHeight() - 8;  // Space for scrollbar
    }

    // Set container size and update element slots reference for arrow painting
    elementSlotsContainer_->setSize(totalWidth, containerHeight);
    elementSlotsContainer_->setElementSlots(&elementSlots_);

    // Add left padding during drag/drop to show insertion indicator before first element
    bool isDraggingOrDropping = dragOriginalIndex_ >= 0 || dropInsertIndex_ >= 0;
    int x = isDraggingOrDropping ? DRAG_LEFT_PADDING : 0;

    // Layout element slots inside the container with zoom applied
    int scaledArrowWidth = getScaledWidth(ARROW_WIDTH);
    for (auto& slot : elementSlots_) {
        int slotWidth = getScaledWidth(slot->getPreferredWidth());
        slot->setBounds(x, 0, slotWidth, containerHeight);
        x += slotWidth + scaledArrowWidth;
    }

    // Add device button after all slots (not scaled)
    addDeviceButton_.setBounds(x, (containerHeight - 20) / 2, 20, 20);
}

int ChainPanel::calculateTotalContentWidth() const {
    // Add left padding during drag/drop to show insertion indicator before first element
    bool isDraggingOrDropping = dragOriginalIndex_ >= 0 || dropInsertIndex_ >= 0;
    int totalWidth = isDraggingOrDropping ? DRAG_LEFT_PADDING : 0;

    int scaledArrowWidth = getScaledWidth(ARROW_WIDTH);
    for (const auto& slot : elementSlots_) {
        totalWidth += getScaledWidth(slot->getPreferredWidth()) + scaledArrowWidth;
    }
    totalWidth += 30;  // Space for add device button (not scaled)
    return totalWidth;
}

int ChainPanel::getContentWidth() const {
    // Return the total content width (devices + arrows + add button)
    return juce::jmax(300, calculateTotalContentWidth());  // Minimum 300px
}

void ChainPanel::setMaxWidth(int maxWidth) {
    maxWidth_ = maxWidth;
}

void ChainPanel::setZoomLevel(float zoom) {
    DBG("ChainPanel::setZoomLevel - requested=" << zoom << " current=" << zoomLevel_);
    float newZoom = juce::jlimit(MIN_ZOOM, MAX_ZOOM, zoom);
    if (std::abs(zoomLevel_ - newZoom) > 0.001f) {
        zoomLevel_ = newZoom;
        DBG("  -> Zoom changed to " << zoomLevel_);
        resized();
        repaint();
        if (onLayoutChanged) {
            onLayoutChanged();
        }
    } else {
        DBG("  -> No change (clamped to " << newZoom << ")");
    }
}

void ChainPanel::resetZoom() {
    setZoomLevel(1.0f);
}

int ChainPanel::getScaledWidth(int width) const {
    return static_cast<int>(std::round(width * zoomLevel_));
}

void ChainPanel::mouseEnter(const juce::MouseEvent&) {
    DBG("ChainPanel::mouseEnter - visible=" << (isVisible() ? "yes" : "no")
                                            << " bounds=" << getBounds().toString());
}

void ChainPanel::mouseWheelMove(const juce::MouseEvent& event,
                                const juce::MouseWheelDetails& wheel) {
    DBG("ChainPanel::mouseWheelMove - deltaY=" << wheel.deltaY << " isAltDown="
                                               << (event.mods.isAltDown() ? "yes" : "no"));

    // Option/Alt + scroll wheel = zoom (Cmd+scroll is intercepted by macOS)
    if (event.mods.isAltDown()) {
        float delta = wheel.deltaY > 0 ? ZOOM_STEP : -ZOOM_STEP;
        DBG("  -> Zooming to " << (zoomLevel_ + delta));
        setZoomLevel(zoomLevel_ + delta);
    } else {
        // Normal scroll - let viewport handle it
        NodeComponent::mouseWheelMove(event, wheel);
    }
}

void ChainPanel::onDeviceLayoutChanged() {
    // Recalculate container size and relayout
    resized();
    repaint();
    // Notify parent (RackComponent) that our preferred width may have changed
    if (onLayoutChanged) {
        onLayoutChanged();
    }
}

// Show a chain with full path context (for proper nesting)
void ChainPanel::showChain(const magda::ChainNodePath& chainPath) {
    DBG("ChainPanel::showChain - received path with " << chainPath.steps.size() << " steps");
    for (size_t i = 0; i < chainPath.steps.size(); ++i) {
        DBG("  step[" << i << "]: type=" << static_cast<int>(chainPath.steps[i].type)
                      << ", id=" << chainPath.steps[i].id);
    }

    chainPath_ = chainPath;
    trackId_ = chainPath.trackId;

    // Extract rackId and chainId from the path
    // The path should end with a Chain step
    if (!chainPath.steps.empty()) {
        for (const auto& step : chainPath.steps) {
            if (step.type == magda::ChainStepType::Rack) {
                rackId_ = step.id;
            } else if (step.type == magda::ChainStepType::Chain) {
                chainId_ = step.id;
            }
        }
    }

    hasChain_ = true;

    // Update name from chain data (using the top-level rack ID for now)
    // For deeply nested chains, we'd need to walk the path
    auto resolved = magda::TrackManager::getInstance().resolvePath(chainPath);
    DBG("  resolved.valid=" << (resolved.valid ? "yes" : "no")
                            << " resolved.chain=" << (resolved.chain ? "found" : "nullptr"));
    if (resolved.valid && resolved.chain) {
        setNodeName(resolved.chain->name);
        setBypassed(false);  // Chains don't have bypass yet
    }

    rebuildElementSlots();
    setVisible(true);
    resized();
    repaint();
}

// Legacy: show a chain by IDs (computes path internally)
void ChainPanel::showChain(magda::TrackId trackId, magda::RackId rackId, magda::ChainId chainId) {
    // Build the path from IDs and call the path-based version
    auto chainPath = magda::ChainNodePath::chain(trackId, rackId, chainId);
    showChain(chainPath);
}

void ChainPanel::refresh() {
    if (!hasChain_)
        return;

    // Update name from chain data using path-based resolution
    auto resolved = magda::TrackManager::getInstance().resolvePath(chainPath_);
    if (resolved.valid && resolved.chain) {
        setNodeName(resolved.chain->name);
    }

    rebuildElementSlots();
    resized();
    repaint();
}

void ChainPanel::clear() {
    DBG("ChainPanel::clear() called - chainId=" << chainId_ << " rackId=" << rackId_);
    // Unfocus any child components before destroying them to prevent use-after-free
    unfocusAllComponents();

    hasChain_ = false;
    elementSlots_.clear();
    setVisible(false);
}

void ChainPanel::rebuildElementSlots() {
    if (!hasChain_) {
        unfocusAllComponents();
        elementSlots_.clear();
        return;
    }

    // Use path-based resolution to support nested chains at any depth
    auto resolved = magda::TrackManager::getInstance().resolvePath(chainPath_);
    if (!resolved.valid || !resolved.chain) {
        DBG("ChainPanel::rebuildElementSlots - chain not found via path!");
        unfocusAllComponents();
        elementSlots_.clear();
        return;
    }
    const auto* chain = resolved.chain;

    // Smart rebuild: preserve existing slots, only add/remove as needed
    std::vector<std::unique_ptr<NodeComponent>> newSlots;

    for (const auto& element : chain->elements) {
        if (magda::isDevice(element)) {
            const auto& device = magda::getDevice(element);

            // Check if we already have a slot for this device
            DeviceSlotComponent* existingDeviceSlot = nullptr;
            size_t existingIndex = 0;
            for (size_t i = 0; i < elementSlots_.size(); ++i) {
                if (auto* deviceSlot = dynamic_cast<DeviceSlotComponent*>(elementSlots_[i].get())) {
                    if (deviceSlot->getDeviceId() == device.id) {
                        existingDeviceSlot = deviceSlot;
                        existingIndex = i;
                        break;
                    }
                }
            }

            if (existingDeviceSlot) {
                // Found existing slot - preserve it and update its data
                existingDeviceSlot->updateFromDevice(device);
                existingDeviceSlot->setNodePath(chainPath_.withDevice(device.id));
                newSlots.push_back(std::move(elementSlots_[existingIndex]));
                elementSlots_.erase(elementSlots_.begin() + static_cast<long>(existingIndex));
            } else {
                // Create new slot for new device
                auto slot = std::make_unique<DeviceSlotComponent>(*this, device);
                // Set the full path - this is used by all path-based operations
                slot->setNodePath(chainPath_.withDevice(device.id));
                elementSlotsContainer_->addAndMakeVisible(*slot);
                newSlots.push_back(std::move(slot));
            }
        } else if (magda::isRack(element)) {
            const auto& rack = magda::getRack(element);

            // Build the path for this nested rack
            auto nestedRackPath = chainPath_.withRack(rack.id);
            DBG("ChainPanel::rebuildElementSlots - creating nestedRackPath for rack id="
                << rack.id);
            DBG("  chainPath_ has " << chainPath_.steps.size()
                                    << " steps, trackId=" << chainPath_.trackId);
            DBG("  nestedRackPath has " << nestedRackPath.steps.size() << " steps");

            // Check if we already have a RackComponent for this rack
            RackComponent* existingRackComp = nullptr;
            size_t existingIndex = 0;
            for (size_t i = 0; i < elementSlots_.size(); ++i) {
                if (auto* rackComp = dynamic_cast<RackComponent*>(elementSlots_[i].get())) {
                    if (rackComp->getRackId() == rack.id) {
                        existingRackComp = rackComp;
                        existingIndex = i;
                        break;
                    }
                }
            }

            if (existingRackComp) {
                // Found existing RackComponent - preserve it and update its data
                existingRackComp->updateFromRack(rack);
                existingRackComp->setNodePath(nestedRackPath);
                newSlots.push_back(std::move(elementSlots_[existingIndex]));
                elementSlots_.erase(elementSlots_.begin() + static_cast<long>(existingIndex));
            } else {
                // Create new RackComponent for nested rack (with path context)
                auto rackComp = std::make_unique<RackComponent>(nestedRackPath, rack);
                rackComp->setNodePath(nestedRackPath);
                rackComp->onLayoutChanged = [this]() { onDeviceLayoutChanged(); };
                elementSlotsContainer_->addAndMakeVisible(*rackComp);
                newSlots.push_back(std::move(rackComp));
            }
        }
    }

    // Unfocus before destroying remaining old slots (elements that were removed)
    if (!elementSlots_.empty()) {
        unfocusAllComponents();
    }

    // Move new slots to member variable (old slots are destroyed here)
    elementSlots_ = std::move(newSlots);

    // Wire up drag-to-reorder callbacks for all element slots
    // Use SafePointer because moveElementInChainByPath triggers rebuild which may destroy this
    auto safeThis = juce::Component::SafePointer<ChainPanel>(this);

    for (auto& slot : elementSlots_) {
        // Wire up zoom callback (Cmd+scroll on any node forwards to ChainPanel)
        slot->onZoomDelta = [safeThis](float delta) {
            if (safeThis != nullptr) {
                safeThis->setZoomLevel(safeThis->getZoomLevel() + delta);
            }
        };

        slot->onDragStart = [safeThis](NodeComponent* node, const juce::MouseEvent&) {
            if (safeThis == nullptr)
                return;
            safeThis->draggedElement_ = node;
            safeThis->dragOriginalIndex_ = safeThis->findElementIndex(node);
            safeThis->dragInsertIndex_ = safeThis->dragOriginalIndex_;
            // Capture ghost image and make original semi-transparent
            safeThis->dragGhostImage_ = node->createComponentSnapshot(node->getLocalBounds());
            node->setAlpha(0.4f);
            safeThis->startTimerHz(10);  // Start timer to detect stale drag state
            // Re-layout to add left padding for drop indicator
            safeThis->resized();
        };

        slot->onDragMove = [safeThis](NodeComponent*, const juce::MouseEvent& e) {
            if (safeThis == nullptr)
                return;
            auto pos = e.getEventRelativeTo(safeThis->elementSlotsContainer_.get()).getPosition();
            safeThis->dragInsertIndex_ = safeThis->calculateInsertIndex(pos.x);
            safeThis->dragMousePos_ = pos;
            safeThis->elementSlotsContainer_->repaint();
        };

        slot->onDragEnd = [safeThis](NodeComponent* node, const juce::MouseEvent&) {
            if (safeThis == nullptr)
                return;

            // Restore alpha and clear ghost
            node->setAlpha(1.0f);
            safeThis->dragGhostImage_ = juce::Image();
            safeThis->stopTimer();

            int elementCount = static_cast<int>(safeThis->elementSlots_.size());
            if (safeThis->dragOriginalIndex_ >= 0 && safeThis->dragInsertIndex_ >= 0 &&
                safeThis->dragOriginalIndex_ != safeThis->dragInsertIndex_) {
                // Convert insert position to target index
                int targetIndex = safeThis->dragInsertIndex_;
                if (safeThis->dragInsertIndex_ > safeThis->dragOriginalIndex_) {
                    targetIndex = safeThis->dragInsertIndex_ - 1;
                }
                targetIndex = juce::jlimit(0, elementCount - 1, targetIndex);
                if (targetIndex != safeThis->dragOriginalIndex_) {
                    // Capture chainPath before the move (in case safeThis becomes invalid)
                    auto chainPath = safeThis->chainPath_;
                    int fromIndex = safeThis->dragOriginalIndex_;

                    // Clear state before the move (which triggers rebuild)
                    safeThis->draggedElement_ = nullptr;
                    safeThis->dragOriginalIndex_ = -1;
                    safeThis->dragInsertIndex_ = -1;

                    // Perform the move - this may destroy safeThis
                    magda::TrackManager::getInstance().moveElementInChainByPath(
                        chainPath, fromIndex, targetIndex);
                    return;  // Don't access safeThis after this point
                }
            }

            // Only reached if no move happened
            safeThis->draggedElement_ = nullptr;
            safeThis->dragOriginalIndex_ = -1;
            safeThis->dragInsertIndex_ = -1;
            // Re-layout and repaint to remove left padding and indicator
            safeThis->resized();
            safeThis->elementSlotsContainer_->repaint();
        };
    }
}

void ChainPanel::onAddDeviceClicked() {
    if (!hasChain_)
        return;

    juce::PopupMenu menu;

    // Devices submenu
    juce::PopupMenu devicesMenu;
    devicesMenu.addItem(1, "Pro-Q 3");
    devicesMenu.addItem(2, "Pro-C 2");
    devicesMenu.addItem(3, "Saturn 2");
    devicesMenu.addItem(4, "Valhalla Room");
    devicesMenu.addItem(5, "Serum");
    menu.addSubMenu("Add Device", devicesMenu);

    menu.addSeparator();
    menu.addItem(100, "Create Rack");

    // Use SafePointer to handle case where this component is destroyed before callback
    auto safeThis = juce::Component::SafePointer<ChainPanel>(this);
    auto chainPath = chainPath_;  // Capture by value for async safety

    menu.showMenuAsync(juce::PopupMenu::Options(), [safeThis, chainPath](int result) {
        if (result == 100) {
            // Create nested rack using path-based method for proper nesting support
            magda::TrackManager::getInstance().addRackToChainByPath(chainPath);
            if (safeThis != nullptr) {
                safeThis->rebuildElementSlots();
                safeThis->resized();
                safeThis->repaint();
            }
        } else if (result > 0 && result < 100) {
            magda::DeviceInfo device;
            switch (result) {
                case 1:
                    device.name = "Pro-Q 3";
                    device.manufacturer = "FabFilter";
                    break;
                case 2:
                    device.name = "Pro-C 2";
                    device.manufacturer = "FabFilter";
                    break;
                case 3:
                    device.name = "Saturn 2";
                    device.manufacturer = "FabFilter";
                    break;
                case 4:
                    device.name = "Valhalla Room";
                    device.manufacturer = "Valhalla DSP";
                    break;
                case 5:
                    device.name = "Serum";
                    device.manufacturer = "Xfer Records";
                    break;
            }
            device.format = magda::PluginFormat::VST3;
            magda::TrackManager::getInstance().addDeviceToChainByPath(chainPath, device);
            if (safeThis != nullptr) {
                safeThis->rebuildElementSlots();
                safeThis->resized();
                safeThis->repaint();
            }
        }
    });
}

void ChainPanel::clearDeviceSelection() {
    selectedDeviceId_ = magda::INVALID_DEVICE_ID;
    for (auto& slot : elementSlots_) {
        slot->setSelected(false);
    }
    if (onDeviceSelected) {
        onDeviceSelected(magda::INVALID_DEVICE_ID);
    }
    // Clear centralized selection so re-selecting the same node works
    magda::SelectionManager::getInstance().clearChainNodeSelection();
}

void ChainPanel::onDeviceSlotSelected(magda::DeviceId deviceId) {
    // Exclusive selection - deselect all others
    selectedDeviceId_ = deviceId;
    for (auto& slot : elementSlots_) {
        if (auto* deviceSlot = dynamic_cast<DeviceSlotComponent*>(slot.get())) {
            deviceSlot->setSelected(deviceSlot->getDeviceId() == deviceId);
        } else {
            slot->setSelected(false);
        }
    }
    if (onDeviceSelected) {
        onDeviceSelected(deviceId);
    }
}

int ChainPanel::findElementIndex(NodeComponent* element) const {
    for (size_t i = 0; i < elementSlots_.size(); ++i) {
        if (elementSlots_[i].get() == element) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int ChainPanel::calculateInsertIndex(int mouseX) const {
    // Find insert position based on mouse X and element midpoints
    for (size_t i = 0; i < elementSlots_.size(); ++i) {
        int midX = elementSlots_[i]->getX() + elementSlots_[i]->getWidth() / 2;
        if (mouseX < midX) {
            return static_cast<int>(i);
        }
    }
    // After last element
    return static_cast<int>(elementSlots_.size());
}

int ChainPanel::calculateIndicatorX(int index) const {
    // Before first element - center in the drag padding area
    if (index == 0) {
        return DRAG_LEFT_PADDING / 2;
    }

    // After previous element (use scaled arrow width)
    if (index > 0 && index <= static_cast<int>(elementSlots_.size())) {
        int scaledArrowWidth = getScaledWidth(ARROW_WIDTH);
        return elementSlots_[index - 1]->getRight() + scaledArrowWidth / 2;
    }

    // Fallback
    return DRAG_LEFT_PADDING / 2;
}

void ChainPanel::timerCallback() {
    // Check if internal drag state is stale (drag was cancelled)
    if (dragInsertIndex_ >= 0 || draggedElement_ != nullptr) {
        // Check if any mouse button is still down - if not, the drag was cancelled
        if (!juce::Desktop::getInstance().getMainMouseSource().isDragging()) {
            if (draggedElement_) {
                draggedElement_->setAlpha(1.0f);
            }
            draggedElement_ = nullptr;
            dragOriginalIndex_ = -1;
            dragInsertIndex_ = -1;
            dragGhostImage_ = juce::Image();
            stopTimer();
            resized();
            elementSlotsContainer_->repaint();
            return;
        }
    }

    // Check if external drop state is stale (drag was cancelled)
    if (dropInsertIndex_ >= 0) {
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(
                elementSlotsContainer_.get())) {
            if (!container->isDragAndDropActive()) {
                dropInsertIndex_ = -1;
                stopTimer();
                resized();
                elementSlotsContainer_->repaint();
                return;
            }
        }
    }

    // No stale state, stop the timer
    if (dragInsertIndex_ < 0 && draggedElement_ == nullptr && dropInsertIndex_ < 0) {
        stopTimer();
    }
}

}  // namespace magda::daw::ui
