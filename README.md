# 🔮 Magica — Multi-Agent Generative Interface for Creative Audio

**Magica** (**M**ulti-**A**gent **G**enerative **I**nterface for **C**reative **A**udio) is an experimental AI-driven Digital Audio Workstation built from the ground up for multi-agent collaboration. It combines a modern DAW engine with a comprehensive gRPC API, enabling intelligent agents to compose, arrange, and manipulate music in real-time.

## 🎵 What Makes Magica Special

- **AI-First Design**: Built specifically for AI agent collaboration
- **Universal gRPC API**: Agents can be written in any language (Python, Go, JavaScript, etc.)
- **Natural Language Interface**: Talk to your DAW in plain English
- **Real-time Collaboration**: Multiple agents working together simultaneously
- **Modern Architecture**: Clean separation between DAW engine, API, and UI

## 🏗️ Architecture

```
🤖 AI Agents (Python/Go/JS) 
       ↓ gRPC
🔮 Magica DAW (C++)
   ├── gRPC Server (Port 50051)
   ├── Tracktion Audio Engine  
   └── JUCE User Interface
```

## 🚀 Quick Start

### 1. Start Magica DAW
```bash
# Build and run the DAW
make build
./build/magica_daw

# Output:
# ✓ Audio engine initialized
# 🚀 Magica DAW gRPC server listening on 0.0.0.0:50051
# 🤖 Ready for AI agents to connect!
# 🔮 Magica DAW is ready!
```

### 2. Connect AI Agents

**Utility Agent (Go) - High-performance MIDI processing:**
```bash
go run agents/utility_agent.go --daw localhost:50051 --action listen

# Output:
# 🤖 Starting Utility Agent...
# ✓ Registered as agent: util_7f8a9b2c
# 🔗 Connected to Magica DAW at localhost:50051
# 👂 Listening for events... (Press Ctrl+C to exit)
```

**Orchestrator Agent (Python) - Natural language interface:**
```bash
python agents/orchestrator.py --daw localhost:50051

# Output:
# 🔗 Connecting to Magica DAW at localhost:50051
# ✅ Registered as orchestrator: orch_001
# 🎹 Interactive mode started. Type 'quit' to exit.
# 
# 🔮 Magica> 
```

### 3. Use Natural Language Commands

```bash
🔮 Magica> Clean up the piano recording

# 📊 Session context: 3 tracks, 120 BPM
# 🤖 Found 2 connected agents:
#    • UtilityAgent (utility): deduplicate_notes, remove_short_notes, quantize_notes, cleanup_recording
#    • OrchestratorAgent (orchestrator): natural_language_processing, agent_coordination
# 🎯 Intent: Clean up MIDI recording (complexity: simple)
# 🎯 Executing workflow with 1 steps
# 📋 Step 1: cleanup_recording via utility agent
# ✅ Step 1 sent to UtilityAgent

# Meanwhile, in Utility Agent terminal:
# 🧹 Deduplicating notes in clip: clip_piano_take_1
#    📝 Original notes: 1247
#    🗑️  Removed duplicates: 89
#    ✅ Deduplication complete: 1158 notes remaining
# ✂️  Removing notes shorter than 0.050 beats
#    🗑️  Removed short notes: 23
#    ✅ Short note removal complete: 1135 notes remaining
# 📐 Quantizing clip to 0.250 beat grid
#    ✅ Quantization complete
# 🎉 Recording cleanup complete for clip: clip_piano_take_1
```

## 🎛️ Complete gRPC API

Magica exposes a comprehensive API covering all DAW functionality:

### **Transport Control**
```python
# Play/Stop/Record
await client.Play(PlayRequest())
await client.Stop(StopRequest())
await client.SetTempo(SetTempoRequest(tempo=140.0))
```

### **Track Management**
```python
# Create tracks
track_resp = await client.CreateTrack(CreateTrackRequest(
    name="Piano",
    type=TrackType.MIDI
))

# Mute/Solo/Arm
await client.MuteTrack(MuteTrackRequest(
    track_id=track_resp.track_id,
    muted=True
))
```

### **MIDI Operations**
```python
# Create MIDI clip with notes
notes = [
    MidiNote(pitch=60, velocity=100, start_time=0.0, duration=0.5),
    MidiNote(pitch=64, velocity=100, start_time=0.5, duration=0.5),
    MidiNote(pitch=67, velocity=100, start_time=1.0, duration=0.5),
]

clip_resp = await client.CreateMidiClip(CreateMidiClipRequest(
    track_id=track_id,
    start_time=0.0,
    length=4.0,
    initial_notes=notes
))
```

### **Agent Management**
```python
# Register agent
register_resp = await client.RegisterAgent(RegisterAgentRequest(
    name="MyComposerAgent",
    type="composer",
    capabilities=["melody_generation", "chord_progressions"]
))

# Send messages between agents
await client.SendMessageToAgent(SendMessageRequest(
    target_agent_id="other_agent_id",
    message=json.dumps({"task": "harmonize", "clip_id": "clip_123"})
))
```

## 🤖 Agent Examples

### **Utility Agent (Go)**
High-performance MIDI processing for:
- Note deduplication
- Short note removal  
- Quantization
- Velocity normalization
- Recording cleanup

```bash
# Direct clip processing
go run agents/utility_agent.go --daw localhost:50051 --action cleanup --clip clip_123

# Event-driven processing
go run agents/utility_agent.go --daw localhost:50051 --action listen
```

### **Orchestrator Agent (Python)**
Natural language interface with LLM integration:
- Intent analysis
- Workflow coordination
- Agent routing
- Session management

```bash
# Interactive mode
python agents/orchestrator.py --daw localhost:50051 --openai-key sk-...

# Single command
python agents/orchestrator.py --daw localhost:50051 --request "Add a jazz melody"
```

### **Melody Agent (Python)**
AI composition capabilities:
- Melody generation
- Chord progressions
- Harmonization
- Style transfer

### **Percussion Agent (Python)**
Rhythm generation:
- Drum patterns
- Groove templates
- Fill generation
- Style adaptation

## 🎯 Real-World Usage Scenarios

### **Scenario 1: Post-Recording Cleanup**
```
User: "I just recorded this piano part but it has duplicates and short notes"

Workflow:
1. Orchestrator analyzes intent
2. Routes to Utility Agent
3. Utility Agent processes 1000+ notes in <20ms
4. Piano roll updates with clean MIDI
5. User can immediately continue editing
```

### **Scenario 2: Creative Composition**
```
User: "Add a walking bass line that follows the chord progression"

Workflow:
1. Orchestrator analyzes current session
2. Detects chord progression from existing tracks
3. Routes to Bass Agent with musical context
4. Bass Agent generates walking bass line
5. New MIDI track created with bass line
6. User can adjust and refine
```

### **Scenario 3: Full Arrangement**
```
User: "Turn this 4-bar loop into a full song with drums and strings"

Workflow:
1. Orchestrator creates complex workflow
2. Structure Agent analyzes loop and creates song form
3. Percussion Agent generates drum patterns for each section
4. String Agent creates orchestral arrangement
5. All parts coordinated and timed correctly
6. Full arrangement ready for mixing
```

## 🔧 Building from Source

### **Prerequisites**
- C++20 compiler (GCC 10+, Clang 12+)
- CMake 3.20+
- gRPC v1.60.0+
- JUCE framework
- Protocol Buffers

### **Build Steps**
```bash
# Clone repository
git clone https://github.com/yourusername/magica.git
cd magica

# Build with CMake
make build

# Run tests
make test

# Debug build
make debug
```

### **Dependencies**
```cmake
# Automatically fetched via CMake FetchContent:
# - gRPC v1.60.0
# - nlohmann/json v3.11.3
# - Catch2 v3.4.0 (testing)
# - Tracktion Engine (audio)
```

## 🎼 Why Magica?

**Traditional DAWs** were designed for human workflows. **Magica** is designed for **human-AI collaboration**:

- **Natural Language Interface**: Talk to your DAW like a collaborator
- **Real-time AI Assistance**: Agents continuously help with composition and arrangement
- **Unlimited Extensibility**: Create custom agents for any musical task
- **Language Agnostic**: Write agents in Python for AI, Go for performance, JavaScript for web integration
- **Modern Architecture**: Built with today's AI and cloud technologies

## 🤝 Contributing

Magica is an experimental project exploring the future of AI-driven music production. Contributions welcome!

### **Areas for Contribution**
- **New Agent Types**: Mastering, sound design, music theory analysis
- **UI Improvements**: Modern, AI-aware interface components
- **Performance Optimization**: Real-time audio processing enhancements
- **Language Bindings**: Client libraries for more languages
- **Documentation**: Tutorials, API reference, architectural guides

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

---

**Magica** - Where AI and music creation meet. 🎵🤖

---

## ✨ Vision

Magica serves as the middleware layer between a music production engine and intelligent agents (e.g., LLMs, MIDI generators, mixing assistants). The goal is to create a modular, programmable DAW architecture where agents can:
- Assist with music composition and arrangement
- Automate track editing and mixing tasks
- Control playback and transport
- Respond to user prompts or other agents

---

## 🧠 Key Components

### ✅ MCP Server (Multi-agent Control Protocol)
A WebSocket-based server that agents connect to. Handles:
- Registration and capability discovery
- Routing JSON-based commands
- Sending asynchronous DAW state events

### ✅ Generic API
An abstract control layer that defines what agents can do. Example modules:
- `TransportInterface`: play, stop, locate
- `TrackInterface`: add, mute, delete tracks
- `ClipInterface`: insert MIDI clips
- `MixerInterface`: volume, pan, FX routing
- `PromptInterface`: bridge to language models

### ✅ Host Integration (coming soon)
Adapters for real DAW backends (starting with Tracktion Engine) to bind the Magica API to actual audio and MIDI operations.

---

## 🧪 Status

Magica is in early research and prototyping. It is **not yet ready for production use**, but contributors and feedback are welcome as we design the core protocols and data model.

---

## 📦 Example Command

```json
{
  "command": "addMidiClip",
  "trackId": "track_1",
  "start": 4.0,
  "length": 2.0,
  "notes": [
    { "note": 60, "velocity": 100, "start": 0.0, "duration": 0.5 }
  ]
}
