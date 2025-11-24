# TODO: link to external libraries here.

# Configure spdlog options 
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH  OFF CACHE BOOL "" FORCE)
add_subdirectory(spdlog)

# SDL3 - https://github.com/libsdl-org/SDL
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_DISABLE_INSTALL ON)
set(SDL_DISABLE_INSTALL_DOCS ON)
set(SDL_INSTALL_TESTS OFF)
add_subdirectory(SDL3)
target_link_libraries(thermite-engine PRIVATE SDL3::SDL3-static)