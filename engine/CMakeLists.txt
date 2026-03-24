# Collect source files
file(GLOB_RECURSE THERMITE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

# Add the engine as static library
add_library(thermite-engine STATIC ${THERMITE_SOURCES})

# Supress external library warnings
# SPDLOG: "warning C4459: declaration of 'formattable' hides global declaration"
# uuid_v4: "warning C4081: expected 'newline'; found ';'" (malformed pragma warning in header)
target_compile_options(thermite-engine PRIVATE /wd4459 /wd4081)

# Add include paths
target_include_directories(thermite-engine 
    PUBLIC ${CMAKE_SOURCE_DIR}  
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add compile definitons
target_compile_definitions(thermite-engine PRIVATE NOMINMAX) # Makes windows.h not define min and max
target_compile_definitions(thermite-engine PRIVATE THERMITE_ENGINE) # Define that we're building the engine
target_compile_definitions(thermite-engine PRIVATE UFBX_REAL_IS_FLOAT) # Use 32 bit floats when parsing fbx.

# Compile Shaders
include(../scripts/cmake/shader_compilation.cmake)
compile_shaders()

# Add the precompiled header
target_precompile_headers(thermite-engine PRIVATE "shared/pch.hpp")

# Set warning level (W4 = level 4, WX = warnings as errors)
target_compile_options(thermite-engine PRIVATE /W4 /permissive- /WX)
