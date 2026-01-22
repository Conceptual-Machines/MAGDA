#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/MacroInfo.hpp"
#include "core/ModInfo.hpp"
#include "core/SelectionManager.hpp"
#include "core/TypeIds.hpp"
#include "ui/components/common/TextSlider.hpp"

namespace magda::daw::ui {

/**
 * @brief A parameter slot with modulation indicator and linking support
 *
 * Displays a parameter name and value, with visual indicators for any
 * mods/macros linked to this parameter.
 *
 * Supports drag-and-drop: drop a ModKnobComponent here to create a link.
 */
class ParamSlotComponent : public juce::Component, public juce::DragAndDropTarget {
  public:
    ParamSlotComponent(int paramIndex);
    ~ParamSlotComponent() override = default;

    void setParamName(const juce::String& name);
    void setParamValue(double value);
    void setFonts(const juce::Font& labelFont, const juce::Font& valueFont);

    // Set the device this param belongs to (for mod/macro lookups)
    void setDeviceId(magda::DeviceId deviceId) {
        deviceId_ = deviceId;
    }

    // Set the device path (for param selection)
    void setDevicePath(const magda::ChainNodePath& path) {
        devicePath_ = path;
    }

    // Set available mods and macros for linking
    void setAvailableMods(const magda::ModArray* mods) {
        availableMods_ = mods;
    }
    void setAvailableMacros(const magda::MacroArray* macros) {
        availableMacros_ = macros;
    }

    // Contextual selection - when set, only shows this mod's link
    void setSelectedModIndex(int modIndex) {
        selectedModIndex_ = modIndex;
        repaint();
    }
    void clearSelectedMod() {
        selectedModIndex_ = -1;
        repaint();
    }
    int getSelectedModIndex() const {
        return selectedModIndex_;
    }

    // Selection state (this param cell is selected)
    void setSelected(bool selected) {
        selected_ = selected;
        repaint();
    }
    bool isSelected() const {
        return selected_;
    }

    // Callbacks
    std::function<void(double)> onValueChanged;
    std::function<void(int modIndex, magda::ModTarget target)> onModLinked;
    std::function<void(int modIndex, magda::ModTarget target, float amount)> onModLinkedWithAmount;
    std::function<void(int modIndex, magda::ModTarget target)> onModUnlinked;
    std::function<void(int modIndex, magda::ModTarget target, float amount)> onModAmountChanged;
    std::function<void(int macroIndex, magda::MacroTarget target)> onMacroLinked;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // DragAndDropTarget
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;

  private:
    int paramIndex_;
    magda::DeviceId deviceId_ = magda::INVALID_DEVICE_ID;
    magda::ChainNodePath devicePath_;  // For param selection
    const magda::ModArray* availableMods_ = nullptr;
    const magda::MacroArray* availableMacros_ = nullptr;
    int selectedModIndex_ = -1;  // -1 means no mod selected (show all)
    bool selected_ = false;      // This param cell is selected

    juce::Label nameLabel_;
    TextSlider valueSlider_{TextSlider::Format::Decimal};

    // Shift+drag state for mod amount editing
    bool isModAmountDrag_ = false;
    float modAmountDragStart_ = 0.0f;
    int modAmountDragY_ = 0;
    int modAmountDragModIndex_ = -1;

    // Amount label shown during Shift+drag
    juce::Label amountLabel_;

    // Drag-and-drop state
    bool isDragOver_ = false;

    // Find mods/macros targeting this param (returns mod index + link pointer)
    // If selectedModIndex_ >= 0, only returns that mod's link (if any)
    std::vector<std::pair<int, const magda::ModLink*>> getLinkedMods() const;
    std::vector<std::pair<int, const magda::MacroInfo*>> getLinkedMacros() const;

    void showLinkMenu();
    void paintModulationIndicators(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamSlotComponent)
};

}  // namespace magda::daw::ui
