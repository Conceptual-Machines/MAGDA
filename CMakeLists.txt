cmake_minimum_required(VERSION 3.24)

project(Magica VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Options
option(MAGICA_BUILD_TESTS "Build tests" ON)
option(MAGICA_BUILD_EXAMPLES "Build examples" ON)
option(MAGICA_BUILD_JUCE_ADAPTER "Build JUCE/Tracktion adapter" OFF)

# Find packages
find_package(Threads REQUIRED)

# Include FetchContent for dependency management
include(FetchContent)

# Set policy for FetchContent
if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# nlohmann/json for JSON handling
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# gRPC for server communication
FetchContent_Declare(
    gRPC
    GIT_REPOSITORY https://github.com/grpc/grpc
    GIT_TAG v1.60.0
)

# Catch2 for testing
if(MAGICA_BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
endif()

# Make dependencies available
FetchContent_MakeAvailable(nlohmann_json gRPC)

if(MAGICA_BUILD_TESTS)
    FetchContent_MakeAvailable(Catch2)
endif()

# Find required protobuf components
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Product-based subdirectories
add_subdirectory(daw)
add_subdirectory(mcp)

# Examples
if(MAGICA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(MAGICA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# JUCE adapter (optional)
if(MAGICA_BUILD_JUCE_ADAPTER)
    add_subdirectory(adapters/juce)
endif()

# Install rules
install(TARGETS magica_daw magica_mcp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
) 