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

class LogTestWithInit : public ::testing::Test {
   protected:
    static void SetUpTestSuite() {  // NOLINT(readability-identifier-naming)
        log_file =
            (std::filesystem::temp_directory_path() / "test_log_suite.txt")
                .string();
        if (std::filesystem::exists(log_file)) {
            std::filesystem::remove(log_file);
        }
        tmt::engine.init({.log_file = log_file});

        // Verify file was created
        EXPECT_TRUE(std::filesystem::exists(log_file))
            << "Log file not created at: " << log_file;
    }

    static void TearDownTestSuite() {  // NOLINT(readability-identifier-naming)
        spdlog::drop_all();
        spdlog::shutdown();
    }

    static std::string log_file;

    std::string read_log_file() const {
        spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& l) {
            l->flush();
        });

        EXPECT_TRUE(std::filesystem::exists(log_file))
            << "Log file missing: " << log_file;

        std::ifstream file(log_file);
        EXPECT_TRUE(file.is_open()) << "Failed to open: " << log_file;
        std::stringstream buffer;
        buffer << file.rdbuf();

        std::string content = buffer.str();
        std::cout << "=== Log file content ===\n"
                  << content << "\n=== End ===" << std::endl;

        return content;
    }
};

std::string LogTestWithInit::log_file;

TEST_F(LogTestWithInit, GlobalLoggingToConsoleOnly) {
    ASSERT_NO_THROW(tmt::Log::info("No scope here!"));
}

TEST_F(LogTestWithInit, ScopedLogging) {
    tmt::Log::warn(tmt::Log::Scope::GAME, "Game scope here!");
    tmt::Log::warn(tmt::Log::Scope::RENDERER, "Renderer scope here!");
    tmt::Log::warn(tmt::Log::Scope::ENGINE, "Engine scope here!");

    std::string content = read_log_file();
    EXPECT_TRUE(content.find("Game scope here!") != std::string::npos);
    EXPECT_TRUE(content.find("Renderer scope here!") != std::string::npos);
    EXPECT_TRUE(content.find("Engine scope here!") != std::string::npos);
}

TEST_F(LogTestWithInit, AllScopesWork) {
    ASSERT_NO_THROW(tmt::Log::info(tmt::Log::Scope::ENGINE, "Engine info"));
    ASSERT_NO_THROW(tmt::Log::info(tmt::Log::Scope::GAME, "Game info"));
    ASSERT_NO_THROW(tmt::Log::info(tmt::Log::Scope::RENDERER, "Renderer info"));

    std::string content = read_log_file();
    EXPECT_TRUE(content.find("Engine info") != std::string::npos);
    EXPECT_TRUE(content.find("Game info") != std::string::npos);
    EXPECT_TRUE(content.find("Renderer info") != std::string::npos);
}
