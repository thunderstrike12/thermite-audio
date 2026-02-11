#include <gtest/gtest.h>
#include "engine/core/logger.hpp"
#include "engine/engine.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

TEST(LogTest, DoubleInitAsserts) {
    ASSERT_DEATH(
        {
            tmt::Log::init();
            tmt::Log::init();
        },
        ""
    );
}

TEST(LogTest, GlobalLoggingToConsoleOnly) {
    ASSERT_NO_THROW(tmt::Log::info("No scope here!"));
}
std::string log_file { "log_tests.txt" };
std::string read_log_file() {
    spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& l) { l->flush(); });

    EXPECT_TRUE(std::filesystem::exists(log_file)) << "Log file missing: " << log_file;

    std::ifstream file(log_file);
    EXPECT_TRUE(file.is_open()) << "Failed to open: " << log_file;
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string content = buffer.str();
    std::cout << "=== Log file content ===\n" << content << "\n=== End ===" << std::endl;

    return content;
}

;
TEST(LogTest, ScopedLogging) {
    tmt::Log::warn(tmt::Log::Scope::GAME, "Game scope here!");
    tmt::Log::warn(tmt::Log::Scope::RENDERER, "Renderer scope here!");
    tmt::Log::warn(tmt::Log::Scope::ENGINE, "Engine scope here!");

    std::string content = read_log_file();
    EXPECT_TRUE(content.find("Game scope here!") != std::string::npos);
    EXPECT_TRUE(content.find("Renderer scope here!") != std::string::npos);
    EXPECT_TRUE(content.find("Engine scope here!") != std::string::npos);
}

TEST(LogTest, AllScopesWork) {
    ASSERT_NO_THROW(tmt::Log::info(tmt::Log::Scope::ENGINE, "Engine info"));
    ASSERT_NO_THROW(tmt::Log::info(tmt::Log::Scope::GAME, "Game info"));
    ASSERT_NO_THROW(tmt::Log::info(tmt::Log::Scope::RENDERER, "Renderer info"));

    std::string content = read_log_file();
    EXPECT_TRUE(content.find("Engine info") != std::string::npos);
    EXPECT_TRUE(content.find("Game info") != std::string::npos);
    EXPECT_TRUE(content.find("Renderer info") != std::string::npos);
}
