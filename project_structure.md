# 🏗️ MAGDA Project Structure

This document describes the product-based architecture of the MAGDA project.

## 🎯 Product-Based Organization

The project is organized by product domains rather than technical layers:

```
magda/
├── daw/                      # 🎵 DAW (Digital Audio Workstation) Domain
│   ├── command.hpp/.cpp      # Command pattern for DAW operations
│   ├── magda.hpp/.cpp       # DAW initialization and core functionality
│   ├── magda_daw_main.cpp   # Main application entry point with UI
│   ├── interfaces/           # DAW operation interfaces
│   │   ├── clip_interface.hpp
│   │   ├── mixer_interface.hpp
│   │   ├── prompt_interface.hpp
│   │   ├── track_interface.hpp
│   │   └── transport_interface.hpp
│   ├── engine/               # Audio engine (future aideas-core integration)
│   ├── components/           # UI components (future expansion)
│   └── CMakeLists.txt        # DAW build configuration
│
├── mcp/                      # 🤖 MCP (Model Context Protocol) Domain
│   ├── proto/                # Protocol buffer definitions
│   │   ├── magda_daw.proto  # DAW operations protocol
│   │   └── mcp_service.proto # MCP service definitions
│   ├── server/               # MCP server implementation
│   │   ├── grpc_mcp_server.hpp/.cpp
│   │   └── mcp_server_interface.hpp
│   ├── agents/               # Example AI agents
│   │   ├── orchestrator.py   # Python orchestrator with NLP
│   │   └── utility_agent.go  # Go utility agent for performance
│   └── CMakeLists.txt        # MCP library build configuration
│
├── tests/                    # 🧪 Testing
│   ├── test_command.cpp      # DAW command tests
│   ├── test_interfaces.cpp   # Interface tests
│   └── CMakeLists.txt        # Test configuration
│
├── adapters/                 # 🔌 External Integrations
│   └── juce/                 # JUCE/Tracktion adapter (future)
│
├── examples/                 # 📚 Usage Examples
│   └── CMakeLists.txt        # Example build configuration
│
├── CMakeLists.txt            # Main build configuration
├── Makefile                  # Convenience build wrapper
└── README.md                 # Project documentation
```

## 🎨 Domain Responsibilities

### 🎵 DAW Domain (`daw/`)
**The Digital Audio Workstation itself**

- **Purpose**: Complete DAW application including audio engine, UI, and core functionality
- **Components**:
  - Audio engine and DAW core logic
  - User interface and main application
  - DAW operation interfaces (tracks, clips, mixer, transport)
  - Command pattern for DAW operations
- **Libraries**: `magda_daw` (core library)
- **Executables**: `magda_daw_app` (main DAW application)
- **Future Integration**: This is where **aideas-core** will be integrated for advanced audio processing

### 🤖 MCP Domain (`mcp/`)
**The Multi-Agent Communication System**

- **Purpose**: Model Context Protocol server and agent ecosystem
- **Components**:
  - Protocol buffer definitions for DAW operations
  - gRPC MCP server implementation
  - Example AI agents (orchestrator, utility)
  - Agent communication and management
- **Libraries**: `magda_mcp` (MCP server library)
- **External Processes**: Agents run as separate processes connecting via gRPC

## 🔧 Build System & Dependencies

### Build Order
1. `daw/` → `magda_daw` library + `magda_daw_app` executable
2. `mcp/` → `magda_mcp` library (depends on `magda_daw`)
3. `tests/` → Test executables (depend on `magda_daw`)

### Key Dependencies
- **DAW Domain**:
  - nlohmann/json for configuration
  - Future: aideas-core for audio processing
- **MCP Domain**:
  - gRPC for agent communication
  - Protocol Buffers for message serialization
  - Depends on `magda_daw` for DAW operations

## 🚀 Benefits of Product-Based Organization

1. **Clear Product Boundaries**: Each directory represents a distinct product/system
2. **Independent Development**: DAW and MCP can be developed independently
3. **Modular Architecture**: Clean separation between the DAW and agent framework
4. **Easy Integration**: aideas-core integration is clearly scoped to the DAW domain
5. **Scalable**: Easy to add new domains (e.g., `plugins/`, `cloud/`, `mobile/`)

## 🔄 Key Architectural Decisions

### DAW ← MCP Relationship
- **MCP depends on DAW**: The MCP server needs to call DAW operations
- **DAW is independent**: The DAW can run without the MCP system
- **Agent Communication**: Agents connect to MCP server, which translates to DAW operations

### Integration Points
- **aideas-core**: Will be integrated into the `daw/` domain for audio processing
- **UI Framework**: JUCE/other UI frameworks live in the `daw/` domain
- **Plugin System**: Future plugin architecture would likely span both domains

## 🎯 Usage Patterns

### For DAW Development
Work primarily in `daw/` directory:
```bash
# Build just the DAW
cd daw && cmake -B build && make -C build

# Run the DAW application
./daw/build/magda_daw_app
```

### For Agent Development
Work primarily in `mcp/` directory:
```bash
# Run MCP server (separate from DAW)
./build/mcp_server

# Develop agents
cd mcp/agents
python orchestrator.py --daw localhost:50051
go run utility_agent.go --daw localhost:50051
```

This organization makes it crystal clear what belongs where and supports both independent development and integrated usage.
