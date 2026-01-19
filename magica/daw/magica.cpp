#include "magica.hpp"

#include <iostream>
#include <memory>

#include "engine/TracktionEngineWrapper.hpp"

// Global engine instance
static std::unique_ptr<magica::TracktionEngineWrapper> g_engine;

bool magica_initialize() {
    std::cout << "Magica v" << MAGICA_VERSION
              << " - Multi-Agent Generative Interface for Creative Audio" << std::endl;
    std::cout << "Initializing system..." << std::endl;

    try {
        // Initialize Tracktion Engine
        g_engine = std::make_unique<magica::TracktionEngineWrapper>();
        if (!g_engine->initialize()) {
            std::cerr << "ERROR: Failed to initialize Tracktion Engine" << std::endl;
            return false;
        }

        // TODO: Initialize additional systems
        // - WebSocket server setup
        // - Interface registry
        // - Plugin discovery

        std::cout << "Magica initialized successfully!" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: Magica initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void magica_shutdown() {
    std::cout << "Shutting down Magica..." << std::endl;

    try {
        // Shutdown Tracktion Engine
        if (g_engine) {
            g_engine->shutdown();
            g_engine.reset();
        }

        // TODO: Cleanup additional systems
        // - Stop WebSocket server
        // - Cleanup resources
        // - Unload plugins

        std::cout << "Magica shutdown complete." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: Error during shutdown: " << e.what() << std::endl;
    }
}

// Access to the global engine instance for MCP server
magica::TracktionEngineWrapper* magica_get_engine() {
    return g_engine.get();
}
