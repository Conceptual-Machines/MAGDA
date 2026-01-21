#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

#include "core/RackInfo.hpp"
#include "core/SelectionManager.hpp"
#include "core/TrackManager.hpp"
#include "ui/components/common/SvgButton.hpp"
#include "ui/components/common/TextSlider.hpp"

namespace magda::daw::ui {

class RackComponent;

/**
 * @brief A single chain row within a rack - simple strip layout
 *
 * Layout: [Name] [Gain] [Pan] [M] [S] [On] [X]
 *
 * Clicking the row will open a chain panel on the right side showing devices.
 * Implements SelectionManagerListener for centralized exclusive selection.
 */
class ChainRowComponent : public juce::Component, public magda::SelectionManagerListener {
  public:
    ChainRowComponent(RackComponent& owner, magda::TrackId trackId, magda::RackId rackId,
                      const magda::ChainInfo& chain);
    ~ChainRowComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    int getPreferredHeight() const;
    magda::ChainId getChainId() const {
        return chainId_;
    }
    magda::TrackId getTrackId() const {
        return trackId_;
    }
    magda::RackId getRackId() const {
        return rackId_;
    }

    void updateFromChain(const magda::ChainInfo& chain);

    void setSelected(bool selected);
    bool isSelected() const {
        return selected_;
    }

    // SelectionManagerListener
    void selectionTypeChanged(magda::SelectionType newType) override;
    void chainNodeSelectionChanged(const magda::ChainNodePath& path) override;

    // Callback when chain row is clicked (legacy - for RackComponent to show chain panel)
    std::function<void(ChainRowComponent&)> onSelected;
    // Callbacks for mod/macro panel toggles
    std::function<void(bool)> onModToggled;
    std::function<void(bool)> onMacroToggled;

    // Get toggle states
    bool isModActive() const;
    bool isMacroActive() const;

  private:
    void onMuteClicked();
    void onSoloClicked();
    void onBypassClicked();
    void onDeleteClicked();

    RackComponent& owner_;
    magda::TrackId trackId_;
    magda::RackId rackId_;
    magda::ChainId chainId_;
    bool selected_ = false;
    magda::ChainNodePath nodePath_;  // For centralized selection

    // Single row controls: Name | Gain | Pan | MOD | MACRO | M | S | On | X
    juce::Label nameLabel_;
    TextSlider gainSlider_;                          // Will be configured in constructor
    TextSlider panSlider_;                           // Will be configured in constructor
    std::unique_ptr<magda::SvgButton> modButton_;    // Modulators toggle
    std::unique_ptr<magda::SvgButton> macroButton_;  // Macros toggle
    juce::TextButton muteButton_;
    juce::TextButton soloButton_;
    std::unique_ptr<magda::SvgButton> onButton_;  // Bypass/enable toggle (power icon)
    juce::TextButton deleteButton_;               // Delete chain

    static constexpr int ROW_HEIGHT = 22;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainRowComponent)
};

}  // namespace magda::daw::ui
