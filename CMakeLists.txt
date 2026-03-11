cmake_minimum_required(VERSION 3.25)
project(thermite LANGUAGES CXX)

# Use C++20 as language standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check that we're building for x64
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Thermite currently only supports x64 architecture")
endif()

# Check that we're using MSVC as our compiler
if (NOT MSVC)
    message(FATAL_ERROR "Thermite currently only supports building with MSVC.")
endif()

# Enable bigobj flag so we can compile big object files
# Enable C++ exception handling (required for EnTT consistency across all targets)
add_compile_options( /bigobj /EHsc )

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build version completed by CI
set(BUILD_VERSION "" CACHE STRING "Version suffix for executable")

# Compilation settings
option(WARNINGS_AS_ERRORS "Enable warnings as errors" OFF)
option(THERMITE_EDITOR_BUILD "Build editor configuration" OFF)
option(THERMITE_DEVELOPER_BUILD "Build for developer use on P4" OFF)
option(THERMITE_DEBUG_BUILD "Build debug-style config (extra asserts/logging)" OFF)
option(THERMITE_BUILD_TESTS "Build unit tests" OFF)
option(THERMITE_CI "Treat the build as CI" OFF)
# Engine sub-directory
add_subdirectory("engine")

# Editor sub-directory
if (THERMITE_EDITOR_BUILD)
    add_subdirectory("editor")
endif()

# Add externals
add_subdirectory("extern")

# Editor and Debug defines
if (THERMITE_EDITOR_BUILD)
    target_compile_definitions(thermite-engine PUBLIC THERMITE_EDITOR=1)
    target_compile_definitions(thermite-editor PUBLIC THERMITE_EDITOR=1)
endif()
if (THERMITE_DEBUG_BUILD)
    target_compile_definitions(thermite-engine PUBLIC THERMITE_DEBUG=1)
    if (THERMITE_EDITOR_BUILD)
        target_compile_definitions(thermite-editor PUBLIC THERMITE_DEBUG=1)
    endif()
endif()

# Find and add projects
include(scripts/cmake/code_modules.cmake) 
find_and_add_targets()

# Add unit tests
if(THERMITE_BUILD_TESTS)
    enable_testing()
    add_subdirectory("tests")
endif()
