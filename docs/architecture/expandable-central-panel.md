# Expandable Central Panel Architecture

## Overview

The central panel can display different content based on user action. By default it shows the Arrangement view (timeline with tracks/clips). When a user clicks an expand icon in a bottom panel editor (Piano Roll, Waveform Editor), that editor expands to fill the central panel.

## User Flow

```
1. User selects a MIDI clip
2. Piano Roll appears in bottom panel (current behavior)
3. User clicks expand icon [⤢] in Piano Roll
4. Central panel switches from Arrangement → Piano Roll (full)
5. User clicks collapse icon [⤡]
6. Central panel returns to Arrangement, Piano Roll stays in bottom panel
```

## Visual Layout

### Normal State
```
┌──────────────────────────────────────────────────────────┐
│  Track Headers  │  Arrangement View (timeline, clips)    │
│                 │                                        │
│  Track 1        │  [clip] [clip]                        │
│  Track 2        │       [clip]                          │
│  Track 3        │  [clip]     [clip]                    │
├─────────────────┴────────────────────────────────────────┤
│  Piano Roll                                    [⤢]      │
│  ┌─────────────────────────────────────────────────┐    │
│  │ C5 ░░░░█████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │    │
│  │ B4 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │    │
│  │ A4 ░░░░░░░░░░░░███░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │    │
│  └─────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────┘
```

### Expanded State
```
┌──────────────────────────────────────────────────────────┐
│  Piano Roll (editing: "MIDI 1" on Track 2)       [⤡]    │
│  ┌────────────────────────────────────────────────────┐  │
│  │ C5 ░░░░█████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │  │
│  │ B4 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │  │
│  │ A4 ░░░░░░░░░░░░███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │  │
│  │ G4 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │  │
│  │ F4 ░░░░░░░░░░░░░░░░░░░░████████░░░░░░░░░░░░░░░░░░ │  │
│  │ E4 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │  │
│  │ D4 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │  │
│  │ C4 ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │  │
│  └────────────────────────────────────────────────────┘  │
│  │ 1.1     │ 1.2     │ 1.3     │ 1.4     │ 2.1     │    │
└──────────────────────────────────────────────────────────┘
```

## Architecture Components

### 1. CentralPanelMode Enum

```cpp
// In a new file or existing state file
enum class CentralPanelMode {
    Arrangement,    // Default - timeline with tracks/clips
    PianoRoll,      // Expanded MIDI editor
    WaveformEditor, // Expanded audio editor
    // Future: Mixer, Automation, etc.
};
```

### 2. CentralPanelController

Manages what content is shown in the central panel.

```cpp
class CentralPanelController {
public:
    static CentralPanelController& getInstance();

    // Mode switching
    void setMode(CentralPanelMode mode);
    CentralPanelMode getMode() const;

    // Context for editors
    void setEditingClip(ClipId clipId);  // Which clip is being edited
    ClipId getEditingClip() const;

    // Expand/collapse convenience
    void expandPianoRoll(ClipId clipId);
    void expandWaveformEditor(ClipId clipId);
    void collapseToArrangement();

    // Listener pattern
    void addListener(CentralPanelListener* listener);
    void removeListener(CentralPanelListener* listener);

private:
    CentralPanelMode currentMode_ = CentralPanelMode::Arrangement;
    ClipId editingClipId_ = INVALID_CLIP_ID;
    std::vector<CentralPanelListener*> listeners_;
};

class CentralPanelListener {
public:
    virtual ~CentralPanelListener() = default;
    virtual void centralPanelModeChanged(CentralPanelMode mode, ClipId clipId) = 0;
};
```

### 3. MainView Changes

MainView becomes a CentralPanelListener and swaps content based on mode.

```cpp
class MainView : public juce::Component,
                 public CentralPanelListener {
public:
    // ...existing code...

    // CentralPanelListener
    void centralPanelModeChanged(CentralPanelMode mode, ClipId clipId) override {
        switch (mode) {
            case CentralPanelMode::Arrangement:
                showArrangementView();
                break;
            case CentralPanelMode::PianoRoll:
                showExpandedPianoRoll(clipId);
                break;
            case CentralPanelMode::WaveformEditor:
                showExpandedWaveformEditor(clipId);
                break;
        }
    }

private:
    // Expanded editor components (created on demand)
    std::unique_ptr<ExpandedPianoRoll> expandedPianoRoll_;
    std::unique_ptr<ExpandedWaveformEditor> expandedWaveformEditor_;

    void showArrangementView();
    void showExpandedPianoRoll(ClipId clipId);
    void showExpandedWaveformEditor(ClipId clipId);
};
```

### 4. PanelContent Expand Button

Add expand capability to bottom panel editors.

```cpp
class PianoRollContent : public PanelContent {
public:
    // ...existing code...

private:
    std::unique_ptr<juce::DrawableButton> expandButton_;

    void setupExpandButton() {
        expandButton_ = std::make_unique<juce::DrawableButton>("Expand",
            juce::DrawableButton::ImageFitted);
        // Load expand icon SVG
        expandButton_->onClick = [this]() {
            if (editingClipId_ != INVALID_CLIP_ID) {
                CentralPanelController::getInstance()
                    .expandPianoRoll(editingClipId_);
            }
        };
        addAndMakeVisible(expandButton_.get());
    }
};
```

### 5. Expanded Editor Components

Full-size versions of the editors optimized for the central panel.

```cpp
// Could be the same component with a "mode" flag, or separate components
class ExpandedPianoRoll : public juce::Component {
public:
    void setClip(ClipId clipId);

private:
    ClipId clipId_;
    std::unique_ptr<juce::DrawableButton> collapseButton_;

    // Full piano roll UI: keyboard, note grid, velocity lane, etc.
    std::unique_ptr<PianoKeyboard> keyboard_;
    std::unique_ptr<NoteGrid> noteGrid_;
    std::unique_ptr<VelocityLane> velocityLane_;

    void setupCollapseButton() {
        collapseButton_->onClick = []() {
            CentralPanelController::getInstance().collapseToArrangement();
        };
    }
};
```

## State Synchronization

### Bottom Panel ↔ Expanded View

When expanded, the bottom panel should:
- Either hide (clean UI)
- Or stay visible but indicate "editing in full view"
- Changes in either view update the same clip data

```cpp
// Both views edit the same ClipId through ClipManager
// ClipManager notifications keep them in sync
void ExpandedPianoRoll::noteAdded(/* ... */) {
    ClipManager::getInstance().addMidiNote(clipId_, note);
    // ClipManagerListener notifications update both views
}
```

### Keyboard Shortcuts

```cpp
// In MainView or MainWindow key handler
case juce::KeyPress::escapeKey:
    if (CentralPanelController::getInstance().getMode() != CentralPanelMode::Arrangement) {
        CentralPanelController::getInstance().collapseToArrangement();
        return true;
    }
    break;
```

## File Structure

```
magda/daw/
├── core/
│   └── CentralPanelController.hpp/cpp  // NEW: Mode management
├── ui/
│   ├── views/
│   │   └── MainView.hpp/cpp            // MODIFY: Add mode switching
│   ├── components/
│   │   └── editors/                    // NEW: Expanded editors
│   │       ├── ExpandedPianoRoll.hpp/cpp
│   │       └── ExpandedWaveformEditor.hpp/cpp
│   └── panels/
│       └── content/
│           ├── PianoRollContent.hpp/cpp    // MODIFY: Add expand button
│           └── WaveformEditorContent.hpp/cpp // MODIFY: Add expand button
```

## Implementation Order

1. **Create CentralPanelController** - Mode state management
2. **Update MainView** - Listen for mode changes, show/hide arrangement
3. **Implement Piano Roll properly** - Full MIDI editing capability
4. **Create ExpandedPianoRoll** - Full-size version for central panel
5. **Add expand button to PianoRollContent** - Triggers expansion
6. **Repeat for WaveformEditor** - Same pattern

## Future Considerations

- **Multiple clips**: Could allow editing multiple clips in expanded view
- **Split view**: Show arrangement + editor side by side
- **Detachable windows**: Pop out editors to separate windows
- **Automation lanes**: Expanded view for automation editing
- **Mixer in central**: Full mixer view as central panel option
