#pragma once
#include <gtest/gtest.h>
#include "engine/engine.hpp"
#include <filesystem>

class EngineEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        std::string log_file = "log_tests.txt";
        if (std::filesystem::exists(log_file)) {
            std::filesystem::remove(log_file);
        }

        tmt::ApplicationSpecs specs {.name = "UnitTests", .log_file = log_file};
        tmt::engine.init(specs);
    }

    void TearDown() override { tmt::engine.end(); }
};
