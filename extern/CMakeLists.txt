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

# Link libraries publicly
target_link_libraries(thermite-engine
    PUBLIC 
    spdlog::spdlog
    glm::glm
    EnTT::EnTT
)

# Link libraries privately
target_link_libraries(thermite-engine 
	PRIVATE
	SDL3::SDL3-static
)
