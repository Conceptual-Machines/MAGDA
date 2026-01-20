# MAGDA

**MAGDA** is an experimental AI-native Digital Audio Workstation. Built from the ground up for human-AI collaboration in music production.

## Status

Early research and prototyping. Not yet ready for production use.

## Building

### Prerequisites

- C++20 compiler (GCC 10+, Clang 12+, or Xcode)
- CMake 3.20+

### Quick Start

```bash
# Clone with submodules
git clone --recursive https://github.com/Conceptual-Machines/MAGDA.git
cd magda

# Setup and build
make setup
make debug

# Run
make run
```

### Make Targets

```bash
make setup      # Initialize submodules and dependencies
make debug      # Debug build
make release    # Release build
make test       # Run tests
make clean      # Clean build artifacts
make format     # Format code
make quality    # Run all quality checks
```

## Architecture

```
magda/
├── daw/        # DAW application (C++/JUCE)
│   ├── core/       # Track, clip, selection management
│   ├── engine/     # Tracktion Engine wrapper
│   ├── ui/         # User interface components
│   └── interfaces/ # Abstract interfaces
├── agents/     # Agent system (C++)
└── tests/      # Test suite
```

## Dependencies

- [Tracktion Engine](https://github.com/Tracktion/tracktion_engine) - Audio engine
- [JUCE](https://juce.com/) - GUI framework
- [Catch2](https://github.com/catchorg/Catch2) - Testing (fetched via CMake)
- [nlohmann/json](https://github.com/nlohmann/json) - JSON library (fetched via CMake)

## License

GPL v3 - see [LICENSE](LICENSE) for details.
