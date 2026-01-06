# Check that we're building for x64
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Thermite currently only supports x64 architecture")
endif()

# Check that we're using MSVC as our compiler
if (NOT MSVC)
    message(FATAL_ERROR "Thermite currently only supports building with MSVC.")
endif()

# Collect source files
file(GLOB_RECURSE THERMITE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

# Add the editor as static library
add_library(thermite-editor STATIC ${THERMITE_SOURCES})
# Link editor to engine
target_link_libraries(thermite-editor PRIVATE thermite-engine)

# Add compile definitons
target_compile_definitions(thermite-engine PRIVATE NOMINMAX) # Makes windows.h not define min and max

# Add include paths
target_include_directories(thermite-editor
    PUBLIC ${CMAKE_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add project compile definitions
target_compile_definitions(thermite-editor PRIVATE "IMGUI_DEFINE_MATH_OPERATORS")

# Add the precompiled header
target_precompile_headers(thermite-editor PRIVATE "shared/pch.hpp")

# Set warning level
target_compile_options(thermite-editor PRIVATE /W4 /permissive-)
