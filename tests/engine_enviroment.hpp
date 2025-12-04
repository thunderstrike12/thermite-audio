#pragma once
#include <gtest/gtest.h>
#include "engine/engine.hpp"
#include "engine/core/logger.hpp"

#include <filesystem>

class TestApp : public tmt::Application {
   public:
    TestApp(const tmt::ApplicationSpecs& specs) : tmt::Application(specs) {}
    void on_start() override {}
    void on_update(const tmt::FrameData& time) override {}
    void on_end() override {}
};

class EngineEnvironment : public ::testing::Environment {
   public:
    void SetUp() override {
        std::string log_file = "log_tests.txt";
        if (std::filesystem::exists(log_file)) {
            std::filesystem::remove(log_file);
        }

        tmt::Log::init(log_file);
    }

    void TearDown() override {}
};
