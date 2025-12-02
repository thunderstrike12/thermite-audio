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

# Add the precompiled header
target_precompile_headers(thermite-engine PRIVATE "shared/pch.hpp")

# Set warning level
target_compile_options(thermite-engine PRIVATE /W4 /permissive-)
