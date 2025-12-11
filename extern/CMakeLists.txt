# Spdlog - Logging
# https://github.com/gabime/spdlog
set(SPDLOG_BUILD_SHARED  OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH   OFF CACHE BOOL "" FORCE)
add_subdirectory(spdlog)

# SDL3 - Windowing & Input
# https://github.com/libsdl-org/SDL
set(SDL_SHARED               OFF CACHE BOOL "" FORCE)
set(SDL_STATIC               ON  CACHE BOOL "" FORCE)
set(SDL_INSTALL				 OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS        OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL      ON  CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL_DOCS ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY         OFF CACHE BOOL "" FORCE)
set(SDL_TESTS                OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES             OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO    ON  CACHE BOOL "" FORCE)
set(SDL_VIDEO    ON  CACHE BOOL "" FORCE)
set(SDL_JOYSTICK ON  CACHE BOOL "" FORCE)
set(SDL_HIDAPI   ON CACHE BOOL "" FORCE)
set(SDL_HAPTIC   ON CACHE BOOL "" FORCE)
set(SDL_DIALOG   ON CACHE BOOL "" FORCE)

# Turn off everything else we don't care about:
set(SDL_GPU      OFF CACHE BOOL "" FORCE)
set(SDL_RENDER   OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA   OFF CACHE BOOL "" FORCE)
set(SDL_POWER    OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR   OFF CACHE BOOL "" FORCE)
set(SDL_TRAY     OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL3)

# GLM - Vector Math
# https://github.com/g-truc/glm
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
add_subdirectory(glm)

# ENTT - Entity Component System
# https://github.com/skypjack/entt
add_subdirectory(entt)

# Vulkan SDK - Graphics API
# https://vulkan.lunarg.com/sdk/home
set(VULKAN_HEADERS_INSTALL_DIR "${CMAKE_SOURCE_DIR}/extern/vulkan_sdk/")

# graphite - render graph library
# https://github.com/mxcop/graphite
set(GRAPHITE_SAMPLES OFF CACHE BOOL "" FORCE)
if (THERMITE_EDITOR_BUILD)
    set(GRAPHITE_IMGUI_DIR "${CMAKE_SOURCE_DIR}/extern/imgui/")
endif()
add_subdirectory(graphite)

# IMGUI - Immediate Mode GUI
# https://github.com/ocornut/imgui
if (THERMITE_EDITOR_BUILD)
    # Sources
    target_sources(thermite-editor
        PRIVATE "./imgui/imgui_demo.cpp"
        PRIVATE "./imgui/imgui_draw.cpp"
        PRIVATE "./imgui/imgui_tables.cpp"
        PRIVATE "./imgui/imgui_widgets.cpp"
        PRIVATE "./imgui/imgui.cpp"
        PRIVATE "./imgui/backends/imgui_impl_sdl3.cpp"
        PRIVATE "./imgui/backends/imgui_impl_vulkan.cpp"
        PRIVATE "./imgui/backends/imgui_impl_win32.cpp"
    )

    # Includes
    target_include_directories(thermite-editor
        PRIVATE "./imgui"
        PRIVATE "./imgui/backends"
    )
endif()

# visit_struct - struct reflection
# https://github.com/cbeck88/visit_struct
add_library(visit_struct INTERFACE)
target_include_directories(visit_struct
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/visit_struct/
)

# JsonReflect - json reflection
# https://github.com/Sven-vh/JsonReflect
add_library(JsonReflect INTERFACE)
target_include_directories(JsonReflect
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/JsonReflect/
    ${CMAKE_CURRENT_SOURCE_DIR}/JsonReflect/extern
)

# ImReflect - ImGui reflection
# https://github.com/Sven-vh/ImReflect
add_library(ImReflect INTERFACE)
target_include_directories(JsonReflect
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ImReflect/
    ${CMAKE_CURRENT_SOURCE_DIR}/ImReflect/extern
)

# nlohmann json - json parsing
# https://github.com/nlohmann/json
add_subdirectory(nlohmann_json)

if (THERMITE_EDITOR_BUILD)
    # ImGUI - immediate mode gui
    # https://github.com/ocornut/imgui
    target_sources(thermite-editor
        PRIVATE "./imgui/imgui_demo.cpp"
        PRIVATE "./imgui/imgui_draw.cpp"
        PRIVATE "./imgui/imgui_tables.cpp"
        PRIVATE "./imgui/imgui_widgets.cpp"
        PRIVATE "./imgui/imgui.cpp"
        PRIVATE "./imgui/backends/imgui_impl_sdl3.cpp"
        PRIVATE "./imgui/backends/imgui_impl_vulkan.cpp"
        PRIVATE "./imgui/backends/imgui_impl_win32.cpp"
        PRIVATE "./imgui/misc/cpp/imgui_stdlib.cpp"
    )
    target_include_directories(thermite-editor
        PRIVATE "./imgui"
        PRIVATE "./imgui/backends"
        PRIVATE "./imgui/misc/cpp"
    )

    target_compile_definitions(thermite-editor PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK)

    # ImGuizmo - ImGui based gizmo manipulation
    # https://github.com/CedricGuillemet/ImGuizmo
    target_sources(thermite-editor
        PRIVATE "./imguizmo/GraphEditor.cpp"
        PRIVATE "./imguizmo/ImCurveEdit.cpp"
        PRIVATE "./imguizmo/ImGradient.cpp"
        PRIVATE "./imguizmo/ImSequencer.cpp"
        PRIVATE "./imguizmo/GraphEditor.cpp"
        PRIVATE "./imguizmo/ImGuizmo.cpp"
    )
    target_include_directories(thermite-editor
        PRIVATE "./imguizmo"
    )

endif()


# tracy - profiler integration
# https://github.com/wolfpld/tracy
option(TRACY_ENABLE "" OFF)
option(TRACY_ON_DEMAND "" ON)

if(THERMITE_EDITOR_BUILD)
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(tracy)

# ufbx - single source fbx loading
# https://github.com/ufbx/ufbx
target_sources(thermite-engine
    PRIVATE "./ufbx/ufbx.cpp"
)
target_include_directories(thermite-engine
    PRIVATE "./ufbx"
)

# zlib - zip (un)compression 
# https://github.com/madler/zlib
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(zlib)

# Link engine libraries publicly
target_link_libraries(thermite-engine
    PUBLIC 
    spdlog::spdlog
    glm::glm
    EnTT::EnTT
    JsonReflect
    graphite
    Tracy::TracyClient
)

# Link engine libraries privately
target_link_libraries(thermite-engine 
	PRIVATE
	SDL3::SDL3-static
    visit_struct
    nlohmann_json::nlohmann_json
    ZLIB::ZLIBSTATIC
)

if (THERMITE_EDITOR_BUILD)
    # Link editor libraries privately
    target_link_libraries(thermite-editor
	    PRIVATE
	    SDL3::SDL3-static
        visit_struct
        nlohmann_json::nlohmann_json
    )
endif()
