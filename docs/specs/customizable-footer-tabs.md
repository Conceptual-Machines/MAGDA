# Customizable Footer Tabs Specification

## Overview
Extend the FooterBar to support user-customizable tabs alongside the fixed view mode buttons. This allows utility content (AI Chat, Script Console, Signal Flow, MIDI Editor) to be accessed as full-width bottom panel views.

## Current State
- FooterBar displays 4 fixed view mode buttons: Live, Arrange, Mix, Master
- Bottom panel uses TabbedPanel but is collapsed by default
- No way for users to add persistent utility tabs to the footer

## Proposed Design

### Footer Layout
```
┌─────────────────────────────────────────────────────────────────┐
│  [Live] [Arrange] [Mix] [Master]  │  [+] [AI Chat] [Script]     │
│  └── Fixed View Modes ──────────┘    └── User Tabs (removable) ─┘│
└─────────────────────────────────────────────────────────────────┘
```

### Components

#### 1. FooterBar Extensions
- Add `userTabs_` array alongside `modeButtons_`
- Add separator between fixed modes and user tabs
- Add "+" button to open content picker
- Support right-click on user tabs for remove option

#### 2. FooterState (new)
```cpp
struct FooterState {
    std::vector<PanelContentType> userTabs;  // User-added tabs
    int activeUserTab = -1;  // -1 = none active (view mode active)
};
```

#### 3. FooterController (new, or extend ViewModeController)
- Manages footer state
- Events: AddUserTab, RemoveUserTab, SetActiveUserTab
- Persists user tab configuration

#### 4. Bottom Content Area
- When a user tab is active, show its content in the bottom panel area
- When a view mode is active, bottom panel shows view-specific content (or collapses)
- Smooth transition between states

### Interaction Model

1. **Click view mode** → Deactivate any user tab, switch view mode
2. **Click user tab** → Activate user tab, show content in bottom area
3. **Click "+"** → Show popup with available content types to add
4. **Right-click user tab** → Context menu with "Remove" option
5. **Drag user tabs** → Reorder (future enhancement)

### Content Types Available for Footer
- AIChatConsole
- ScriptingConsole
- SignalFlowEditor (future)
- MidiEditor (future)
- AudioEditor (future)

### State Persistence
- Save user tabs to config file
- Restore on app launch
- Default: empty (no user tabs)

### Visual Design
- User tabs use same SvgButton style as view modes
- Separator: subtle vertical line or spacing
- "+" button: smaller, muted color until hover
- Active user tab: same highlight as active view mode

### Implementation Phases

**Phase 1: Footer State Management**
- FooterState struct
- FooterController (or extend ViewModeController)
- Events and listener interface

**Phase 2: FooterBar UI Updates**
- Add user tabs section
- Add separator
- Add "+" button
- Wire up to controller

**Phase 3: Bottom Panel Integration**
- Connect footer user tab activation to bottom panel content
- Handle transitions between view modes and user tabs

**Phase 4: Persistence**
- Save/load user tab configuration
- Add to preferences if needed

## Relationship to Side Panels

| Feature | Side Panels | Footer Tabs |
|---------|-------------|-------------|
| Location | Left/Right sidebars | Bottom area |
| Tab bar | PanelTabBar (bottom of panel) | FooterBar (app footer) |
| Content size | Narrow, vertical | Full width, horizontal |
| Max tabs | 4 per panel | TBD (4-6 recommended) |
| Controller | PanelController | FooterController |

Users choose based on content needs:
- Narrow content (browsers, inspector) → Side panels
- Wide content (chat, editors, signal flow) → Footer tabs
