#pragma once

/**
 * @file magica.hpp
 * @brief Main header for the Magica Multi-Agent DAW system
 *
 * Magica is a Multi-Agent Generative Interface for Creative Audio.
 * It enables multiple intelligent agents to collaboratively compose, arrange, and
 * manipulate music in real time through a unified API and server-based communication model.
 */

#include "command.hpp"
#include "interfaces/clip_interface.hpp"
#include "interfaces/mixer_interface.hpp"
#include "interfaces/prompt_interface.hpp"
#include "interfaces/track_interface.hpp"
#include "interfaces/transport_interface.hpp"

// Forward declaration
namespace magica {
class TracktionEngineWrapper;
}

/**
 * @brief Current version of Magica
 */
constexpr const char* MAGICA_VERSION = "0.1.0";

/**
 * @brief Initialize the Magica system
 * @return true if initialization was successful
 */
bool magica_initialize();

/**
 * @brief Shutdown the Magica system
 */
void magica_shutdown();

/**
 * @brief Get access to the global Tracktion Engine instance
 * @return Pointer to the engine wrapper, or nullptr if not initialized
 */
magica::TracktionEngineWrapper* magica_get_engine();