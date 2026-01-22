#include "ParamSlotComponent.hpp"

#include "ui/themes/DarkTheme.hpp"
#include "ui/themes/FontManager.hpp"

namespace magda::daw::ui {

ParamSlotComponent::ParamSlotComponent(int paramIndex) : paramIndex_(paramIndex) {
    nameLabel_.setJustificationType(juce::Justification::centredLeft);
    nameLabel_.setColour(juce::Label::textColourId, DarkTheme::getSecondaryTextColour());
    nameLabel_.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(nameLabel_);

    valueSlider_.setRange(0.0, 1.0, 0.01);
    valueSlider_.setValue(0.5, juce::dontSendNotification);
    valueSlider_.onValueChanged = [this](double value) {
        if (onValueChanged) {
            onValueChanged(value);
        }
    };
    valueSlider_.onClicked = [this]() {
        // Regular click (no Shift): select this param
        if (devicePath_.isValid()) {
            magda::SelectionManager::getInstance().selectParam(devicePath_, paramIndex_);
        }
    };
    valueSlider_.onRightClicked = [this]() {
        // Show link menu on right-click
        showLinkMenu();
    };

    // Amount label for Shift+drag feedback
    amountLabel_.setFont(FontManager::getInstance().getUIFont(10.0f));
    amountLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    amountLabel_.setColour(juce::Label::backgroundColourId,
                           DarkTheme::getColour(DarkTheme::ACCENT_ORANGE).withAlpha(0.9f));
    amountLabel_.setJustificationType(juce::Justification::centred);
    amountLabel_.setVisible(false);
    addAndMakeVisible(amountLabel_);

    // Shift+drag: edit mod amount when a mod is selected
    valueSlider_.onShiftDragStart = [this](float /*startValue*/) {
        if (selectedModIndex_ < 0 || !availableMods_ ||
            selectedModIndex_ >= static_cast<int>(availableMods_->size())) {
            return;
        }

        const auto& selectedMod = (*availableMods_)[static_cast<size_t>(selectedModIndex_)];
        magda::ModTarget thisTarget{deviceId_, paramIndex_};

        // Check if already linked
        const auto* existingLink = selectedMod.getLink(thisTarget);
        bool isLinked = existingLink != nullptr || (selectedMod.target.deviceId == deviceId_ &&
                                                    selectedMod.target.paramIndex == paramIndex_);

        float startAmount = 0.5f;
        if (!isLinked) {
            // Create link at 50%
            if (onModLinkedWithAmount) {
                onModLinkedWithAmount(selectedModIndex_, thisTarget, 0.5f);
            }
        } else {
            // Use existing amount as start value
            startAmount = existingLink ? existingLink->amount : selectedMod.amount;
        }
        valueSlider_.setShiftDragStartValue(startAmount);

        isModAmountDrag_ = true;
        modAmountDragModIndex_ = selectedModIndex_;

        // Show amount label
        int percent = static_cast<int>(startAmount * 100);
        amountLabel_.setText(juce::String(percent) + "%", juce::dontSendNotification);
        amountLabel_.setBounds(getLocalBounds().withHeight(14).translated(0, -16));
        amountLabel_.setVisible(true);
    };

    valueSlider_.onShiftDrag = [this](float newAmount) {
        if (!isModAmountDrag_ || modAmountDragModIndex_ < 0) {
            return;
        }
        magda::ModTarget thisTarget{deviceId_, paramIndex_};
        if (onModAmountChanged) {
            onModAmountChanged(modAmountDragModIndex_, thisTarget, newAmount);
        }

        // Update amount label
        int percent = static_cast<int>(newAmount * 100);
        amountLabel_.setText(juce::String(percent) + "%", juce::dontSendNotification);

        repaint();
    };

    valueSlider_.onShiftDragEnd = [this]() {
        isModAmountDrag_ = false;
        modAmountDragModIndex_ = -1;
        amountLabel_.setVisible(false);
    };

    valueSlider_.onShiftClicked = [this]() {
        // Shift+click (no drag): just create link at 50% if not linked
        if (selectedModIndex_ < 0 || !availableMods_ ||
            selectedModIndex_ >= static_cast<int>(availableMods_->size())) {
            return;
        }

        const auto& selectedMod = (*availableMods_)[static_cast<size_t>(selectedModIndex_)];
        magda::ModTarget thisTarget{deviceId_, paramIndex_};

        const auto* existingLink = selectedMod.getLink(thisTarget);
        bool isLinked = existingLink != nullptr || (selectedMod.target.deviceId == deviceId_ &&
                                                    selectedMod.target.paramIndex == paramIndex_);

        if (!isLinked && onModLinkedWithAmount) {
            onModLinkedWithAmount(selectedModIndex_, thisTarget, 0.5f);
            repaint();
        }
    };

    // Disable right-click editing - we use right-click for link menu
    valueSlider_.setRightClickEditsText(false);
    addAndMakeVisible(valueSlider_);

    setInterceptsMouseClicks(true, true);
}

void ParamSlotComponent::setParamName(const juce::String& name) {
    nameLabel_.setText(name, juce::dontSendNotification);
}

void ParamSlotComponent::setParamValue(double value) {
    valueSlider_.setValue(value, juce::dontSendNotification);
}

void ParamSlotComponent::setFonts(const juce::Font& labelFont, const juce::Font& valueFont) {
    nameLabel_.setFont(labelFont);
    valueSlider_.setFont(valueFont);
}

void ParamSlotComponent::paint(juce::Graphics& /*g*/) {
    // Selection highlight is drawn in paintOverChildren() so it appears on top
}

void ParamSlotComponent::paintOverChildren(juce::Graphics& g) {
    // Draw drag-over highlight (orange border when a mod is being dragged over)
    if (isDragOver_) {
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_ORANGE).withAlpha(0.3f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_ORANGE));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 2.0f);
    }
    // Draw selection highlight on top of children
    else if (selected_) {
        g.setColour(juce::Colour(0xff888888).withAlpha(0.15f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
        g.setColour(juce::Colour(0xff888888));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
    }

    // Draw value indicator line at bottom of slider area
    auto sliderBounds = valueSlider_.getBounds();
    int indicatorHeight = 3;
    int indicatorY = sliderBounds.getBottom() - indicatorHeight - 1;
    int totalWidth = sliderBounds.getWidth() - 4;
    int x = sliderBounds.getX() + 2;

    // Draw current value as a grey line
    double value = valueSlider_.getValue();
    int barWidth = static_cast<int>(totalWidth * value);
    g.setColour(juce::Colour(0xff888888).withAlpha(0.6f));  // Grey
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(indicatorY),
                           static_cast<float>(barWidth), static_cast<float>(indicatorHeight), 1.5f);

    // Draw modulation indicators (stacked above value line)
    paintModulationIndicators(g);
}

void ParamSlotComponent::resized() {
    auto bounds = getLocalBounds();

    // Label takes top portion
    int labelHeight = juce::jmin(12, getHeight() / 3);
    nameLabel_.setBounds(bounds.removeFromTop(labelHeight));

    // Value slider takes the rest
    valueSlider_.setBounds(bounds);
}

void ParamSlotComponent::mouseDown(const juce::MouseEvent& e) {
    // Handle right-click anywhere on the component
    if (e.mods.isPopupMenu()) {
        showLinkMenu();
        return;
    }

    // Regular click on label area (not slider): select param
    if (e.mods.isLeftButtonDown() && !e.mods.isShiftDown() &&
        !valueSlider_.getBounds().contains(e.getPosition())) {
        if (devicePath_.isValid()) {
            magda::SelectionManager::getInstance().selectParam(devicePath_, paramIndex_);
        }
    }
    // Note: Shift+drag and regular drag on slider are handled by valueSlider_ callbacks
}

void ParamSlotComponent::mouseDrag(const juce::MouseEvent& /*e*/) {
    // Drag handling is done by valueSlider_ callbacks
}

void ParamSlotComponent::mouseUp(const juce::MouseEvent& /*e*/) {
    // Mouse up handling is done by valueSlider_ callbacks
}

std::vector<std::pair<int, const magda::ModLink*>> ParamSlotComponent::getLinkedMods() const {
    std::vector<std::pair<int, const magda::ModLink*>> linked;
    if (!availableMods_ || deviceId_ == magda::INVALID_DEVICE_ID) {
        return linked;
    }

    magda::ModTarget thisTarget{deviceId_, paramIndex_};

    // If a mod is selected, only check that specific mod
    if (selectedModIndex_ >= 0 && selectedModIndex_ < static_cast<int>(availableMods_->size())) {
        const auto& mod = (*availableMods_)[static_cast<size_t>(selectedModIndex_)];
        if (const auto* link = mod.getLink(thisTarget)) {
            linked.push_back({selectedModIndex_, link});
        }
        // Legacy check
        else if (mod.target.deviceId == deviceId_ && mod.target.paramIndex == paramIndex_) {
            static magda::ModLink legacyLink;
            legacyLink.target = mod.target;
            legacyLink.amount = mod.amount;
            linked.push_back({selectedModIndex_, &legacyLink});
        }
        return linked;
    }

    // No mod selected - show all linked mods
    for (size_t i = 0; i < availableMods_->size(); ++i) {
        const auto& mod = (*availableMods_)[i];
        // Check new links vector
        if (const auto* link = mod.getLink(thisTarget)) {
            linked.push_back({static_cast<int>(i), link});
        }
        // Legacy: also check old target field
        else if (mod.target.deviceId == deviceId_ && mod.target.paramIndex == paramIndex_) {
            // Create a temporary link for legacy data
            static magda::ModLink legacyLink;
            legacyLink.target = mod.target;
            legacyLink.amount = mod.amount;
            linked.push_back({static_cast<int>(i), &legacyLink});
        }
    }
    return linked;
}

std::vector<std::pair<int, const magda::MacroInfo*>> ParamSlotComponent::getLinkedMacros() const {
    std::vector<std::pair<int, const magda::MacroInfo*>> linked;
    if (!availableMacros_ || deviceId_ == magda::INVALID_DEVICE_ID) {
        return linked;
    }

    for (size_t i = 0; i < availableMacros_->size(); ++i) {
        const auto& macro = (*availableMacros_)[i];
        if (macro.target.deviceId == deviceId_ && macro.target.paramIndex == paramIndex_) {
            linked.push_back({static_cast<int>(i), &macro});
        }
    }
    return linked;
}

void ParamSlotComponent::paintModulationIndicators(juce::Graphics& g) {
    auto linkedMods = getLinkedMods();
    auto linkedMacros = getLinkedMacros();

    if (linkedMods.empty() && linkedMacros.empty()) {
        return;
    }

    // Draw indicators stacked above the value line
    auto sliderBounds = valueSlider_.getBounds();
    int indicatorHeight = 3;
    // Start above the value line (which is at bottom - indicatorHeight - 1)
    int indicatorY = sliderBounds.getBottom() - (indicatorHeight * 2) - 2;
    int totalWidth = sliderBounds.getWidth() - 4;
    int x = sliderBounds.getX() + 2;

    // Draw mod indicators (orange) - use mod.amount, not link->amount
    for (const auto& [modIndex, link] : linkedMods) {
        // Get the mod's amount (not the per-link amount)
        float modAmount = 0.5f;
        if (availableMods_ && modIndex < static_cast<int>(availableMods_->size())) {
            modAmount = (*availableMods_)[static_cast<size_t>(modIndex)].amount;
        }
        int barWidth = static_cast<int>(totalWidth * modAmount);
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_ORANGE).withAlpha(0.8f));
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(indicatorY),
                               static_cast<float>(barWidth), static_cast<float>(indicatorHeight),
                               1.5f);
        indicatorY -= (indicatorHeight + 1);  // Stack multiple indicators
    }

    // Draw macro indicators (purple)
    for (const auto& [macroIndex, macro] : linkedMacros) {
        int barWidth = static_cast<int>(totalWidth * macro->value);
        g.setColour(DarkTheme::getColour(DarkTheme::ACCENT_PURPLE).withAlpha(0.8f));
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(indicatorY),
                               static_cast<float>(barWidth), static_cast<float>(indicatorHeight),
                               1.5f);
        indicatorY -= (indicatorHeight + 1);
    }
}

void ParamSlotComponent::showLinkMenu() {
    juce::PopupMenu menu;

    magda::ModTarget thisTarget{deviceId_, paramIndex_};

    // ========================================================================
    // Contextual mode: if a mod is selected, show simple link/unlink options
    // ========================================================================
    if (selectedModIndex_ >= 0 && availableMods_ &&
        selectedModIndex_ < static_cast<int>(availableMods_->size())) {
        const auto& selectedMod = (*availableMods_)[static_cast<size_t>(selectedModIndex_)];
        juce::String modName = selectedMod.name;

        // Check if already linked
        const auto* existingLink = selectedMod.getLink(thisTarget);
        bool isLinked = existingLink != nullptr || (selectedMod.target.deviceId == deviceId_ &&
                                                    selectedMod.target.paramIndex == paramIndex_);

        if (isLinked) {
            float currentAmount = existingLink ? existingLink->amount : selectedMod.amount;
            int amountPercent = static_cast<int>(currentAmount * 100);
            menu.addSectionHeader(modName + " (" + juce::String(amountPercent) + "%)");
            menu.addItem(1, "Unlink from " + modName);
        } else {
            menu.addSectionHeader(modName);
            menu.addItem(2, "Link to " + modName + " (50%)");
        }

        // Show contextual menu
        auto safeThis = juce::Component::SafePointer<ParamSlotComponent>(this);
        auto deviceId = deviceId_;
        auto paramIdx = paramIndex_;
        auto modIndex = selectedModIndex_;

        menu.showMenuAsync(juce::PopupMenu::Options(),
                           [safeThis, deviceId, paramIdx, modIndex](int result) {
                               if (safeThis == nullptr || result == 0) {
                                   return;
                               }

                               magda::ModTarget target{deviceId, paramIdx};

                               if (result == 1) {
                                   // Unlink
                                   if (safeThis->onModUnlinked) {
                                       safeThis->onModUnlinked(modIndex, target);
                                   }
                                   safeThis->repaint();
                               } else if (result == 2) {
                                   // Link with default amount (50%)
                                   if (safeThis->onModLinkedWithAmount) {
                                       safeThis->onModLinkedWithAmount(modIndex, target, 0.5f);
                                   }
                                   safeThis->repaint();
                               }
                           });
        return;
    }

    // ========================================================================
    // Full menu: no mod selected - show all options
    // ========================================================================
    auto linkedMods = getLinkedMods();
    auto linkedMacros = getLinkedMacros();

    // Section: Currently linked mods - unlink option only (Shift+drag to edit amount)
    if (!linkedMods.empty() || !linkedMacros.empty()) {
        menu.addSectionHeader("Currently Linked");

        for (const auto& [modIndex, link] : linkedMods) {
            juce::String modName = "Mod " + juce::String(modIndex + 1);
            if (availableMods_ && modIndex < static_cast<int>(availableMods_->size())) {
                modName = (*availableMods_)[static_cast<size_t>(modIndex)].name;
            }
            int currentAmountPercent = static_cast<int>(link->amount * 100);
            juce::String label = modName + " (" + juce::String(currentAmountPercent) + "%)";
            menu.addItem(1500 + modIndex, "Unlink " + label);
        }

        for (const auto& [macroIndex, macro] : linkedMacros) {
            menu.addItem(2000 + macroIndex, "Unlink from " + macro->name + " (Macro)");
        }

        menu.addSeparator();
    }

    // Section: Link to Mod
    if (availableMods_ && !availableMods_->empty()) {
        juce::PopupMenu modsMenu;
        for (size_t i = 0; i < availableMods_->size(); ++i) {
            const auto& mod = (*availableMods_)[i];
            bool alreadyLinked =
                mod.getLink(thisTarget) != nullptr ||
                (mod.target.deviceId == deviceId_ && mod.target.paramIndex == paramIndex_);

            if (!alreadyLinked) {
                modsMenu.addItem(3000 + static_cast<int>(i), mod.name);
            }
        }
        if (modsMenu.getNumItems() > 0) {
            menu.addSubMenu("Link to Mod", modsMenu);
        }
    }

    // Section: Link to Macro
    if (availableMacros_ && !availableMacros_->empty()) {
        juce::PopupMenu macrosMenu;
        for (size_t i = 0; i < availableMacros_->size(); ++i) {
            const auto& macro = (*availableMacros_)[i];
            bool alreadyLinked =
                (macro.target.deviceId == deviceId_ && macro.target.paramIndex == paramIndex_);
            macrosMenu.addItem(4000 + static_cast<int>(i), macro.name, !alreadyLinked,
                               alreadyLinked);
        }
        menu.addSubMenu("Link to Macro", macrosMenu);
    }

    // Show full menu
    auto safeThis = juce::Component::SafePointer<ParamSlotComponent>(this);
    auto deviceId = deviceId_;
    auto paramIdx = paramIndex_;
    auto linkedModsCopy = linkedMods;  // Copy for use in lambda

    menu.showMenuAsync(juce::PopupMenu::Options(), [safeThis, deviceId, paramIdx](int result) {
        if (safeThis == nullptr || result == 0) {
            return;
        }

        magda::ModTarget target{deviceId, paramIdx};

        if (result >= 1500 && result < 2000) {
            // Unlink from mod
            int modIndex = result - 1500;
            if (safeThis->onModUnlinked) {
                safeThis->onModUnlinked(modIndex, target);
            }
        } else if (result >= 2000 && result < 3000) {
            // Unlink from macro
            int macroIndex = result - 2000;
            if (safeThis->onMacroLinked) {
                safeThis->onMacroLinked(macroIndex, magda::MacroTarget{});
            }
        } else if (result >= 3000 && result < 4000) {
            // Link to mod at 50%
            int modIndex = result - 3000;
            if (safeThis->onModLinkedWithAmount) {
                safeThis->onModLinkedWithAmount(modIndex, target, 0.5f);
            }
        } else if (result >= 4000 && result < 5000) {
            // Link to macro
            int macroIndex = result - 4000;
            if (safeThis->onMacroLinked) {
                magda::MacroTarget macroTarget;
                macroTarget.deviceId = deviceId;
                macroTarget.paramIndex = paramIdx;
                safeThis->onMacroLinked(macroIndex, macroTarget);
            }
        }

        safeThis->repaint();
    });
}

// ============================================================================
// DragAndDropTarget
// ============================================================================

bool ParamSlotComponent::isInterestedInDragSource(const SourceDetails& details) {
    // Accept drags from mod knobs (description starts with "mod_drag:")
    auto desc = details.description.toString();
    return desc.startsWith("mod_drag:");
}

void ParamSlotComponent::itemDragEnter(const SourceDetails& /*details*/) {
    isDragOver_ = true;
    repaint();
}

void ParamSlotComponent::itemDragExit(const SourceDetails& /*details*/) {
    isDragOver_ = false;
    repaint();
}

void ParamSlotComponent::itemDropped(const SourceDetails& details) {
    isDragOver_ = false;

    // Parse the drag description: "mod_drag:trackId:topLevelDeviceId:modIndex"
    auto desc = details.description.toString();
    if (!desc.startsWith("mod_drag:")) {
        return;
    }

    auto parts = juce::StringArray::fromTokens(desc.substring(9), ":", "");
    if (parts.size() < 3) {
        return;
    }

    int modIndex = parts[2].getIntValue();

    // Create the link at 50% default amount
    magda::ModTarget target{deviceId_, paramIndex_};
    if (onModLinkedWithAmount) {
        onModLinkedWithAmount(modIndex, target, 0.5f);
    }

    repaint();
}

}  // namespace magda::daw::ui
