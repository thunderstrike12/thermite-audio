# TODO: link to external libraries here.

# Configure spdlog options 
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH  OFF CACHE BOOL "" FORCE)

add_subdirectory(spdlog)