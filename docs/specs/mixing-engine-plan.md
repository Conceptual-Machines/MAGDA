# Mixing Engine Implementation Plan

## Requirements
1. All channels are mixed into master channel
2. It is possible to send audio from one track to another (aux sends)
3. It is possible to route audio to monitoring output (future - out of scope)
4. Integrated with UI elements: M/S in all views

---

## Current Status

### Working
- Volume/Pan controls (TrackInfo → TrackManager → AudioBridge → Tracktion VolumeAndPanPlugin)
- Mute/Solo mapped to Tracktion Engine
- Master channel strip with volume/pan
- Track metering via LevelMeterPlugin
- Basic audio output routing (track → master or hardware device)

### Partially Working
- Master metering (just fixed: uses LevelMeterPlugin now)
- Track output routing to master (just added: explicit `setOutputToDefaultDevice()`)

### Not Implemented
- Aux Sends (send from one track to another)
- M/S audit across all views

---

## Phase 1: Validate Core Signal Flow (Immediate)

### Task 1.1: Verify All Tracks Route to Master
- [ ] Test that track audio actually reaches master output
- [ ] Verify metering shows signal on both track and master
- [ ] Confirm mute/solo affects audio correctly

### Task 1.2: Fix Any Remaining Routing Issues
- [ ] Ensure new tracks route to master by default (done: added to createAudioTrack)
- [ ] Ensure master output is properly configured

---

## Phase 2: Aux Sends Implementation

### Task 2.1: Data Model (TrackInfo.hpp)
```cpp
struct SendInfo {
    int id;                  // Unique send ID
    TrackId targetTrackId;   // Destination track (or -1 for new bus)
    float level;             // Send level (0.0 - 1.0)
    bool preFader;           // Pre-fader (true) or post-fader (false)
    bool enabled;            // Mute this send
};

// Add to TrackInfo:
std::vector<SendInfo> sends;
```

### Task 2.2: TrackManager API
```cpp
// New methods:
SendId addSend(TrackId sourceTrack, TrackId destTrack, float level = 1.0f, bool preFader = false);
void removeSend(TrackId sourceTrack, SendId sendId);
void setSendLevel(TrackId sourceTrack, SendId sendId, float level);
void setSendPreFader(TrackId sourceTrack, SendId sendId, bool preFader);
void setSendEnabled(TrackId sourceTrack, SendId sendId, bool enabled);
const std::vector<SendInfo>& getSends(TrackId trackId) const;
```

### Task 2.3: AudioBridge Implementation
```cpp
// Map sends to Tracktion Engine
// Option A: Use te::AuxSendPlugin + te::AuxReturnPlugin
// Option B: Use te::RackType for custom routing
// Option C: Use direct plugin-to-plugin routing

void createSend(TrackId source, TrackId dest, const SendInfo& info);
void updateSend(TrackId source, SendId sendId, const SendInfo& info);
void removeSend(TrackId source, SendId sendId);
```

### Task 2.4: Tracktion Engine Research
- [ ] Investigate te::AuxSendPlugin / te::AuxReturnPlugin
- [ ] Investigate te::RackType for routing
- [ ] Determine best approach for internal track-to-track sends

### Task 2.5: UI - InspectorContent Sends Section
- [ ] Replace "No sends" placeholder with actual send list
- [ ] Add "+" button to create new send
- [ ] For each send: dropdown (dest track), level slider, pre/post toggle, remove button

### Task 2.6: UI - MixerView Sends
- [ ] Add small send section below fader (expandable)
- [ ] Show send levels as mini-faders or knobs
- [ ] Click to expand/edit sends

---

## Phase 3: M/S Audit Across All Views

### Task 3.1: Audit All Views for M/S
- [ ] ArrangementView track headers - check/add M/S buttons
- [ ] SessionView track display - check/add M/S buttons
- [ ] Piano Roll View - check if track selection needs M/S

### Task 3.2: Ensure M/S State Sync
- [ ] All views listen to TrackManager for mute/solo changes
- [ ] All views update visual state correctly
- [ ] Test: changing M/S in MixerView reflects in Inspector and vice versa

---

## Implementation Priority

1. **Immediate**: Test current build - verify track→master routing works
2. **Phase 2**: Aux Sends (biggest missing feature)
3. **Phase 3**: M/S audit (mostly done, just verification)

---

## Technical Notes

### Tracktion Engine Send Options

**Option A: AuxSendPlugin + AuxReturnPlugin**
- Built-in Tracktion plugins for send/return routing
- AuxSendPlugin on source track → AuxReturnPlugin on destination
- May require bus track concept

**Option B: RackType**
- Custom routing matrix
- More flexible but more complex
- Good for complex routing scenarios

**Option C: Direct Signal Splitting**
- Use LevelMeterPlugin or custom plugin to tap signal
- Route tapped signal to another track's input
- More manual but full control

### Recommended Approach
Start with Option A (AuxSendPlugin) as it's the standard DAW pattern. If limitations appear, fall back to Option B.

---

## Files to Modify

### Phase 2 (Sends)
- `magda/daw/core/TrackInfo.hpp` - Add SendInfo struct
- `magda/daw/core/TrackManager.hpp/cpp` - Add send management API
- `magda/daw/audio/AudioBridge.hpp/cpp` - Implement send routing
- `magda/daw/ui/panels/content/InspectorContent.hpp/cpp` - Sends UI
- `magda/daw/ui/views/MixerView.hpp/cpp` - Mini-sends display

### Phase 3 (M/S Audit)
- `magda/daw/ui/views/ArrangementView.hpp/cpp` - Verify M/S
- `magda/daw/ui/views/SessionView.hpp/cpp` - Verify M/S
