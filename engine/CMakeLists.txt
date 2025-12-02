# Check that we're building for x64
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Thermite currently only supports x64 architecture")
endif()

# Check that we're using MSVC as our compiler
if (NOT MSVC)
    message(FATAL_ERROR "Thermite currently only supports building with MSVC.")
endif()

# Collect source files
file(GLOB_RECURSE THERMITE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

# Add the engine as static library
add_library(thermite-engine STATIC ${THERMITE_SOURCES})

# Add include paths
target_include_directories(thermite-engine 
    PUBLIC ${CMAKE_SOURCE_DIR}  
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
# Python interpreter
find_package(Python3 REQUIRED)

# Where your shaders live
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/engine/assets/shaders)

# Where compiled SPIR-V should go
set(SHADER_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/engine/assets/shaders)

# Script path
set(SHADER_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/compile_shaders.py)

add_custom_target(shaders ALL
    DEPENDS ${SHADER_SCRIPT}
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Compiling Slang shaders..."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${Python3_EXECUTABLE} ${SHADER_SCRIPT}
            ${SHADER_SOURCE_DIR}
            ${SHADER_OUTPUT_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command(
    TARGET thermite-engine PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} ${SHADER_SCRIPT}
            ${SHADER_SOURCE_DIR}
            ${SHADER_OUTPUT_DIR}
    COMMENT "Compiling Slang shaders..."
)

add_dependencies(thermite-engine shaders)

# Add the precompiled header
target_precompile_headers(thermite-engine PRIVATE "shared/pch.hpp")

# Set warning level
target_compile_options(thermite-engine PRIVATE /W4 /permissive-)
