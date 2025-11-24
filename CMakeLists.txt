cmake_minimum_required(VERSION 3.25)
project(thermite LANGUAGES CXX)

# Use C++20 as language standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define THERMITE_*_BUILD macro defaults 
option(THERMITE_EDITOR_BUILD "Build editor version" ON)
option(THERMITE_DEBUG_BUILD "Thermite in debug build" ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compilation settings
option(WARNINGS_AS_ERRORS "Enable warnings as errors" OFF)
option(THERMITE_EDITOR_BUILD "Build editor configuration" OFF)
option(THERMITE_DEBUG_BUILD "Build debug-style config (extra asserts/logging)" OFF)

# Engine sub-directory
add_subdirectory("engine")

# Editor and Debug defines
if (THERMITE_EDITOR_BUILD)
    target_compile_definitions(thermite-engine PUBLIC THERMITE_EDITOR=1)
endif()
if (THERMITE_DEBUG_BUILD)
    target_compile_definitions(thermite-engine PUBLIC THERMITE_DEBUG=1)
endif()

# Find and add projects
include(scripts/cmake/code_modules.cmake) 
find_and_add_targets()

# Add externals
add_subdirectory("extern")
