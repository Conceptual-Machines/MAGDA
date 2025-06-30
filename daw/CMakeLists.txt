# DAW Domain - Digital Audio Workstation Core + UI
# This includes the audio engine, interfaces, UI, and main application

# DAW Core sources
set(DAW_CORE_SOURCES
    command.cpp
    magica.cpp
)

# DAW UI sources
set(DAW_UI_SOURCES
    magica_daw_main.cpp
)

# All DAW headers
set(DAW_HEADERS
    command.hpp
    magica.hpp
    interfaces/clip_interface.hpp
    interfaces/mixer_interface.hpp
    interfaces/prompt_interface.hpp
    interfaces/track_interface.hpp
    interfaces/transport_interface.hpp
)

# Create DAW library (core functionality)
add_library(magica_daw STATIC ${DAW_CORE_SOURCES})

# Create DAW executable (main application with UI)
add_executable(magica_daw_app ${DAW_UI_SOURCES})

# Link required libraries for DAW core
target_link_libraries(magica_daw
    PUBLIC
    nlohmann_json::nlohmann_json
)

# Link required libraries for DAW application
target_link_libraries(magica_daw_app
    PRIVATE
    magica_daw
    # NOTE: When aideas-core is integrated, add it here:
    # aideas_core::audio
    # aideas_core::midi  
    # aideas_core::music_theory
)

# Include directories for DAW library
target_include_directories(magica_daw
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
    PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Include directories for DAW application
target_include_directories(magica_daw_app
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Install targets
install(TARGETS magica_daw_app DESTINATION bin)
install(FILES ${DAW_HEADERS} DESTINATION include/magica/daw) 