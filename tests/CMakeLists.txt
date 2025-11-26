message(STATUS "### Unit Tests are enabled")

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
    GTest::gtest_main
    thermite-engine
)

# Include directories (if needed - thermite-engine should provide these)
# Uncomment if you need explicit include paths
# target_include_directories(UnitTests PRIVATE 
#     ${CMAKE_SOURCE_DIR}/engine
# )

# Set output directory
set_target_properties(UnitTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(UnitTests)