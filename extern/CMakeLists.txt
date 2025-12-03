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
add_subdirectory(SDL3)

# GLM - Vector Math
# https://github.com/g-truc/glm
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
add_subdirectory(glm)

# ENTT - Entity Component System
# https://github.com/skypjack/entt
add_subdirectory(entt)

# graphite - render graph library
# https://github.com/mxcop/graphite
add_subdirectory(graphite)

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

# nlohmann json - json parsing
# https://github.com/nlohmann/json
add_subdirectory(nlohmann_json)

# ImGUI - immediate mode gui
# https://github.com/ocornut/imgui
target_sources(thermite-engine
    PRIVATE "./imgui/imgui_demo.cpp"
    PRIVATE "./imgui/imgui_draw.cpp"
    PRIVATE "./imgui/imgui_tables.cpp"
    PRIVATE "./imgui/imgui_widgets.cpp"
    PRIVATE "./imgui/imgui.cpp"
    PRIVATE "./imgui/backends/imgui_impl_sdl3.cpp"
    PRIVATE "./imgui/backends/imgui_impl_vulkan.cpp"
    PRIVATE "./imgui/backends/imgui_impl_win32.cpp"
)
target_include_directories(thermite-engine
    PRIVATE "./imgui"
    PRIVATE "./imgui/backends"
)

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

# Link libraries publicly
target_link_libraries(thermite-engine
    PUBLIC 
    spdlog::spdlog
    glm::glm
    EnTT::EnTT
    JsonReflect
    graphite
)

# Link libraries privately
target_link_libraries(thermite-engine 
	PRIVATE
	SDL3::SDL3-static
    visit_struct
    nlohmann_json::nlohmann_json
    ZLIB::ZLIBSTATIC
)
