#include <gtest/gtest.h>
#include "engine_enviroment.hpp"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new EngineEnvironment);
    return RUN_ALL_TESTS();
}
