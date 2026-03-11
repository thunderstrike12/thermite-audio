# Collect source files
file(GLOB_RECURSE THERMITE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

# Add the editor as static library
add_library(thermite-editor STATIC ${THERMITE_SOURCES})
# Link editor to engine
target_link_libraries(thermite-editor PRIVATE thermite-engine)

# Supress external library warnings
# SPDLOG: "warning C4459: declaration of 'formattable' hides global declaration"
target_compile_options(thermite-editor PRIVATE /wd4459)

# Add compile definitons
target_compile_definitions(thermite-engine PRIVATE NOMINMAX) # Makes windows.h not define min and max

# Add include paths
target_include_directories(thermite-editor
    PUBLIC ${CMAKE_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add project compile definitions
target_compile_definitions(thermite-editor PRIVATE "IMGUI_DEFINE_MATH_OPERATORS" NOMINMAX)

# Add the precompiled header
target_precompile_headers(thermite-editor PRIVATE "shared/pch.hpp")

# Set warning level (W4 = level 4, WX = warnings as errors)
target_compile_options(thermite-editor PRIVATE /W4 /permissive- /WX)

# Disable warnings for external imgui-node-editor sources
set_source_files_properties(
    "${CMAKE_SOURCE_DIR}/extern/imgui-node-editor/imgui_node_editor.cpp"
    "${CMAKE_SOURCE_DIR}/extern/imgui-node-editor/imgui_node_editor_api.cpp"
    "${CMAKE_SOURCE_DIR}/extern/imgui-node-editor/imgui_canvas.cpp"
    "${CMAKE_SOURCE_DIR}/extern/imgui-node-editor/crude_json.cpp"
    PROPERTIES COMPILE_FLAGS "/wd4100"
)
