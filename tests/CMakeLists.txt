message(STATUS "Unit Tests are enabled")
# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Create test executable
add_executable(UnitTests)

# Find all test source files
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")
target_sources(UnitTests PRIVATE ${TEST_SOURCES})

# Link against GoogleTest and your engine
target_link_libraries(UnitTests 
    PRIVATE 
    GTest::gtest
    thermite-engine
)

# Copy FMOD DLLs for UnitTests
set(FMOD_POSTFIX "$<$<CONFIG:Debug>:L>")
add_custom_command(
    TARGET UnitTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/extern/fmod/lib/fmod${FMOD_POSTFIX}.dll"
            "${CMAKE_BINARY_DIR}/bin/fmod${FMOD_POSTFIX}.dll"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/extern/fmod/lib/fmodstudio${FMOD_POSTFIX}.dll"
            "${CMAKE_BINARY_DIR}/bin/fmodstudio${FMOD_POSTFIX}.dll"
)
set_target_properties(UnitTests PROPERTIES
     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

include(GoogleTest)
gtest_discover_tests(UnitTests
    DISCOVERY_MODE PRE_TEST
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)