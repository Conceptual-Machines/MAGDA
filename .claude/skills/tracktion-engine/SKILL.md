---
name: tracktion-engine
description: Work with Tracktion Engine audio DAW framework. Use when creating/manipulating Edits, Tracks, Clips, Plugins, or Transport control. Covers the core model, playback, and plugin hosting patterns.
---

# Tracktion Engine

Tracktion Engine is the audio engine powering MAGDA. This skill covers core patterns for working with the API.

## Core Object Hierarchy

```
Engine (singleton, app lifecycle)
└── Edit (project/session)
    ├── TrackList
    │   ├── AudioTrack (contains clips + plugins)
    │   ├── FolderTrack (groups other tracks)
    │   ├── MasterTrack
    │   └── TempoTrack, ChordTrack, MarkerTrack...
    ├── TransportControl (playback/recording)
    ├── PluginList (master plugins)
    └── UndoManager
```

## Quick Reference

### Creating an Edit

```cpp
#include <tracktion_engine/tracktion_engine.h>
namespace te = tracktion;

// Initialize engine (once, at app startup)
te::Engine engine("MAGDA");

// Create a new edit
auto edit = te::Edit::createSingleTrackEdit(engine);

// Or with options
te::Edit::Options options;
options.editState = valueTree;  // Load from saved state
options.engine = &engine;
auto edit = te::Edit::createEdit(options);
```

### Working with Tracks

```cpp
// Get all tracks
for (auto track : edit->getTrackList()) {
    DBG(track->getName());
}

// Add a new audio track
auto track = edit->insertNewAudioTrack(
    te::TrackInsertPoint::getEndOfTracks(*edit),
    nullptr,  // SelectionManager (optional)
    true      // addDefaultPlugins (volume, pan, level meter)
);

// Find tracks by type
for (auto audioTrack : te::getAudioTracks(*edit)) {
    // audioTrack is AudioTrack*
}

// Track properties
track->setName("Lead Vocal");
track->setMuted(true);
track->setSolo(true);
```

### Working with Clips

```cpp
// Insert MIDI clip
auto midiClip = track->insertMIDIClip(
    te::TimeRange { 0_tp, 8_tp },  // start, end (TimePosition)
    nullptr  // SelectionManager
);

// Insert audio clip
auto audioClip = track->insertWaveClip(
    "My Audio",
    juce::File("/path/to/audio.wav"),
    te::ClipPosition { { 0_tp, 8_tp }, {} },
    te::DeleteExistingClips::no
);

// Iterate clips on a track
for (auto clip : track->getClips()) {
    auto pos = clip->getPosition();
    DBG("Clip: " + clip->getName() + " at " + juce::String(pos.getStart().inSeconds()));
}

// Clip manipulation
clip->setPosition(te::ClipPosition { { 2_tp, 10_tp }, {} });
clip->setName("Verse 1");
clip->setMuted(true);
```

### MIDI Editing

```cpp
if (auto midiClip = dynamic_cast<te::MidiClip*>(clip)) {
    auto& sequence = midiClip->getSequence();

    // Add notes
    sequence.addNote(
        60,     // pitch (middle C)
        0.0,    // start beat
        1.0,    // length in beats
        100,    // velocity
        0,      // colour index
        nullptr // undo manager
    );

    // Iterate notes
    for (auto note : sequence.getNotes()) {
        DBG("Note: " + juce::String(note->getNoteNumber()));
    }
}
```

### Transport Control

```cpp
auto& transport = edit->getTransport();

// Playback
transport.play(false);
transport.stop(false, false);
transport.playFromStart(false);

// Position
transport.setPosition(te::TimePosition::fromSeconds(10.0));
auto pos = transport.getPosition();

// Recording
transport.record(false);
transport.stopRecording(false);

// Loop
transport.setLoopRange(te::TimeRange { 0_tp, 16_tp });

// Check state
if (transport.isPlaying()) { /* ... */ }
if (transport.isRecording()) { /* ... */ }
```

### Plugin Management

```cpp
// Get track's plugin list
auto& plugins = track->getPlugins();

// Built-in plugins
if (auto vol = track->getVolumePlugin()) {
    vol->setVolumeDb(-6.0f);
    vol->setPan(-0.5f);  // -1 to 1
}

// Find plugins by type
auto eqs = plugins.getPluginsOfType<te::EqualiserPlugin>();

// Iterate all plugins
for (auto plugin : plugins) {
    if (plugin->isEnabled()) {
        DBG("Plugin: " + plugin->getName());
    }
}

// Plugin parameters
for (auto param : plugin->getAutomatableParameters()) {
    DBG("Param: " + param->getParameterName());
}
```

## Important Patterns

### RAII Helpers

```cpp
// Prevent playback graph rebuilds during batch edits
{
    te::TransportControl::ReallocationInhibitor inhibitor(transport);
    // ... make many changes ...
}  // Graph rebuilds once here

// Inhibit undo grouping
{
    te::Edit::UndoTransactionInhibitor inhibitor(*edit);
    // ... operations won't be grouped ...
}

// Resume playback after scope
{
    te::TransportControl::ScopedPlaybackRestarter restarter(transport);
    transport.stop(false, false);
    // ... do work ...
}  // Playback resumes here
```

### Time Types

```cpp
// TimePosition - absolute time
te::TimePosition pos = te::TimePosition::fromSeconds(5.0);
te::TimePosition pos2 = 5_tp;  // User-defined literal

// TimeDuration - relative duration
te::TimeDuration dur = te::TimeDuration::fromSeconds(2.0);

// BeatPosition / BeatDuration - musical time
te::BeatPosition beat = te::BeatPosition::fromBeats(4.0);

// TimeRange - start + end
te::TimeRange range { startPos, endPos };
```

### Edit State & Undo

```cpp
// Undo/Redo
edit->undo();
edit->redo();

// Check for changes
if (edit->hasChangedSinceSaved()) {
    // Prompt to save
}

// Mark as saved
edit->resetChangedStatus();

// Flush state to ValueTree (for saving)
edit->flushState();
juce::ValueTree state = edit->state;
```

### Listeners

```cpp
// Transport listener
struct MyListener : te::TransportControl::Listener {
    void playbackContextChanged() override { /* ... */ }
    void recordingStarted(te::SyncPoint, std::optional<te::TimeRange>) override { /* ... */ }
    void recordingFinished(te::InputDeviceInstance&, te::EditItemID, te::Clip::Array) override { /* ... */ }
};

transport.addListener(&myListener);
```

## Common Pitfalls

1. **Always initialize Engine first** - Before creating any Edit
2. **Don't block audio thread** - Use AsyncUpdater for UI updates from audio callbacks
3. **Use inhibitors for batch operations** - Prevents expensive graph rebuilds
4. **Check for nullptr** - `getVolumePlugin()` etc. can return nullptr
5. **TimePosition vs BeatPosition** - They're different types, convert explicitly

## File Locations

Key headers in `third_party/tracktion_engine/modules/tracktion_engine/`:
- `model/edit/tracktion_Edit.h` - Edit class
- `model/tracks/tracktion_Track.h` - Track base class
- `model/clips/tracktion_Clip.h` - Clip base class
- `playback/tracktion_TransportControl.h` - Transport
- `plugins/tracktion_Plugin.h` - Plugin base class
