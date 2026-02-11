#include <gtest/gtest.h>
#include "engine/tools/profiler.hpp"
#include <thread>
#include <chrono>

class TracyProfilerTest : public ::testing::Test {};

TEST_F(TracyProfilerTest, ZoneScopedWorks) {
    TMT_ZONE_SCOPED;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    SUCCEED();
}

TEST_F(TracyProfilerTest, NamedZoneWorks) {
    TMT_ZONE_SCOPED_N("TestNamedZone");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    SUCCEED();
}

TEST_F(TracyProfilerTest, NestedZonesWork) {
    TMT_ZONE_SCOPED_N("OuterZone");
    {
        TMT_ZONE_SCOPED_N("InnerZone");
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    SUCCEED();
}

TEST_F(TracyProfilerTest, FrameMarkWorks) {
    for (int i = 0; i < 3; ++i) {
        TMT_ZONE_SCOPED_N("SimulatedFrame");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        TMT_FRAME_MARK;
    }
    SUCCEED();
}

TEST_F(TracyProfilerTest, ZoneTextWorks) {
    TMT_ZONE_SCOPED;
    const char* msg = "Test zone text";
    TMT_ZONE_TEXT(msg, strlen(msg));
    SUCCEED();
}

TEST_F(TracyProfilerTest, ZoneValueWorks) {
    TMT_ZONE_SCOPED;
    TMT_ZONE_VALUE(42);
    SUCCEED();
}

#ifdef TRACY_ENABLE
TEST_F(TracyProfilerTest, TracyIsEnabled) {
    EXPECT_TRUE(true) << "TRACY_ENABLE is defined";
}
#else
TEST_F(TracyProfilerTest, TracyIsDisabled) {
    GTEST_SKIP() << "Tracy is disabled (TRACY_ENABLE not defined)";
}
#endif
